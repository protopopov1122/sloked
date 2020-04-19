export default interface Crypto {
    deriveKey(password: string, salt: string): Promise<Buffer>;
    encrypt(data: Buffer, key: Buffer, iv: Buffer): Promise<Buffer>;
    decrypt(data: Buffer, key: Buffer, iv: Buffer): Promise<Buffer>;
    blockSize(): number;
}