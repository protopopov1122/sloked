class PipeDescriptor {
    constructor() {
        this._open = true
    }

    isOpen(): boolean {
        return this._open
    }

    close() {
        this._open = false
    }

    private _open: boolean;
}

class SimplexPipe {
    constructor(descriptor: PipeDescriptor) {
        this._descriptor = descriptor
        this._queue = []
        this._awaiting = []
        this._callback = null
    }

    isOpen(): boolean {
        return this._descriptor.isOpen()
    }

    listen(callback: () => void) {
        this._callback = callback
    }

    notify() {
        if (this._callback) {
            this._callback()
        }
    }

    close() {
        this._descriptor.close()
        this._awaiting = []
        this._callback = null
    }

    push(msg: any) {
        if (!this._descriptor.isOpen()) {
            throw new Error('Attempt to push inside closed pipe')
        }
        if (this._awaiting.length > 0) {
            const callback = this._awaiting[0]
            this._awaiting.splice(0, 1)
            callback(msg)
        } else {
            this._queue.push(msg)
            this.notify()
        }
    }

    pop(): Promise<any> {
        return new Promise((resolve, reject) => {
            if (this._queue.length > 0) {
                const msg = this._queue[0]
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

    private _descriptor: PipeDescriptor;
    private _queue: any[];
    private _awaiting: ((result: any) => void)[];
    private _callback: (() => void) | null;
}

export default class Pipe {
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

    read(): any {
        return this._in.pop()
    }

    write(msg: any) {
        this._out.push(msg)
    }

    listen(callback: () => void) {
        this._in.listen(callback)
    }

    close() {
        this._in.close()
        this._out.notify()
    }

    static make(): [Pipe, Pipe] {
        const descriptor = new PipeDescriptor()
        const input = new SimplexPipe(descriptor)
        const output = new SimplexPipe(descriptor)
        return [new Pipe(input, output), new Pipe(output, input)]
    }

    private _in: SimplexPipe;
    private _out: SimplexPipe;
}
