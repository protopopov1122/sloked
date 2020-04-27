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
