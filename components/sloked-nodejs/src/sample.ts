import SlaveServer from './modules/slave'
import { BinarySerializer } from './modules/serialize'
import * as net from 'net'
import { CryptoStream } from './modules/cryptoSocket'
import { DefaultCrypto } from './modules/crypto'

// (async () => {
//     const crypto = new DefaultCrypto()
//     const key = await crypto.deriveKey('1', '2')
//     const str = Readable.from(Buffer.from(JSON.stringify({
//         '1': 'Hello, world!',
//         'true': false,
//         'Hello': {
//             'a': []
//         }
//     }))).pipe(new EncryptionStream(crypto, key)).pipe(new DecryptionStream(crypto, key))
//     const encrypted = []
//     for await (let block of str) {
//         encrypted.push(block)
//     }
//     console.log(Buffer.concat(encrypted).toString())

//     process.exit()
// })()

const socket = new net.Socket()
socket.connect(1234, '::1', async () => {
    const crypto = new DefaultCrypto()
    const key = await crypto.deriveKey('password', 'salt')
    const slave = new SlaveServer(new CryptoStream(socket, crypto, key), new BinarySerializer())
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