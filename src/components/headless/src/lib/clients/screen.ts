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

import { Pipe, DefaultServiceClient, ServiceClient } from 'sloked-nodejs'
import { EventEmitter } from 'events'

export enum ScreenSplitterDirection {
    Vertical = 0,
    Horizontal
}

export class ScreenHandleClient {
    constructor(client: ServiceClient) {
        this._client = client
    }

    newMultiplexer(path: string): Promise<boolean> {
        return this._client.call('handle.newMultiplexer', path)
    }

    newSplitter(path: string, direction: ScreenSplitterDirection): Promise<boolean> {
        return this._client.call('handle.newSplitter', {
            path,
            direction
        })
    }

    private _client: ServiceClient
}

export class ScreenMultiplexerClient {
    constructor(client: ServiceClient) {
        this._client = client
    }

    newWindow(path: string, position: {x: number, y: number}, size: {width: number, height: number}): Promise<string> {
        return this._client.call('multiplexer.newWindow', {
            path,
            'pos': {
                'line': position.y,
                'column': position.x
            },
            'size': {
                'line': size.height,
                'column': size.width
            }
        })
    }

    resizeWidth(path: string, size: {width: number, height: number}): Promise<boolean> {
        return this._client.call('multiplexer.resizeWindow', {
            path,
            'size': {
                'line': size.height,
                'column': size.width
            }
        })
    }

    private _client: ServiceClient
}
export class ScreenSplitterClient {
    constructor(client: ServiceClient) {
        this._client = client
    }

    newWindow(path: string, constraints: {dim: number, min?: number, max?: number}): Promise<string> {
        return this._client.call('splitter.newWindow', {
            path,
            constraints: {
                dim: constraints.dim,
                min: constraints.min || 0,
                max: constraints.max || 0
            }
        })
    }

    private _client: ServiceClient
}

export class ScreenClient {
    constructor(pipe: Pipe) {
        this._client = new DefaultServiceClient(pipe)
        this.Handle = new ScreenHandleClient(this._client)
        this.Multiplexer = new ScreenMultiplexerClient(this._client)
        this.Splitter = new ScreenSplitterClient(this._client)
    }

    close(): Promise<void> {
        return this._client.close()
    }

    public readonly Handle: ScreenHandleClient
    public readonly Multiplexer: ScreenMultiplexerClient
    public readonly Splitter: ScreenSplitterClient
    private _client: DefaultServiceClient
}

export class ScreenSizeClient extends EventEmitter {
    async connect(pipe: Pipe): Promise<void> {
        if (this._pipe) {
            this._pipe.listen(() => {})
            await this._pipe.close()
        }
        this._pipe = pipe
        this._pipe.listen(() => {
            while (this._pipe && this._pipe.available() > 0) {
                this._readSize()
            }
        })
        await this._pipe.write(null)
        await this._readSize()
    }

    size(): {width: number, height: number} {
        if (this._size) {
            return {...this._size}
        } else {
            throw new Error('Unknown size')
        }
    }

    private async _readSize(): Promise<void> {
        if (this._pipe) {
            this._size = await this._pipe.read()
            this.emit('resize', this._size)
        }
    }

    private _pipe?: Pipe
    private _size?: {width: number, height: number}
}