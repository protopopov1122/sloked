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

import { Crypto, StreamEncryption } from '../types/crypto'
import { CredentialStorage } from '../types/credentials'
import { Authenticator } from '../types/authenticator'

export class SlaveAuthenticator implements Authenticator {
    constructor (crypto: Crypto, credentialProvider: CredentialStorage, salt: string, encryption: StreamEncryption) {
        this._crypto = crypto
        this._credentialProvider = credentialProvider
        this._salt = salt
        this._encryption = encryption
        this._account = undefined
        this._pending = undefined
        this._unwatchCredentials = undefined
        this._initialKey = encryption.getEncryptionKey()
        this._unwatchKeyChange = this._encryption.onKeyChange(async (id?: string) => {
            if (id === this._pending) {
                if (id) {
                    this._account = this._pending
                    this._pending = undefined
                    await this._setupEncryption()
                } else {
                    this._account = undefined
                    this._logout()
                }
            } else {
                throw new Error('Unexpected key change')
            }
        })
    }

    close(): void {
        if (this._unwatchKeyChange) {
            this._unwatchKeyChange()
        }
    }

    async authenticate(account: string, nonce: number): Promise<string> {
        this._account = undefined
        this._pending = account
        const acc = this._credentialProvider.getAccount(account)
        const key: Buffer = await acc.deriveKey(this._salt)
        const nonceBuffer: Buffer = Buffer.alloc(4)
        nonceBuffer.writeUInt32LE(nonce >>> 0)
        const iv: Buffer = Buffer.alloc(this._crypto.IVSize())
        const encryptedNonce = await this._crypto.encrypt(nonceBuffer, key, iv)
        const token = encryptedNonce.toString('base64')
        return token
    }

    private async _setupEncryption(): Promise<void> {
        if (this._unwatchCredentials) {
            this._unwatchCredentials()
            this._unwatchCredentials = undefined
        }
        if (this._account) {
            const acc = this._credentialProvider.getAccount(this._account)
            this._unwatchCredentials = acc.watch(async () => {
                const key: Buffer = await acc.deriveKey(this._salt)
                this._encryption.setEncryptionKey(key)
            })
            const key: Buffer = await acc.deriveKey(this._salt)
            this._encryption.setEncryptionKey(key)
        }
    }

    private _logout(): void {
        if (this._unwatchCredentials) {
            this._unwatchCredentials()
            this._unwatchCredentials = undefined
        }
        this._encryption.setEncryptionKey(this._initialKey)
    }

    private _crypto: Crypto
    private _credentialProvider: CredentialStorage
    private _salt: string
    private _encryption: StreamEncryption
    private _account?: string
    private _pending?: string
    private _unwatchKeyChange?: () => void
    private _unwatchCredentials?: () => void
    private _initialKey?: Buffer
}
