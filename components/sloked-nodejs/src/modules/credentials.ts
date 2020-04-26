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