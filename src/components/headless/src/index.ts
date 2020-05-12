/*
  SPDX-License-Identifier: LGPL-3.0

  Copyright (c) 2019-2020 Jevgenijs Protopopovs

  This file is part of Sloked project.

  Sloked is free software: you can redistribute it and/or modify
  it under the terms of the GNU Lesser General Public License version 3 as
  published by the Free Software Foundation.


  Sloked is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public License
  along with Sloked.  If not, see <http://www.gnu.org/licenses/>.
*/

import { SlokedApplication } from './lib/application'
import { ScreenClient, ScreenSizeClient, ScreenSplitterDirection } from './lib/clients/screen'
import { ScreenInputClient, KeyInputEvent, ControlKeyCode } from './lib/clients/screenInput'
import { DocumentSetClient } from './lib/clients/documents'
import * as net from 'net'
import { NetSlaveServer, AuthServer, BinarySerializer, EncryptedStream, Crypto,
    DefaultCrypto, EncryptedDuplexStream, ModifyableCredentialStorage,
    DefaultCredentialStorage, CompressedStream,
    Authenticator, SlaveAuthenticator } from 'sloked-nodejs'
import { Duplex } from 'stream'
import * as path from 'path'
import { ShutdownClient } from './lib/clients/shutdown'

const bootstrap = process.argv[2]
const applicationLibrary = process.argv[3]

function bootstrapEditor(): SlokedApplication {
    const editor: SlokedApplication = new SlokedApplication({
        bootstrap,
        applicationLibrary
    })

    editor.on('error', err => console.log('Application error: ', err))

    editor.on('exit', () => {
        console.log('Application exiting')
        process.exit(0)
    })

    process.on('SIGINT', function () {
        editor.terminate()
    })
    return editor
}

async function initializeEditor(host: string, port: number): Promise<NetSlaveServer> {
    const socket = new net.Socket()
    await new Promise<any>(resolve => {
        socket.connect(port, host, resolve)
    })

    const crypto: Crypto = new DefaultCrypto()
    const key: Buffer = await crypto.deriveKey('password', 'salt')
    const cryptoStream: EncryptedDuplexStream = new EncryptedStream(socket, crypto, key)
    const stream: Duplex = new CompressedStream(cryptoStream)
    const credentials: ModifyableCredentialStorage = new DefaultCredentialStorage(crypto)
    credentials.newAccount('user1', 'password1')
    const authenticator: Authenticator = new SlaveAuthenticator(crypto, credentials, 'salt', cryptoStream.getEncryption())
    const slave = new NetSlaveServer(stream, new BinarySerializer(), authenticator)
    await slave.authorize('user1')
    return slave
}

async function initializeEditorScreen(editor: AuthServer<string>): Promise<void> {
    const documentSetClient = new DocumentSetClient(await editor.connect('/document/manager'))
    const docId = await documentSetClient.open(path.resolve(process.argv[4]), 'system', 'system', 'default')
    console.log(docId)

    const screenSizeClient = new ScreenSizeClient()
    await screenSizeClient.connect(await editor.connect('/screen/size/notify'))
    const screenClient = new ScreenClient(await editor.connect('/screen/manager'))
    await screenClient.Handle.newMultiplexer('/')
    const mainWindow = await screenClient.Multiplexer.newWindow('/', {x: 0, y: 0}, screenSizeClient.size())
    screenSizeClient.on('resize', (size: {width: number, height: number}) => {
        screenClient.Multiplexer.resizeWidth(mainWindow, size)
    })
    await screenClient.Handle.newSplitter(mainWindow, ScreenSplitterDirection.Vertical)
    await screenClient.Splitter.newWindow(mainWindow, {
        dim: 1
    })
    await screenClient.Splitter.newWindow(mainWindow, {
        dim: 0,
        min: 1
    })
    await screenClient.Handle.newTabber('/0/0')
    await screenClient.Tabber.newWindow('/0/0')
    await screenClient.Handle.newTextEditor('/0/0/0', docId, 'default')

    const screenInput = new ScreenInputClient(await editor.connect('/screen/component/input/notify'))
    await screenInput.on('input', async (evt: KeyInputEvent) => {
        if (evt.key === ControlKeyCode.Escape && !evt.alt) {
            await documentSetClient.saveAs(path.resolve(process.argv[5]))
            const shutdown = new ShutdownClient(await editor.connect('/editor/shutdown'))
            await shutdown.requestShutdown()
        }
    }).listenAll('/', true)
}

bootstrapEditor().once('ready', async () => {
    const editor = await initializeEditor('::1', 1234)
    initializeEditorScreen(editor)
}).start("./src/config.json")
