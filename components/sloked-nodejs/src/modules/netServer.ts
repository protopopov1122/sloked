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

import { NetInterface } from './net-interface'
import { DefaultPipe } from './pipe'
import { Authenticator } from '../types/authenticator'
import { Pipe } from '../types/pipe'
import { Duplex } from 'stream'
import { Serializer } from '../types/serialize'
import { AuthServer, Service } from '../types/server'
import { LocalServer } from './localServer'

interface AuthRequestRsp {
    nonce: number
}

interface SendParams {
    pipe: number,
    data: any
}

export class NetSlaveServer implements AuthServer<string> {
    constructor(socket: Duplex, serializer: Serializer, authenticator?: Authenticator) {
        this._net = new NetInterface(socket, serializer)
        this._pipes = {}
        this._localServer = new LocalServer()
        this._authenticator = authenticator
        this._net.bindMethod('ping', this._ping.bind(this))
        this._net.bindMethod('send', this._send.bind(this))
        this._net.bindMethod('connect', this._connect.bind(this))
        this._net.bindMethod('close', this._close.bind(this))
    }

    async connect(service: string): Promise<Pipe> {
        const pipeId: number = await this._net.invoke('connect', service).next().value
        const [pipe1, pipe2]: [Pipe, Pipe] = DefaultPipe.make()
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
        while (!pipe1.empty()) {
            this._net.invoke('send', {
                'pipe': pipeId,
                'data': await pipe1.read()
            })
        }
        if (!pipe1.isOpen()) {
            this._net.invoke('close', pipeId)
            delete this._pipes[pipeId]
        } else {
            this._net.invoke('activate', pipeId)
        }
        return pipe2
    }

    connector(service: string): () => Promise<Pipe> {
        return this.connect.bind(this, service)
    }

    async register(serviceName: string, service: Service): Promise<void> {
        await this._localServer.register(serviceName, service)
        const success: boolean = await this._net.invoke('bind', serviceName).next().value
        if (!success) {
            await this._localServer.deregister(serviceName)
            throw new Error(`Failing binding service ${serviceName}`)
        }
    }

    async registered(service: string): Promise<boolean> {
        const boundRsp: boolean = await this._net.invoke('bound', service).next().value
        return boundRsp
    }

    async deregister(serviceName: string): Promise<void> {
        await this._localServer.deregister(serviceName)
        const success: boolean = await this._net.invoke('unbind', serviceName).next().value
        if (!success) {
            throw new Error(`Error while detaching ${serviceName}`)
        }
    }

    async authorize(account: string): Promise<void> {
        if (this._authenticator) {
            const authRequest: AuthRequestRsp = await this._net.invoke('auth-request', null).next().value
            const nonce: number = authRequest.nonce
            const token: string = await this._authenticator.authenticate(account, nonce)
            const authResponse: boolean = await this._net.invoke('auth-response', {
                'id': account,
                'result': token
            }).next().value
            if (!authResponse) {
                throw new Error(`Authentication as ${account} failed`)
            }
        } else {
            throw new Error('Authenticator not defined')
        }
    }

    async logout(): Promise<void> {
        if (this._authenticator) {
            await this._net.invoke('auth-logout', null).next().value
        } else {
            throw new Error('Authenticator not defined')
        }
    }

    close(): void {
        if (this._authenticator) {
            this._authenticator.close()
        }
        this._net.close()
        for (const pipe of Object.values(this._pipes)) {
            pipe.close()
        }
        this._pipes = {}
    }

    async _ping(): Promise<string> {
        return 'pong'
    }

    async _send(params: SendParams): Promise<boolean> {
        const id: number = params.pipe
        const data: any = params.data
        if (this._pipes[id]) {
            this._pipes[id].write(data)
            return true
        } else {
            return false
        }
    }

    async _connect(params: any): Promise<number | boolean> {
        try {
            const service: string = params['service']
            const pipeId: number = params['pipe']
            const pipe = await this._localServer.connect(service)
            pipe.listen(async () => {
                while (!pipe.empty()) {
                    this._net.invoke('send', {
                        'pipe': pipeId,
                        'data': await pipe.read()
                    })
                }
                if (!pipe.isOpen()) {
                    this._net.invoke('close', pipeId)
                    delete this._pipes[pipeId]
                }
            })
            this._pipes[pipeId] = pipe
            while (!pipe.empty()) {
                this._net.invoke('send', {
                    'pipe': pipeId,
                    'data': await pipe.read()
                })
            }
            if (!pipe.isOpen()) {
                this._net.invoke('close', pipeId)
                delete this._pipes[pipeId]
            }
            return pipeId
        } catch (err) {
            return false
        }
    }

    async _close(params: number) {
        const id: number = params
        if (this._pipes[id]) {
            this._pipes[id].close()
            delete this._pipes[id]
        }
    }

    private _net: NetInterface
    private _pipes: { [id: number]: Pipe }
    private _localServer: LocalServer
    private _authenticator?: Authenticator
}
