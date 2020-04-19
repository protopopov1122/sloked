import * as crypto from 'crypto'
import Crypto from './int/crypto'

export class DefaultCrypto implements Crypto {
    deriveKey(password: string, salt: string): Promise<Buffer> {
        return new Promise<Buffer>((resolve, reject) => {
            crypto.scrypt(password, salt, 32, {
                N: 32768,
                r: 8,
                p: 1,
                maxmem: 32768 * 8 * 128 * 2
            }, (err, key) => {
                if (err) {
                    reject(err)
                } else {
                    resolve(key)
                }
            })
        })
    }

    encrypt(data: Buffer, key: Buffer, iv: Buffer): Promise<Buffer> {
        const Algorithm = 'aes-256-cbc'
        return new Promise<Buffer>((resolve, reject) => {
            const cipher = crypto.createCipheriv(Algorithm, key, iv)
            cipher.setAutoPadding(false)
            let encrypted = Buffer.alloc(0)
            cipher.on('readable', () => {
                let chunk;
                while (null !== (chunk = cipher.read())) {
                    encrypted = Buffer.concat([encrypted, chunk])
                }
            })
            cipher.on('error', reject)
            cipher.on('end', () => {
                resolve(encrypted)
            })
            cipher.write(data)
            cipher.end()
        })
    }

    decrypt(data: Buffer, key: Buffer, iv: Buffer): Promise<Buffer> {
        const Algorithm = 'aes-256-cbc'
        return new Promise<Buffer>((resolve, reject) => {
            const cipher = crypto.createDecipheriv(Algorithm, key, iv)
            cipher.setAutoPadding(false)
            let encrypted = Buffer.alloc(0)
            cipher.on('readable', () => {
                let chunk;
                while (null !== (chunk = cipher.read())) {
                    encrypted = Buffer.concat([encrypted, chunk])
                }
            })
            cipher.on('error', reject)
            cipher.on('end', () => {
                resolve(encrypted)
            })
            cipher.write(data)
            cipher.end()
        })
    }

    blockSize(): number {
        return 16 // AES-256-CBC
    }
}