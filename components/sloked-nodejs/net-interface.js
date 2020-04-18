class NetInterface {
    static DefaultConfig = {
        ResponseTimeout: 400
    }

    constructor(socket, serializer, config = NetInterface.DefaultConfig) {
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
        this._socket.on('data', this._receiveData.bind(this))
        this._socket.on('end', this._receiveEnd.bind(this))
    }

    bindMethod(method, callback) {
        this._methods[method] = callback
    }

    invoke(method, params) {
        const id = this._nextId++
        this._awaiting[id] = {
            callbacks: [],
            queue: [],
            timeout: null
        }
        const cleanup = () => {
            if (this._awaiting[id]) {
                delete this._awaiting[id]
            }
        }
        this._awaiting[id].timeout = setTimeout(cleanup, this._config.ResponseTimeout)
        this._write({
            'action': 'invoke',
            method,
            id,
            params
        })
        return () => new Promise((resolve, reject) => {
            if (this._awaiting[id]) {
                if (this._awaiting[id].queue.length > 0) {
                    const {result, error} = this._awaiting[id].queue[0]
                    this._awaiting[id].queue.splice(0, 1)
                    if (typeof result !== 'undefined') {
                        resolve(result)
                    } else {
                        reject(error)
                    }
                } else {
                    this._awaiting[id].callbacks.push((result, err) => {
                        if (typeof result !== 'undefined') {
                            resolve(result)
                        } else {
                            reject(err)
                        }
                    })
                }
            }
        })
    }

    close () {
        this._write({
            "action": "close"
        })
        this._socket.destroy()
        for (const awaiting of Object.values(this._awaiting)) {
            clearTimeout(awaiting.timeout)
        }
        this._awaiting = null
        this._buffer = null
        this._serializer = null
        this._socket = null
        this._methods = null
    }

    _receiveData(rawData) {
        let data = rawData
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

    _processQueue(queue) {
        for (const message of queue) {
            console.log(`IN: ${JSON.stringify(message)}`)
            const action = message.action
            if (this._actions[action]) {
                this._actions[action](message)
            }
        }
    }

    _actionInvoke(msg) {
        const id = msg['id']
        const method = msg['method']
        const params = msg['params']
        if (this._methods[method]) {
            this._handleInvokeResult(id, this._methods[method](params))
        } else {
            this._handleInvokeResult(id, this._defaultMethod(method, params))
        }
    }

    async _defaultMethod(method) {
        throw `Unknown method '${method}'`
    }

    _handleInvokeResult(id, deferred) {
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

    _actionResponse(msg) {
        const id = msg['id']
        const cleanup = () => {
            if (this._awaiting[id]) {
                delete this._awaiting[id]
            }
        }
        if (this._awaiting[id]) {
            clearTimeout(this._awaiting[id].timeout)
            this._awaiting[id].timeout = setTimeout(cleanup, this._config.ResponseTimeout)
            if (this._awaiting[id].callbacks.length > 0) {
                const callback = this._awaiting[id].callbacks[0]
                this._awaiting[id].callbacks.splice(0, 1)
                callback(msg.result, msg.error)
            } else {
                this._awaiting[id].queue.push(msg)
            }
        }
    }

    _actionClose() {
        this._socket.destroy()
        for (const awaiting of Object.values(this._awaiting)) {
            clearTimeout(awaiting.timeout)
        }
        this._awaiting = null
        this._buffer = null
        this._serializer = null
        this._socket = null
        this._methods = null
    }

    _write(msg) {
        console.log(`OUT: ${JSON.stringify(msg)}`)
        const content = this._serializer.serialize(msg)
        const length = Buffer.alloc(4)
        length.writeInt32LE(content.length)
        this._socket.write(Buffer.concat([length, content]))
    }
}

module.exports = NetInterface