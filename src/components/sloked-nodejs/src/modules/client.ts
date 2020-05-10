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

import { ServiceClient, ServiceResponse } from '../types/client'
import { Pipe } from '../types/pipe'

export * from '../types/client'

type ResponseCallback = {
    resolve: (result: any) => void
    reject: (error: any) => void
}

class ResponseQueue {
    constructor() {
        this._queue = []
        this._awaiting = []
    }

    next(): Promise<any> {
        return new Promise<any>((resolve, reject) => {
            if (this._queue.length > 0) {
                const value = this._queue[0]
                this._queue.splice(0, 1)
                if (value.error) {
                    reject(value.error)
                } else {
                    resolve(value.result)
                }
            } else {
                this._awaiting.push({
                    resolve,
                    reject
                })
            }
        })
    }

    push(value: any): void {
        if (this._awaiting.length > 0) {
            const callback = this._awaiting[0]
            this._awaiting.splice(0, 1)
            if (value.error) {
                callback.reject(value.error)
            } else {
                callback.resolve(value.result)
            }
        } else {
            this._queue.push(value)
        }
    }

    private _queue: any[]
    private _awaiting: ResponseCallback[]
}

class DefaultServiceResponse implements ServiceResponse {
    constructor(queue: ResponseQueue, closeCallback: () => Promise<void>) {
        this._queue = queue
        this._closeCallback = closeCallback
    }

    next(): Promise<any> {
        return this._queue.next()
    }

    async last(): Promise<any> {
        const res = await this.next()
        await this.close()
        return res
    }

    close(): Promise<void> {
        return this._closeCallback()
    }

    private _queue: ResponseQueue
    private _closeCallback: () => Promise<void>
}

export class DefaultServiceClient implements ServiceClient {
    constructor(pipe: Pipe) {
        this._pipe = pipe
        this._nextId = 0
        this._responseQueue = {}
        this._pipe.listen(async () => {
            while (this._pipe.available() > 0) {
                const rsp = await this._pipe.read()
                const queue = this._responseQueue[rsp['id']]
                if (queue) {
                    queue.push(rsp)
                }
            }
        })
    }

    async invoke(method: string, params: any): Promise<ServiceResponse> {
        const id: number = this._nextId++
        const responseQueue = new ResponseQueue()
        this._responseQueue[id] = responseQueue
        await this._pipe.write({
            id,
            method,
            params
        })
        return new DefaultServiceResponse(responseQueue, async () => {
            delete this._responseQueue[id]
        })
    }

    async call(method: string, params: any): Promise<any> {
        return this.invoke(method, params).then(res => res.last())
    }

    async send(method: string, params: any): Promise<void> {
        const id: number = this._nextId++
        await this._pipe.write({
            id,
            method,
            params
        })
    }

    async close(): Promise<void> {
        this._pipe.listen(() => {})
        await this._pipe.close()
        this._responseQueue = {}
    }

    private _pipe: Pipe
    private _nextId: number
    private _responseQueue: {[id: number]: ResponseQueue}
}