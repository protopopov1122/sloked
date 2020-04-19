import NetInterface from '../modules/net-interface'
import DefaultPipe from '../modules/pipe'
import { Pipe } from '../types/pipe'
import { Duplex } from 'stream'
import { Serializer } from '../types/serialize'

export default class SlaveServer {
    constructor (socket: Duplex, serializer: Serializer) {
        this._net = new NetInterface(socket, serializer)
        this._net.bindMethod('ping', this._ping.bind(this))
        this._net.bindMethod('send', this._send.bind(this))
        this._net.bindMethod('close', this._close.bind(this))
        this._pipes = {}
    }

    async connect (service: string) {
        const pipeId = await this._net.invoke('connect', service)()
        const [pipe1, pipe2] = DefaultPipe.make()
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

    connector (service: string) {
        return this.connect.bind(this, service)
    }

    close () {
        this._net.close()
        for (const pipe of Object.values(this._pipes)) {
            pipe.close()
        }
        this._pipes = {}
    }

    async _ping () {
        return 'pong'
    }

    async _send (params: any) {
        const id = params.pipe
        const data = params.data
        if (this._pipes[id]) {
            this._pipes[id].write(data)
            return true
        } else {
            return false
        }
    }

    async _close (params: any) {
        const id = params
        if (this._pipes[id]) {
            this._pipes[id].close()
            delete this._pipes[id]
        }
    }

    private _net: NetInterface
    private _pipes: {[id: number]: Pipe}
}

module.exports = SlaveServer