class PipeDescriptor {
    constructor () {
        this._open = true
    }

    isOpen () {
        return this._open
    }

    close () {
        this._open = false
    }
}

class SimplexPipe {
    constructor (descriptor) {
        this._descriptor = descriptor
        this._queue = []
        this._awaiting = []
        this._callback = null
    }

    isOpen () {
        return this._descriptor.isOpen()
    }

    listen (callback) {
        this._callback = callback
    }

    notify () {
        if (this._callback) {
            this._callback()
        }
    }

    close () {
        this._descriptor.close()
        this._awaiting = null
        this._callback = null
    }

    push (msg) {
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

    pop () {
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

    count () {
        return this._queue.length
    }
}

class Pipe {
    constructor (input, output) {
        this._in = input
        this._out = output
    }

    isOpen () {
        return this._in.isOpen()
    }

    available () {
        return this._in.count()
    }

    empty () {
        return this.available() === 0
    }

    read () {
        return this._in.pop()
    }

    write (msg) {
        this._out.push(msg)
    }

    listen (callback) {
        this._in.listen(callback)
    }

    close () {
        this._in.close()
        this._out.notify()
    }

    static make () {
        const descriptor = new PipeDescriptor()
        const input = new SimplexPipe(descriptor)
        const output = new SimplexPipe(descriptor)
        return [new Pipe(input, output), new Pipe(output, input)]
    }
}

module.exports = Pipe
