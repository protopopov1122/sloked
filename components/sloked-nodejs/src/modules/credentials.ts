import { Crypto } from '../types/crypto'
import { CredentialAccount, CredentialStorage } from '../types/credentials'

class DefaultCredentialAccount implements CredentialAccount {
    constructor(crypto: Crypto, id: string, password: string) {
        this._crypto = crypto
        this._id = id
        this._password = password
    }

    getIdentifier(): string {
        return this._id
    }

    getPassword(): string {
        return this._password
    }

    setPassword(password: string) {
        this._password = password
    }

    deriveKey(salt: string): Promise<Buffer> {
        return this._crypto.deriveKey(this._password, salt)
    }

    private _crypto: Crypto;
    private _id: string
    private _password: string
}

export class DefaultCredentialStorage implements CredentialStorage {
    constructor(crypto: Crypto) {
        this._crypto = crypto
    }

    newAccount(id: string, password: string) {
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

    private _crypto: Crypto;
    private _accounts: { [id: string]: DefaultCredentialAccount } = {}
}