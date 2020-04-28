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

import * as crypto from 'crypto'
import { Crypto } from '../types/crypto'

export * from '../types/crypto'

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
        const Algorithm: string = 'aes-256-cbc'
        return new Promise<Buffer>((resolve, reject) => {
            const cipher: crypto.Cipher = crypto.createCipheriv(Algorithm, key, iv)
            let encrypted: Buffer[] = []
            cipher.on('readable', () => {
                let chunk: Buffer | null;
                while ((chunk = cipher.read()) !== null) {
                    encrypted.push(chunk)
                }
            })
            cipher.on('error', reject)
            cipher.on('end', () => {
                resolve(Buffer.concat(encrypted))
            })
            cipher.write(data)
            cipher.end()
        })
    }

    decrypt(data: Buffer, key: Buffer, iv: Buffer): Promise<Buffer> {
        const Algorithm: string = 'aes-256-cbc'
        return new Promise<Buffer>((resolve, reject) => {
            const cipher: crypto.Decipher = crypto.createDecipheriv(Algorithm, key, iv)
            let encrypted: Buffer[] = []
            cipher.on('readable', () => {
                let chunk: Buffer | null;
                while (null !== (chunk = cipher.read())) {
                    encrypted.push(chunk)
                }
            })
            cipher.on('error', reject)
            cipher.on('end', () => {
                resolve(Buffer.concat(encrypted))
            })
            cipher.write(data)
            cipher.end()
        })
    }

    blockSize(): number {
        return 16 // AES-256-CBC
    }

    IVSize(): number {
        return 16 // AES-256-CBC
    }

    RandomBytes(count: number): Buffer {
        return crypto.randomBytes(count)
    }
}
