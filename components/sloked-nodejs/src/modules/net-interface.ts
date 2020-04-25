import { Serializer } from '../types/serialize'
import { Duplex } from 'stream'

type AwaitingResult = {
    result: any,
    error: any
}

type AwaitingCallback = {
    resolve: (result: any) => void,
    reject: (error: any) => void
}

class Awaiting {
    next() {
        return new Promise<any>((resolve, reject) => {
            if (this._queue.length > 0) {
                const value = this._queue[0];
                this._queue.slice(0, 1)
                if (typeof value.result !== 'undefined') {
                    resolve(value.result)
                } else {
                    reject(value.error)
                }
            } else {
                this._callbacks.push({
                    resolve,
                    reject
                })
            }
        })
    }

    push(value: AwaitingResult) {
        if (this._callbacks.length > 0) {
            const callback = this._callbacks[0]
            this._callbacks.splice(0, 1)
            if (typeof value.result !== 'undefined') {
                callback.resolve(value.result)
            } else {
                callback.reject(value.error)
            }
        } else {
            this._queue.push(value)
        }
    }

    notify(fn: () => void, timeout: number) {
        if (this._timeout !== null) {
            clearTimeout(this._timeout)
        }
        setTimeout(fn, timeout)
    }

    reset() {
        if (this._timeout !== null) {
            clearTimeout(this._timeout)
        }
        this._queue = []
        this._callbacks = []
    }

    private _callbacks: AwaitingCallback[] = []
    private _queue: AwaitingResult[] = []
    private _timeout?: number
}

export default class NetInterface {
    static DefaultConfig = {
        ResponseTimeout: 400
    }

    constructor(socket: Duplex, serializer: Serializer, config = NetInterface.DefaultConfig) {
        this._actions = {
            'invoke': this._actionInvoke.bind(this),
            'response': this._actionResponse.bind(this),
            'close': this._actionClose.bind(this)
        }
        this._config = config
        this._methods = {}
        this._buffer = Buffer.alloc(0)
        this._serializer = serializer
        this._nextId = 0
        this._awaiting = {}
        this._socket = socket
        this._socket.on('readable', this._receiveData.bind(this))
        this._socket.on('end', this._receiveEnd.bind(this))
    }

    bindMethod(method: string, callback: (params: any) => Promise<any>) {
        this._methods[method] = callback
    }

    invoke(method: string, params: any) {
        const id = this._nextId++
        this._awaiting[id] = new Awaiting()
        const cleanup = () => {
            if (this._awaiting[id]) {
                delete this._awaiting[id]
            }
        }
        this._awaiting[id].notify(cleanup, this._config.ResponseTimeout)
        this._write({
            'action': 'invoke',
            method,
            id,
            params
        })
        return async () => {
            if (this._awaiting[id]) {
                return this._awaiting[id].next()
            } else {
                throw new Error('Not invoked')
            }
        }
    }

    close() {
        this._write({
            "action": "close"
        })
        this._socket.destroy()
        Object.values(this._awaiting).forEach(el => el.reset());
        this._awaiting = []
        this._buffer = Buffer.alloc(0)
        this._methods = {}
    }

    _receiveData() {
        let data = this._socket.read()
        if (data === null) {
            return // End of stream reached
        }
        if (typeof data === 'string') {
            data = Buffer.from(data, 'utf8')
        }
        this._buffer = Buffer.concat([this._buffer, data])
        this._processBuffer()
    }

    _receiveEnd() {
        console.log('END')
    }

    _processBuffer() {
        const queue = []
        while (this._buffer.length >= 4) {
            const length = this._buffer.readInt32LE(0)
            if (this._buffer.length < length + 4) {
                break
            }
            const msg = this._buffer.slice(4, length + 4);
            const content = this._serializer.deserialize(msg)
            queue.push(content)
            this._buffer = this._buffer.slice(length + 4)
        }
        this._processQueue(queue)
    }

    _processQueue(queue: any[]) {
        for (const message of queue) {
            console.log('IN:', message)
            const action = message.action
            if (this._actions[action]) {
                this._actions[action](message)
            }
        }
    }

    _actionInvoke(msg: any) {
        const id = msg['id']
        const method = msg['method']
        const params = msg['params']
        if (this._methods[method]) {
            this._handleInvokeResult(id, this._methods[method](params))
        } else {
            this._handleInvokeResult(id, this._defaultMethod(method, params))
        }
    }

    async _defaultMethod(method: string, _: any) {
        throw `Unknown method '${method}'`
    }

    _handleInvokeResult(id: number, deferred: Promise<any>) {
        deferred.then(result => {
            this._write({
                'action': 'response',
                'id': id,
                'result': result
            })
        }, err => {
            this._write({
                'action': 'response',
                'id': id,
                'err': err
            })
        })
    }

    _actionResponse(msg: any) {
        const id = msg['id']
        const cleanup = () => {
            if (this._awaiting[id]) {
                delete this._awaiting[id]
            }
        }
        if (this._awaiting[id]) {
            this._awaiting[id].push(msg)
            this._awaiting[id].notify(cleanup, this._config.ResponseTimeout)
        }
    }

    _actionClose() {
        this._socket.destroy()
        Object.values(this._awaiting).forEach(el => el.reset());
        this._awaiting = []
        this._buffer = Buffer.alloc(0)
        this._methods = {}
    }

    _write(msg: any) {
        console.log('OUT', msg)
        const content = this._serializer.serialize(msg)
        const length = Buffer.alloc(4)
        length.writeInt32LE(content.length)
        this._socket.write(Buffer.concat([length, content]))
    }

    private _actions: { [action: string]: (msg: any) => void };
    private _config: any;
    private _methods: { [method: string]: (params: any) => Promise<any> };
    private _buffer: Buffer;
    private _serializer: Serializer;
    private _nextId: number;
    private _awaiting: { [id: number]: Awaiting };
    private _socket: Duplex;
}
