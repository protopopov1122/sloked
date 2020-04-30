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

import { Server, Service } from '../types/server'
import { Pipe } from '../types/pipe'
import { DefaultPipe } from './pipe'

export class LocalServer implements Server<string> {
    constructor() {
        this._services = {}
    }

    async connect(serviceName: string): Promise<Pipe> {
        if (this._services[serviceName]) {
            const [pipe1, pipe2]: [Pipe, Pipe] = DefaultPipe.make()
            const success = await this._services[serviceName].attach(pipe1)
            if (success) {
                return pipe2
            }
        }
        throw new Error(`Attaching pipe to ${serviceName} failed`)
    }

    connector(serviceName: string): () => Promise<Pipe> {
        return () => this.connect(serviceName)
    }

    async register(serviceName: string, service: Service): Promise<void> {
        if (typeof this._services[serviceName] === 'undefined') {
            this._services[serviceName] = service
        } else {
            throw new Error(`Service ${serviceName} is already registered`)
        }
    }

    async registered(serviceName: string): Promise<boolean> {
        return typeof this._services[serviceName] !== 'undefined'
    }

    async deregister(serviceName: string): Promise<void> {
        if (typeof this._services[serviceName] !== 'undefined') {
            delete this._services[serviceName]
        } else {
            throw new Error(`Service ${serviceName} is not registered`)
        }
    }

    private _services: { [id: string]: Service }
}
