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

import { NetSlaveServer } from './modules/netServer'
import { BinarySerializer } from './modules/serialize'
import * as net from 'net'
import { EncryptedStream } from './modules/encryptedStream'
import { Crypto, DefaultCrypto, EncryptedDuplexStream } from './modules/crypto'
import { Authenticator, SlaveAuthenticator } from './modules/authenticator'
import { ModifyableCredentialStorage, DefaultCredentialStorage } from './modules/credentials'
import { Service } from './types/server'
import { Pipe } from './types/pipe'
import { CompressedStream } from './modules/compressedStream'
import { Duplex } from 'stream'
import { DefaultServiceClient } from './modules/client'

class EchoService implements Service {
    async attach(pipe: Pipe): Promise<boolean> {
        pipe.read().then(val => pipe.write({ 'echo': val }))
        return true
    }
}

const socket = new net.Socket()
socket.connect(1234, '::1', async () => {
    const crypto: Crypto = new DefaultCrypto()
    const key: Buffer = await crypto.deriveKey('password', 'salt')
    const cryptoStream: EncryptedDuplexStream = new EncryptedStream(socket, crypto, key)
    const stream: Duplex = new CompressedStream(cryptoStream)
    const credentials: ModifyableCredentialStorage = new DefaultCredentialStorage(crypto)
    credentials.newAccount('user1', 'password1')
    const authenticator: Authenticator = new SlaveAuthenticator(crypto, credentials, 'salt', cryptoStream.getEncryption())
    const slave = new NetSlaveServer(stream, new BinarySerializer(), authenticator)
    await slave.authorize('user1')

    await slave.register('/plugins/echo', new EchoService())
    const echo: Pipe = await slave.connect('/plugins/echo')
    echo.write('Hello, world!')
    console.log(await echo.read())

    const root = new DefaultServiceClient(await slave.connect('/namespace/root'))
    const resp = await root.call('uri', '/usr/bin/bash')
    root.close()
    const cursor = new DefaultServiceClient(await slave.connect('/document/cursor'))
    await cursor.send('connect', {
        documentId: 1
    })
    await cursor.send('insert', resp)

    // const shutdown = await slave.connect('/editor/shutdown')
    // await shutdown.write(null)

    await cursor.close()
    await slave.close()
})
