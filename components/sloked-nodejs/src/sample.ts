import { NetSlaveServer } from './modules/netServer'
import { BinarySerializer } from './modules/serialize'
import * as net from 'net'
import { EncryptedStream } from './modules/encryptedStream'
import { DefaultCrypto } from './modules/crypto'
import { SlaveAuthenticator } from './modules/authenticator'
import { DefaultCredentialStorage } from './modules/credentials'
import { Service } from './types/server'
import { Pipe } from './types/pipe'
import { CompressedStream } from './modules/compressedStream'

class EchoService implements Service {
    async attach(pipe: Pipe): Promise<boolean> {
        pipe.read().then(val => pipe.write({ 'echo': val }))
        return true
    }
}

const socket = new net.Socket()
socket.connect(1234, '::1', async () => {
    const crypto = new DefaultCrypto()
    const key = await crypto.deriveKey('password', 'salt')
    const cryptoStream = new EncryptedStream(socket, crypto, key)
    const stream = new CompressedStream(cryptoStream)
    const credentials = new DefaultCredentialStorage(crypto)
    credentials.newAccount('user1', 'password1')
    const authenticator = new SlaveAuthenticator(crypto, credentials, 'salt', cryptoStream.getEncryption())
    const slave = new NetSlaveServer(stream, new BinarySerializer(), authenticator)
    await slave.authorize('user1')

    await slave.register('/plugins/echo', new EchoService())
    const echo = await slave.connect('/plugins/echo')
    echo.write('Hello, world!')
    console.log(await echo.read())

    const root = await slave.connect('/namespace/root')
    root.write({
        id: 1000,
        method: 'uri',
        params: '/usr/bin/bash'
    })
    const resp = await root.read()
    root.close()
    const cursor = await slave.connect('/document/cursor')
    cursor.write({
        id: 0,
        method: 'connect',
        params: 1
    })
    cursor.write({
        id: 1,
        method: 'insert',
        params: resp.result
    })
    
    setTimeout(() => cursor.close(), 1000)
    setTimeout(() => slave.close(), 1500)
})