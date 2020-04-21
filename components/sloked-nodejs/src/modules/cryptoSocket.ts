import { Transform, TransformOptions, TransformCallback, Duplex, DuplexOptions } from 'stream'
import { Crypto } from '../types/crypto'
import Crc32 from '../modules/crc32'

enum FrameType {
    Data = 0,
    KeyChange = 1
}

class Frame {
    constructor (type: FrameType, payload: Buffer) {
        this._type = type
        this._checksum = Crc32.Calculate(payload)
        this._payload = payload
    }

    getType(): FrameType {
        return this._type
    }

    getChecksum(): number {
        return this._checksum
    }

    getPayload(): Buffer {
        return this._payload
    }

    async encrypt(crypto: Crypto, key: Buffer) {
        if (this._payload.length > 0) {
            let totalLength: number = this._payload.length
            if (totalLength % crypto.blockSize() != 0) {
                totalLength =
                    Math.ceil(totalLength / crypto.blockSize()) *
                    crypto.blockSize()
            }
            const raw: Buffer = Buffer.alloc(totalLength)
            this._payload.copy(raw)
            const iv = crypto.RandomBytes(crypto.IVSize())
            const encrypted = await crypto.encrypt(raw, key, iv)
            const result: Buffer = Buffer.alloc(9 + iv.length + encrypted.length)
            result.writeUInt8(this._type)
            result.writeUInt32LE(this._payload.length, 1)
            result.writeUInt32LE(this._checksum, 5)
            iv.copy(result, 9)
            encrypted.copy(result, 9 + iv.length)
            console.log(result)
            return result;
        } else {
            return Buffer.from([this._type, 0, 0, 0, 0])
        }
    }

    static async decrypt(crypto: Crypto, key: Buffer, input: Buffer): Promise<[Frame, number] | null> {
        const MinimalHeaderLength: number = 5;
        if (input.length < MinimalHeaderLength) {
            return null
        }
        if (typeof FrameType[input[0]] === 'undefined') {
            throw new Error("CryptoSocket: Invalid frame type");
        }
        const type: FrameType = input[0]
        const length: number = input.readUInt32LE(1)
        if (length > 0) {
            const EncryptedHeaderSize: number = 9 + crypto.IVSize();
            const crc32: number = input.readUInt32LE(5)
            let totalLength: number = length
            if (totalLength % crypto.blockSize() != 0) {
                totalLength =
                    Math.ceil(totalLength / crypto.blockSize()) *
                    crypto.blockSize()
            }
            if (input.length < totalLength + EncryptedHeaderSize) {
                return null
            }

            const iv: Buffer = input.slice(EncryptedHeaderSize - crypto.IVSize(), EncryptedHeaderSize)
            const encrypted: Buffer = input.slice(EncryptedHeaderSize, EncryptedHeaderSize + totalLength)
            const raw = (await crypto.decrypt(encrypted, key, iv)).slice(0, length)
            const actualCrc32: number = 
                Crc32.Calculate(raw);
            if (actualCrc32 != crc32) {
                throw new Error(
                    "CryptoSocket: Actual CRC32 doesn't equal to expected");
            }
            return [new Frame(type, raw), EncryptedHeaderSize + totalLength]
        } else {
            return [new Frame(type, Buffer.alloc(0)), MinimalHeaderLength]
        }
    }

    private _type: FrameType
    private _checksum: number
    private _payload: Buffer
}

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
        const frame: Frame = new Frame(FrameType.Data, chunk)
        return frame.encrypt(this._crypto, key)
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
        const result = await Frame.decrypt(this._crypto, key, chunk)
        if (result !== null) {
            this._encBuffer = this._encBuffer.slice(result[1])
            if (result[0].getType() == FrameType.Data) {
                return result[0].getPayload()
            } else {
                return null
            }
        } else {
            return null
        }
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