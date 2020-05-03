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

import { Pipe, PipeCallback } from '../types/pipe'

class PipeDescriptor {
    constructor() {
        this._open = true
    }

    isOpen(): boolean {
        return this._open
    }

    close(): void {
        this._open = false
    }

    private _open: boolean;
}

class SimplexPipe {
    constructor(descriptor: PipeDescriptor) {
        this._descriptor = descriptor
        this._queue = []
        this._awaiting = []
        this._callback = undefined
    }

    isOpen(): boolean {
        return this._descriptor.isOpen()
    }

    listen(callback: PipeCallback): void {
        this._callback = callback
    }

    async notify(): Promise<void> {
        if (this._callback) {
            await this._callback()
        }
    }

    close(): void {
        this._descriptor.close()
        this._awaiting = []
        this._callback = undefined
    }

    async push(msg: any): Promise<void> {
        if (!this._descriptor.isOpen()) {
            throw new Error('Attempt to push inside closed pipe')
        }
        if (this._awaiting.length > 0) {
            const callback: (msg: any) => void = this._awaiting[0]
            this._awaiting.splice(0, 1)
            callback(msg)
        } else {
            this._queue.push(msg)
            await this.notify()
        }
    }

    pop(): Promise<any> {
        return new Promise((resolve, reject) => {
            if (this._queue.length > 0) {
                const msg: any = this._queue[0]
                this._queue.splice(0, 1)
                resolve(msg)
            } else {
                if (!this._descriptor.isOpen()) {
                    reject(new Error('Attempt to pop from closed pipe'))
                } else {
                    this._awaiting.push(resolve)
                }
            }
        })
    }

    count(): number {
        return this._queue.length
    }

    private _descriptor: PipeDescriptor
    private _queue: any[]
    private _awaiting: ((result: any) => void)[]
    private _callback?: PipeCallback
}

export class DefaultPipe implements Pipe {
    constructor(input: SimplexPipe, output: SimplexPipe) {
        this._in = input
        this._out = output
    }

    isOpen(): boolean {
        return this._in.isOpen()
    }

    available(): number {
        return this._in.count()
    }

    empty(): boolean {
        return this.available() === 0
    }

    read(): Promise<any> {
        return this._in.pop()
    }

    async write(msg: any): Promise<void> {
        await this._out.push(msg)
    }

    listen(callback: PipeCallback): void {
        this._in.listen(callback)
    }

    async close(): Promise<void> {
        this._in.close()
        await this._out.notify()
    }

    static make(): [Pipe, Pipe] {
        const descriptor = new PipeDescriptor()
        const input = new SimplexPipe(descriptor)
        const output = new SimplexPipe(descriptor)
        return [new DefaultPipe(input, output), new DefaultPipe(output, input)]
    }

    private _in: SimplexPipe;
    private _out: SimplexPipe;
}
