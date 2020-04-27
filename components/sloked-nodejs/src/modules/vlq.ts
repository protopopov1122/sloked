export function Encode(value: number): Buffer {
    const Mask: number = 0b10000000
    const result: number[] = []
    do {
        let byte = (value >>> 0) & (~Mask)
        value >>= 7
        if (value == 0) {
            byte |= Mask
        }
        result.push(byte)
    } while (value != 0)
    return Buffer.from(result)
}

export function Decode(input: Buffer): [number, Buffer] | null {
    const Mask: number = 0b10000000
    let result: number = 0
    let finished: boolean = false
    let offset: number = 0
    while (!finished && input.length > 0) {
        const byte: number = input[0]
        result |= ((byte & (~Mask)) << offset) >>> 0
        offset += 7
        finished = (byte & Mask) != 0
        input = input.slice(1)
    }
    if (finished) {
        return [result, input]
    } else {
        return null
    }
}