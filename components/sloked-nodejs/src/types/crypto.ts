import { Duplex } from 'stream'

export interface Crypto {
    deriveKey(password: string, salt: string): Promise<Buffer>;
    encrypt(data: Buffer, key: Buffer, iv: Buffer): Promise<Buffer>;
    decrypt(data: Buffer, key: Buffer, iv: Buffer): Promise<Buffer>;
    blockSize(): number;
    IVSize(): number;
    RandomBytes(count: number): Buffer;
}

export interface StreamEncryption {
    setEncryptionKey(key?: Buffer, id?: string): void
    getEncryptionKey(): Buffer | undefined
    onKeyChange(listener: ((id?: string) => Promise<void>) | ((id?: string) => void)): () => void
}

export abstract class EncryptedDuplexStream extends Duplex {
    constructor(...args: any[]) {
        super(...args)
    }

    abstract getEncryption(): StreamEncryption
}