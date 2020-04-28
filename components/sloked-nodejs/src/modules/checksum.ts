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
