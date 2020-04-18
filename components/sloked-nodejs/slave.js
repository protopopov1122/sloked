const NetInterface = require('./net-interface')
const Pipe = require('./pipe')

class SlaveServer {
    constructor (socket, serializer) {
        this._net = new NetInterface(socket, serializer)
        this._net.bindMethod('ping', this._ping.bind(this))
        this._net.bindMethod('send', this._send.bind(this))
        this._net.bindMethod('close', this._close.bind(this))
        this._pipes = {}
    }

    async connect (service) {
        const pipeId = await this._net.invoke('connect', service)()
        const [pipe1, pipe2] = Pipe.make()
        pipe1.listen(async () => {
            while (!pipe1.empty()) {
                this._net.invoke('send', {
                    'pipe': pipeId,
                    'data': await pipe1.read()
                })
            }
            if (!pipe1.isOpen()) {
                this._net.invoke('close', pipeId)
                delete this._pipes[pipeId]
            }
        })
        this._pipes[pipeId] = pipe1
        this._net.invoke('activate', pipeId)
        return pipe2
    }

    connector (service) {
        return this.connect.bind(this, service)
    }

    close () {
        this._net.close()
        for (const pipe of Object.values(this._pipes)) {
            pipe.close()
        }
        this._pipes = null
    }

    async _ping () {
        return 'pong'
    }

    async _send (params) {
        const id = params.pipe
        const data = params.data
        if (this._pipes[id]) {
            this._pipes[id].write(data)
            return true
        } else {
            return false
        }
    }

    async _close (params) {
        const id = params
        if (this._pipes[id]) {
            this._pipes[id].close()
            delete this._pipes[id]
        }
    }
}

module.exports = SlaveServer