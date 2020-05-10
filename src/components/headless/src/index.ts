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
import * as net from 'net'
import { NetSlaveServer, AuthServer } from 'sloked-nodejs'
import { BinarySerializer } from 'sloked-nodejs'
import { EncryptedStream } from 'sloked-nodejs'
import { Crypto, DefaultCrypto, EncryptedDuplexStream } from 'sloked-nodejs'
import { Authenticator, SlaveAuthenticator } from 'sloked-nodejs'
import { ModifyableCredentialStorage, DefaultCredentialStorage } from 'sloked-nodejs'
import { CompressedStream } from 'sloked-nodejs'
import { DefaultServiceClient } from 'sloked-nodejs'
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

    process.on('SIGINT', function() {
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
    const authenticator: Authenticator  = new SlaveAuthenticator(crypto, credentials, 'salt', cryptoStream.getEncryption())
    const slave = new NetSlaveServer(stream, new BinarySerializer(), authenticator)
    await slave.authorize('user1')
    return slave
}

async function initializeEditorScreen(editor: AuthServer<string>): Promise<void> {
    const screenClient = new DefaultServiceClient(await editor.connect('/screen/manager'))
    // await screenClient.call('handle.newMultiplexer', '/')
    await screenClient.close()
    // screenClient.Handle.NewMultiplexer("/");
    //     auto mainWindow =
    //         screenClient.Multiplexer
    //             .NewWindow(
    //                 "/", TextPosition{0, 0},
    //                 screenSizeClient.GetSize())
    //             .UnwrapWait();
    //     screenSizeClient.Listen([&](const auto &size) {
    //         sharedState.GetThreadedExecutor().Enqueue([&, size] {
    //             if (mainWindow.has_value()) {
    //                 screenClient.Multiplexer
    //                     .ResizeWindow(mainWindow.value(), size)
    //                     .UnwrapWait();
    //             }
    //         });
    //     });
    //     screenClient.Handle.NewSplitter(mainWindow.value(),
    //                                     Splitter::Direction::Vertical);
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