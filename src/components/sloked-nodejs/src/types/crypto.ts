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

import { Duplex } from 'stream'

export interface Crypto {
    deriveKey(password: string, salt: string): Promise<Buffer>;
    encrypt(data: Buffer, key: Buffer, iv: Buffer): Promise<Buffer>;
    decrypt(data: Buffer, key: Buffer, iv: Buffer): Promise<Buffer>;
    blockSize(): number;
    IVSize(): number;
    RandomBytes(count: number): Buffer;
}

export interface StreamEncryption {
    setEncryptionKey(key?: Buffer, id?: string): void
    getEncryptionKey(): Buffer | undefined
    onKeyChange(listener: ((id?: string) => Promise<void>) | ((id?: string) => void)): () => void
}

export abstract class EncryptedDuplexStream extends Duplex {
    constructor(...args: any[]) {
        super(...args)
    }

    abstract getEncryption(): StreamEncryption
}
