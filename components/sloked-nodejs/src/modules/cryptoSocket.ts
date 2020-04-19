import { Transform, TransformOptions, TransformCallback, Duplex, DuplexOptions } from 'stream'
import { Crypto } from '../types/crypto'
import Crc32 from '../modules/crc32'

export class EncryptionStream extends Transform {
    constructor(crypto: Crypto, key: Buffer, options?: TransformOptions) {
        super(options)
        this._crypto = crypto
        this._key = key
    }

    _transform(chunk: Buffer, _: string, done: TransformCallback) {
        if (this._key === null || chunk == null) {
            done(null, chunk)
        } else {
            this._transformBlock(chunk, this._key).then(result => done(null, result), err => done(err, null))
        }
    }

    private async _transformBlock(chunk: Buffer, key: Buffer) {
        const blockSize = this._crypto.blockSize()
        const totalLength: number = chunk.length % blockSize != 0
            ? (Math.floor(chunk.length / blockSize) + 1) * blockSize
            : chunk.length
        const raw: Buffer = Buffer.alloc(totalLength)
        chunk.copy(raw)
        const iv = this._crypto.RandomBytes(this._crypto.IVSize())
        const crc32 = Crc32.Calculate(raw)
        const encrypted = await this._crypto.encrypt(raw, key, iv)
        const result: Buffer = Buffer.alloc(8 + iv.length + encrypted.length)
        result.writeUInt32LE(chunk.length, 0)
        result.writeUInt32LE(crc32, 4)
        iv.copy(result, 8)
        encrypted.copy(result, 8 + iv.length)
        return result
    }

    private _crypto: Crypto
    private _key: Buffer | null
}

export class DecryptionStream extends Transform {
    constructor(crypto: Crypto, key: Buffer, options?: TransformOptions) {
        super(options)
        this._crypto = crypto
        this._key = key
        this._encBuffer = Buffer.alloc(0)
    }

    _transform(chunk: Buffer, _: string, done: TransformCallback) {
        if (this._key === null || chunk == null) {
            done(null, chunk)
        } else {
            this._transformData(chunk, this._key).then(result => done(null, result), err => done(err, null))
        }
    }

    async _transformData(chunk: Buffer, key: Buffer): Promise<Buffer | null> {
        this._encBuffer = Buffer.concat([this._encBuffer, chunk])
        const EncryptedHeaderSize = 8 + this._crypto.IVSize()
        if (this._encBuffer.length < EncryptedHeaderSize) {
            return null
        }
        let result: Buffer | null = null
        const blockSize = this._crypto.blockSize()
        while (this._encBuffer.length >= EncryptedHeaderSize) {
            const length = this._encBuffer.readUInt32LE(0)
            const totalLength: number = length % blockSize != 0
                ? (Math.floor(length / blockSize) + 1) * blockSize
                : length
            if (this._encBuffer.length < EncryptedHeaderSize + totalLength) {
                break
            }
            const crc32 = this._encBuffer.readUInt32LE(4)
            const iv = this._encBuffer.slice(8, 8 + this._crypto.IVSize())
            const encrypted = this._encBuffer.slice(EncryptedHeaderSize, EncryptedHeaderSize + totalLength)
            this._encBuffer = this._encBuffer.slice(EncryptedHeaderSize + totalLength)
            const decrypted = await this._crypto.decrypt(encrypted, key, iv)
            const raw = decrypted.slice(0, length)
            const actualCrc32 = Crc32.Calculate(decrypted)
            if (actualCrc32 != crc32) {
                throw new Error('CRC32 mismatch')
            }
            result = result !== null
                ? Buffer.concat([result, raw])
                : raw
        }
        return result
    }

    private _crypto: Crypto
    private _key: Buffer | null
    private _encBuffer: Buffer
}

export class CryptoStream extends Duplex {
    constructor (rawStream: Duplex, crypto: Crypto, key: Buffer, options?: DuplexOptions) {
        super(options)
        this._raw = rawStream
        this._buffer = []
        this._autopush = false
        this._in = new DecryptionStream(crypto, key)
        this._out = new EncryptionStream(crypto, key)
        this._raw.pipe(this._in)
        this._out.pipe(this._raw)

        this._in.on('readable', () => {
            let chunk;
            while (null !== (chunk = this._in.read())) {
                this._buffer.push(chunk)
            }
            this.pushBuffer()
        })
        this._raw.on('pause', () => {
            this.emit('pause')
        })
        this._raw.on('resume', () => {
            this.emit('resume')
        })
        this._raw.on('error', err => {
            this.emit('error', err)
        })
        this._raw.on('end', () => {
            this.emit('end')
        })
        this._raw.on('close', () => {
            this.emit('close')
        })
    }

    _write(chunk: Buffer, enc: string, callback: any) {
        this._out.write(chunk, enc, callback)
    }

    async _read(_?: number) {
        this._autopush = true
        this.pushBuffer()
    }

    private pushBuffer() {
        let pushed = 0
        while (pushed < this._buffer.length && this._autopush) {
            this._autopush = this.push(this._buffer[pushed++])
        }
        this._buffer.splice(0, pushed)
    }

    private _raw: Duplex
    private _in: Transform
    private _out: Transform
    private _buffer: Buffer[]
    private _autopush: boolean
}