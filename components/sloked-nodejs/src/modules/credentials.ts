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

import { Crypto } from '../types/crypto'
import { CredentialAccount, CredentialStorage } from '../types/credentials'
import { EventEmitter } from '../modules/eventEmitter'

class DefaultCredentialAccount implements CredentialAccount {
    constructor(crypto: Crypto, id: string, password: string) {
        this._crypto = crypto
        this._id = id
        this._password = password
        this._eventEmitter = new EventEmitter<void>()
    }

    close() {
        this._password = ''
        this._eventEmitter.emit()
    }

    getIdentifier(): string {
        return this._id
    }

    getPassword(): string {
        return this._password
    }

    setPassword(password: string): void {
        this._password = password
        this._eventEmitter.emit()
    }

    deriveKey(salt: string): Promise<Buffer> {
        return this._crypto.deriveKey(this._password, salt)
    }

    watch(callback: () => void): () => void {
        return this._eventEmitter.subscribe(callback)
    }

    private _crypto: Crypto;
    private _id: string
    private _password: string
    private _eventEmitter: EventEmitter<void>
}

export class DefaultCredentialStorage implements CredentialStorage {
    constructor(crypto: Crypto) {
        this._crypto = crypto
    }

    newAccount(id: string, password: string): void {
        this._accounts[id] = new DefaultCredentialAccount(this._crypto, id, password)
    }

    hasAccount(id: string): boolean {
        return typeof this._accounts[id] !== 'undefined'
    }

    getAccount(id: string): DefaultCredentialAccount {
        if (this.hasAccount(id)) {
            return this._accounts[id]
        } else {
            throw new Error(`No account '${id}'`)
        }
    }

    deleteAccount(id: string): void {
        if (this._accounts[id]) {
            this._accounts[id].close()
            delete this._accounts[id]
        } else {
            throw new Error(`No account '${id}'`)
        }
    }

    private _crypto: Crypto;
    private _accounts: { [id: string]: DefaultCredentialAccount } = {}
}
