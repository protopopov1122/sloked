import SlaveServer from './modules/slave'
import { BinarySerializer } from './modules/serialize'
import * as net from 'net'
import { CryptoStream } from './modules/cryptoSocket'
import { DefaultCrypto } from './modules/crypto'
import { SlaveAuthenticator } from './modules/authenticator'
import { DefaultCredentialStorage } from './modules/credentials'

const socket = new net.Socket()
socket.connect(1234, '::1', async () => {
    const crypto = new DefaultCrypto()
    const key = await crypto.deriveKey('password', 'salt')
    const stream = new CryptoStream(socket, crypto, key)
    const credentials = new DefaultCredentialStorage(crypto)
    credentials.newAccount('user1', 'password1')
    const authenticator = new SlaveAuthenticator(crypto, credentials, 'salt', stream.getEncryption())
    const slave = new SlaveServer(stream, new BinarySerializer(), authenticator)
    await slave.authorize('user1')
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