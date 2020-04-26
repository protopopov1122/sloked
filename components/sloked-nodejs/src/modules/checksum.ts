

export class Crc32 {
    private static LookupTable: Uint32Array = (function(): Uint32Array {
        const table = new Uint32Array(256)
        const reversedPolynomial: number = 0xEDB88320
        for (let n = 0; n < table.length; n++) {
            let checksum: number = n
            for (let i = 0; i < 8; i++) {
                checksum =
                    ((checksum >>> 1) ^
                    (((checksum & 0x1) != 0) ? reversedPolynomial : 0)) >>> 0
            }
            table[n] = checksum
        }
        return table
    })()

    static Calculate(chunk: Buffer): number {
        let checksum: number = 0xffffffff
        chunk.forEach((byte: number) => {
            const idx: number = ((checksum ^ byte) & 0xff) >>> 0
            checksum = (this.LookupTable[idx] ^ (checksum >>> 8)) >>> 0
        })
        return (~checksum & 0xffffffff) >>> 0
    }
}