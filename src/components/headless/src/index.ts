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
import * as net from 'net'
import { NetSlaveServer, AuthServer, BinarySerializer, EncryptedStream, Crypto,
    DefaultCrypto, EncryptedDuplexStream, ModifyableCredentialStorage,
    DefaultCredentialStorage, CompressedStream,
    Authenticator, SlaveAuthenticator } from 'sloked-nodejs'
import { Duplex } from 'stream'

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
        dim: 0.98 // TODO
    })
    await screenClient.Splitter.newWindow(mainWindow, {
        dim: 0.02, // TODO
        min: 1
    })
    console.log(mainWindow)
    //     screenClient.Splitter
    //         .NewWindow(mainWindow.value(), Splitter::Constraints(1.0f))
    //         .UnwrapWait();
    //     auto tabber =
    //         screenClient.Splitter
    //             .NewWindow(mainWindow.value(), Splitter::Constraints(0.0f, 1))
    //             .UnwrapWait();
    //     screenClient.Handle.NewTabber("/0/0");
    //     auto tab1 = screenClient.Tabber.NewWindow("/0/0").UnwrapWait();
    //     screenClient.Handle.NewTextEditor(
    //         tab1.value(), documentClient.GetId().UnwrapWait().value(),
    //         "default");
}

bootstrapEditor().once('ready', async () => {
    const editor = await initializeEditor('::1', 1234)
    initializeEditorScreen(editor)
}).start("./src/config.json")