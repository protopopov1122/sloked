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

import { Transform, TransformOptions, TransformCallback, Duplex, DuplexOptions } from 'stream'
import * as zlib from 'zlib'
import * as VLQ from './vlq'

function Deflate(chunk: Buffer): Promise<Buffer> {
    return new Promise<Buffer>((resolve, reject) => {
        zlib.deflate(chunk, (err, result) => {
            if (err) {
                reject(err)
            } else {
                resolve(result)
            }
        })
    })
}

function Inflate(chunk: Buffer): Promise<Buffer> {
    return new Promise<Buffer>((resolve, reject) => {
        zlib.inflate(chunk, (err, result) => {
            if (err) {
                reject(err)
            } else {
                resolve(result)
            }
        })
    })
}

class Frame {
    constructor(payload: Buffer) {
        this._payload = payload
    }

    getPayload(): Buffer {
        return this._payload
    }

    async encode(): Promise<Buffer> {
        const deflated: Buffer = await Deflate(this._payload)
        const lengthBuf: Buffer = VLQ.Encode(this._payload.length)
        const deflatedLengthBuf: Buffer = VLQ.Encode(deflated.length)
        return Buffer.concat([lengthBuf, deflatedLengthBuf, deflated])
    }

    static async decode(input: Buffer): Promise<[Frame | null, number]> {
        const initialLength: number = input.length
        const lengthDecoded = VLQ.Decode(input)
        if (lengthDecoded === null) {
            return [null, 0]
        }
        const length: number = lengthDecoded[0]
        input = lengthDecoded[1]
        const deflatedLengthDecoded = VLQ.Decode(input)
        if (deflatedLengthDecoded === null) {
            return [null, 0]
        }
        const deflatedLength: number = deflatedLengthDecoded[0]
        input = deflatedLengthDecoded[1]
        if (input.length < deflatedLength) {
            return [null, 0]
        }
        const headerLength: number = initialLength - input.length
        const payload = await Inflate(input)
        if (length !== payload.length) {
            throw new Error('CompressionStream: Expected length does not correspond to actual')
        }
        return [new Frame(payload), headerLength + deflatedLength]
    }

    private _payload: Buffer
}

export class CompressionStream extends Transform {
    constructor(options?: TransformOptions) {
        super(options)
    }

    async _transform(chunk: Buffer, _: string, done: TransformCallback) {
        const frame: Frame = new Frame(chunk)
        try {
            const encoded = await frame.encode()
            done(null, encoded)
        } catch (err) {
            done(err, null)
        }
    }
}

export class DecompressionStream extends Transform {
    constructor(options?: TransformOptions) {
        super(options)
        this._buffer = Buffer.alloc(0)
    }

    async _transform(chunk: Buffer, _: string, done: TransformCallback): Promise<void> {
        if (chunk == null) {
            done(null, chunk)
        } else {
            try {
                let result: Buffer[] = []
                let currentChunk = Buffer.concat([this._buffer, chunk])
                while (currentChunk.length > 0) {
                    const [fragment, tail]: [Buffer | null, Buffer] = await this._transformData(currentChunk)
                    currentChunk = tail
                    if (fragment !== null) {
                        result.push(fragment)
                    } else {
                        break
                    }
                }
                this._buffer = currentChunk
                done(null, Buffer.concat(result))
            } catch (err) {
                done(err, null)
            }
        }
    }

    async _transformData(chunk: Buffer): Promise<[Buffer | null, Buffer]> {
        const [frame, length]: [Frame | null, number] = await Frame.decode(chunk)
        if (frame) {
            return [frame.getPayload(), chunk.slice(length)]
        } else {
            return [null, chunk]
        }
    }

    private _buffer: Buffer
}

export class CompressedStream extends Duplex {
    constructor (rawStream: Duplex, options?: DuplexOptions) {
        super(options)
        this._raw = rawStream
        this._buffer = []
        this._autopush = true
        this._in = new DecompressionStream()
        this._out = new CompressionStream()
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
    private _in: DecompressionStream
    private _out: CompressionStream
    private _buffer: Buffer[]
    private _autopush: boolean
}
