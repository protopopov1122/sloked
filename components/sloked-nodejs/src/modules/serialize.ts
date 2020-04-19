import { Serializer } from '../types/serialize'

export class JsonSerializer implements Serializer {
    serialize(msg: any): Buffer {
        return Buffer.from(JSON.stringify(msg))
    }

    deserialize(buffer: Buffer) {
        return JSON.parse(buffer.toString())
    }
}

enum BinarySerializerType {
    Null = 1,
    Integer8,
    Integer16,
    Integer32,
    Integer64,
    Float,
    BooleanTrue,
    BooleanFalse,
    String,
    Array,
    Object
}

export class BinarySerializer implements Serializer {
    serialize (value: any): Buffer {
        return this._serialize(value)
    }

    deserialize (buffer: Buffer): any {
        return this._deserialize(buffer)[0]
    }

    _serialize (value: any): Buffer {
        if (value === null || typeof value === 'undefined') {
            return Buffer.from([BinarySerializerType.Null])
        } else if (typeof value === 'number' && Number.isInteger(value)) {
            const type = Buffer.from([BinarySerializerType.Integer64])
            const buf = Buffer.alloc(8)
            buf.writeInt32LE(value & 0xffffffff, 0)
            buf.writeInt32LE((value >> 16) >> 16, 4)
            return Buffer.concat([type, buf])
        } else if (typeof value === 'number') {
            const type = Buffer.from([BinarySerializerType.Float])
            const buf = Buffer.alloc(8)
            buf.writeFloatLE(value)
            return Buffer.concat([type, buf])
        } else if (typeof value === 'boolean') {
            return Buffer.from([value ? BinarySerializerType.BooleanTrue : BinarySerializerType.BooleanFalse])
        } else if (typeof value === 'string') {
            const type = Buffer.from([BinarySerializerType.String])
            const length = Buffer.alloc(4)
            length.writeInt32LE(value.length, 0)
            const buf = Buffer.from(value)
            return Buffer.concat([type, length, buf])
        } else if (typeof value === 'object' && Array.isArray(value)) {
            const type = Buffer.from([BinarySerializerType.Array])
            const length = Buffer.alloc(4)
            length.writeInt32LE(value.length, 0)
            let buf = Buffer.alloc(0)
            for (let i = 0; i < value.length; i++) {
                buf = Buffer.concat([buf, this._serialize(value[i])])
            }
            return Buffer.concat([type, length, buf])
        } else if (typeof value === 'object') {
            const type = Buffer.from([BinarySerializerType.Object])
            const length = Buffer.alloc(4)
            length.writeInt32LE(Object.keys(value).length, 0)
            let buf = Buffer.alloc(0)
            for (let key of Object.keys(value)) {
                const keyLength = Buffer.alloc(4)
                keyLength.writeInt32LE(key.length, 0)
                const keyValue = Buffer.from(key)
                buf = Buffer.concat([buf, keyLength, keyValue, this._serialize(value[key])])
            }
            return Buffer.concat([type, length, buf])
        } else {
            console.log(value)
            throw new Error('Unsupported type')
        }
    }

    _deserialize (buffer: Buffer): [any, number] {
        switch (buffer[0]) {
            case BinarySerializerType.Null:
                return [null, 1]

            case BinarySerializerType.Integer8:
                return [buffer.readInt8(1), 2]

            case BinarySerializerType.Integer16:
                return [buffer.readInt16LE(1), 3]

            case BinarySerializerType.Integer32:
                return [buffer.readInt32LE(1), 5]

            case BinarySerializerType.Integer64:
                return [buffer.readInt32LE(1) | ((buffer.readInt32LE(5) << 16) << 16), 9]
            
            case BinarySerializerType.Float:
                return [buffer.readDoubleLE(1), 9]

            case BinarySerializerType.BooleanTrue:
                return [true, 1]

            case BinarySerializerType.BooleanFalse:
                return [false, 1]
          
            case BinarySerializerType.String: {
                const length = buffer.readInt32LE(1)
                const subbuf = buffer.slice(5, 5 + length)
                return [subbuf.toString(), 5 + length]
            }

            case BinarySerializerType.Array: {
                const length = buffer.readInt32LE(1)
                const array = []
                let offset = 5
                for (let i = 0; i < length; i++) {
                    const res = this._deserialize(buffer.slice(offset))
                    array.push(res[0])
                    offset += res[1]
                }
                return [array, offset]
            }

            case BinarySerializerType.Object: {
                const length = buffer.readInt32LE(1)
                const object: {[key: string]: any} = {}
                let offset = 5
                for (let i = 0; i < length; i++) {
                    const keyLength = buffer.readInt32LE(offset)
                    offset += 4
                    const key = buffer.slice(offset, offset + keyLength).toString()
                    offset += keyLength
                    const res = this._deserialize(buffer.slice(offset))
                    object[key] = res[0]
                    offset += res[1]
                }
                return [object, offset]
            }
        }
        console.log(buffer)
        throw new Error("Unsupported type")
    }
}
