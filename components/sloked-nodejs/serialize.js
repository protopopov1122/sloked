class JsonSerializer {
    serialize(msg) {
        return Buffer.from(JSON.stringify(msg))
    }

    deserialize(buffer) {
        return JSON.parse(buffer.toString())
    }
}

class BinarySerializer {
    static Types = {
        Null: 1,
        Integer8: 2,
        Integer16: 3,
        Integer32: 4,
        Integer64: 5,
        Float: 6,
        BooleanTrue: 7,
        BooleanFalse: 8,
        String: 9,
        Array: 10,
        Object: 11
    }

    serialize (value) {
        return this._serialize(value)
    }

    deserialize (buffer) {
        return this._deserialize(buffer)[0]
    }

    _serialize (value) {
        if (value === null) {
            return Buffer.from([BinarySerializer.Types.Null])
        } else if (typeof value === 'number' && Number.isInteger(value)) {
            const type = Buffer.from([BinarySerializer.Types.Integer64])
            const buf = Buffer.alloc(8)
            buf.writeInt32LE(value & 0xffffffff, 0)
            buf.writeInt32LE((value >> 16) >> 16, 4)
            return Buffer.concat([type, buf])
        } else if (typeof value === 'number') {
            const type = Buffer.from([BinarySerializer.Types.Float])
            const buf = Buffer.alloc(8)
            buf.writeFloatLE(value)
            return Buffer.concat([type, buf])
        } else if (typeof value === 'boolean') {
            return Buffer.from([value ? BinarySerializer.Types.BooleanTrue : BinarySerializer.Types.BooleanFalse])
        } else if (typeof value === 'string') {
            const type = Buffer.from([BinarySerializer.Types.String])
            const length = Buffer.alloc(4)
            length.writeInt32LE(value.length, 0)
            const buf = Buffer.from(value)
            return Buffer.concat([type, length, buf])
        } else if (typeof value === 'object' && Array.isArray(value)) {
            const type = Buffer.from([BinarySerializer.Types.Array])
            const length = Buffer.alloc(4)
            length.writeInt32LE(value.length, 0)
            let buf = Buffer.alloc(0)
            for (let i = 0; i < length; i++) {
                buf = Buffer.concat([buf, this._serialize(value[i])])
            }
            return Buffer.concat([type, length, buf])
        } else if (typeof value === 'object') {
            const type = Buffer.from([BinarySerializer.Types.Object])
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
        }
    }

    _deserialize (buffer) {
        switch (buffer[0]) {
            case BinarySerializer.Types.Null:
                return [null, 1]

            case BinarySerializer.Types.Integer8:
                return [buffer.readInt8(1), 2]

            case BinarySerializer.Types.Integer16:
                return [buffer.readInt16LE(1), 3]

            case BinarySerializer.Types.Integer32:
                return [buffer.readInt32LE(1), 5]

            case BinarySerializer.Types.Integer64:
                return [buffer.readInt32LE(1) | ((buffer.readInt32LE(5) << 16) << 16), 9]
            
            case BinarySerializer.Types.Float:
                return [buffer.readDoubleLE(1), 9]

            case BinarySerializer.Types.BooleanTrue:
                return [true, 1]

            case BinarySerializer.Types.BooleanFalse:
                return [false, 1]
          
            case BinarySerializer.Types.String: {
                const length = buffer.readInt32LE(1)
                const subbuf = buffer.slice(5, 5 + length)
                return [subbuf.toString(), 5 + length]
            }

            case BinarySerializer.Types.Array: {
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

            case BinarySerializer.Types.Object: {
                const length = buffer.readInt32LE(1)
                const object = {}
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
    }
}

module.exports = {
    JsonSerializer,
    BinarySerializer
}