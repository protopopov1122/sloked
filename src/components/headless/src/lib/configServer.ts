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

import * as net from 'net'

export class ConfigurationServer {
    constructor() {
        this._server = new net.Server()
        this._configs = {}
        this._server.on('connection', (socket: net.Socket) => {
            let key: string = ''
            socket.on('data', (data: Buffer) => {
                key += data.toString()
                if (!key.endsWith('\n')) {
                    return
                }
                key = key.trim()
                if (this._configs[key]) {
                    socket.write(this._configs[key]())
                }
                socket.destroy()
            })
        })
    }

    start(host: string, port: number): Promise<void> {
        return new Promise<void>((resolve, reject) => {
            this._server.once('listening', resolve).once('error', reject).listen({
                port,
                host
            })
        })
    }

    attach(key: string, callback: () => string): void {
        this._configs[key] = callback
    }

    detach(key: string): void {
        delete this._configs[key]
    }

    close(): Promise<void> {
        return new Promise<void>((resolve, reject) => this._server.once('close', resolve).once('error', reject).close())
    }

    private _server: net.Server
    private _configs: {[key: string]: () => string}
}