import { Transform, TransformOptions, TransformCallback, Duplex, DuplexOptions } from 'stream'
import { Crypto, StreamEncryption, EncryptedDuplexStream } from '../types/crypto'
import { EventEmitter } from '../modules/eventEmitter'
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

    async encrypt(crypto: Crypto, key?: Buffer): Promise<Buffer> {
        if (this._payload.length > 0) {
            const iv = crypto.RandomBytes(crypto.IVSize())
            const encrypted = key
                ? await crypto.encrypt(this._payload, key, iv)
                : this._payload
            const result: Buffer = Buffer.alloc(9 + iv.length + encrypted.length)
            const signature = await Frame.sign(this._checksum, async (buf: Buffer) => key ? crypto.encrypt(buf, key, iv) : buf)
            result.writeUInt8(this._type)
            result.writeUInt32LE(encrypted.length, 1)
            result.writeUInt32LE(signature, 5)
            iv.copy(result, 9)
            encrypted.copy(result, 9 + iv.length)
            return result;
        } else {
            return Buffer.from([this._type, 0, 0, 0, 0])
        }
    }

    static async decrypt(crypto: Crypto, input: Buffer, key?: Buffer): Promise<[Frame, number] | null> {
        const MinimalHeaderLength: number = 5;
        if (input.length < MinimalHeaderLength) {
            return null
        }
        if (typeof FrameType[input[0]] === 'undefined') {
            throw new Error("CryptoSocket: Invalid frame type");
        }
        const type: FrameType = input[0]
        const totalLength: number = input.readUInt32LE(1)
        if (totalLength > 0) {
            const EncryptedHeaderSize: number = 9 + crypto.IVSize();
            const signature: number = input.readUInt32LE(5)
            if (input.length < totalLength + EncryptedHeaderSize) {
                return null
            }

            const iv: Buffer = input.slice(EncryptedHeaderSize - crypto.IVSize(), EncryptedHeaderSize)
            const encrypted: Buffer = input.slice(EncryptedHeaderSize, EncryptedHeaderSize + totalLength)
            const raw = key
                ? await crypto.decrypt(encrypted, key, iv)
                : encrypted
            const actualSignature = await Frame.sign(Crc32.Calculate(raw), async (buf: Buffer) => key ? crypto.encrypt(buf, key, iv) : buf)
            if (actualSignature != signature) {
                throw new Error(
                    "CryptoSocket: Actual CRC32 doesn't equal to expected");
            }
            return [new Frame(type, raw), EncryptedHeaderSize + totalLength]
        } else {
            return [new Frame(type, Buffer.alloc(0)), MinimalHeaderLength]
        }
    }

    static async sign(checksum: number, encrypt: (buf: Buffer) => Promise<Buffer>): Promise<number> {
        const rawSignatureBuffer = Buffer.alloc(4)
        rawSignatureBuffer.writeUInt32LE(checksum)
        const signatureBuffer = await encrypt(rawSignatureBuffer)
        return Crc32.Calculate(signatureBuffer)   
    }

    private _type: FrameType
    private _checksum: number
    private _payload: Buffer
}

export class EncryptionStream extends Transform {
    constructor(crypto: Crypto, key?: Buffer, options?: TransformOptions) {
        super(options)
        this._crypto = crypto
        this._key = key
        this._pending = Buffer.alloc(0)
    }

    async setKey (key?: Buffer, id?: string): Promise<void> {
        if (id) {
            const frame: Frame = new Frame(FrameType.KeyChange, id !== null ? Buffer.from(id) : Buffer.alloc(0))
            const encrypted = await frame.encrypt(this._crypto, this._key)
            this._pending = Buffer.concat([this._pending, encrypted])
        }
        this._key = key
    }

    async _transform(chunk: Buffer, _: string, done: TransformCallback): Promise<void> {
        let result = Buffer.alloc(0)
        if (chunk != null) {
            result = await this._transformBlock(chunk, this._key)
        }
        result = Buffer.concat([this._pending, result])
        this._pending = Buffer.alloc(0)
        done(null, result)
    }

    private async _transformBlock(chunk: Buffer, key?: Buffer): Promise<Buffer> {
        const frame: Frame = new Frame(FrameType.Data, chunk)
        return frame.encrypt(this._crypto, key)
    }

    private _crypto: Crypto
    private _key?: Buffer
    private _pending: Buffer
}

export class DecryptionStream extends Transform {
    constructor(crypto: Crypto, keyChangeEmitter: EventEmitter<string | null>, key?: Buffer, options?: TransformOptions) {
        super(options)
        this._crypto = crypto
        this._key = key
        this._encBuffer = Buffer.alloc(0)
        this._keyChangeEmitter = keyChangeEmitter
    }

    setKey (key?: Buffer): void {
        this._key = key
    }

    _transform(chunk: Buffer, _: string, done: TransformCallback): void {
        if (chunk == null) {
            done(null, chunk)
        } else {
            this._transformData(chunk, this._key).then(result => {
                if (result) {
                    done(null, result)
                } else {
                    done(null, Buffer.alloc(0))
                }
            }, err => done(err, null))
        }
    }

    async _transformData(chunk: Buffer, key?: Buffer): Promise<Buffer | null> {
        const result = await Frame.decrypt(this._crypto, chunk, key)
        if (result !== null) {
            const [frame, length] = result
            this._encBuffer = this._encBuffer.slice(length)
            switch (frame.getType()) {
                case FrameType.Data:
                    return result[0].getPayload()
                
                case FrameType.KeyChange:
                    await this._keyChangeEmitter.emit(frame.getPayload().length > 0 ? frame.getPayload().toString() : null)
                    break
            }
        }
        return null
    }

    private _crypto: Crypto
    private _key?: Buffer
    private _encBuffer: Buffer
    private _keyChangeEmitter: EventEmitter<string | null>
}

export class CryptoStream extends EncryptedDuplexStream implements StreamEncryption {
    constructor (rawStream: Duplex, crypto: Crypto, key?: Buffer, options?: DuplexOptions) {
        super(options)
        this._raw = rawStream
        this._buffer = []
        this._autopush = false
        this._keyChangeEmitter = new EventEmitter<string | null>()
        this._key = key
        this._in = new DecryptionStream(crypto, this._keyChangeEmitter, key)
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

    getEncryption(): StreamEncryption {
        return this
    }

    setEncryptionKey(key?: Buffer, id?: string): void {
        this._in.setKey(key)
        this._out.setKey(key, id)
    }

    getEncryptionKey(): Buffer | undefined {
        return this._key
    }

    onKeyChange(listener: ((id?: string) => Promise<void>) | ((id?: string) => void)): () => void {
        return this._keyChangeEmitter.asyncSubscribe(async val => {
            if (val) {
                await listener(val)
            } else {
                await listener()
            }
        })
    }

    _write(chunk: Buffer, enc: string, callback: any): void {
        this._out.write(chunk, enc, callback)
    }

    async _read(_?: number): Promise<void> {
        this._autopush = true
        this.pushBuffer()
    }

    private pushBuffer(): void {
        let pushed: number = 0
        while (pushed < this._buffer.length && this._autopush) {
            this._autopush = this.push(this._buffer[pushed++])
        }
        this._buffer.splice(0, pushed)
    }

    private _raw: Duplex
    private _in: DecryptionStream
    private _out: EncryptionStream
    private _buffer: Buffer[]
    private _autopush: boolean
    private _key?: Buffer
    private _keyChangeEmitter: EventEmitter<string | null>
}