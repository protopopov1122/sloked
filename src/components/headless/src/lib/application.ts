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

import * as child_process from 'child_process'
import { EventEmitter } from 'events'

interface ApplicationOptions {
    bootstrap: string,
    applicationLibrary: string
}

export class SlokedApplication extends EventEmitter {
    constructor(options: ApplicationOptions) {
        super()
        this._options = options
        this._process = null
    }

    start(configPath: string): void {
        if (this._process !== null) {
            throw new Error('Application already started')
        }
        this._process = child_process.spawn(this._options.bootstrap, ["--load-application", this._options.applicationLibrary, "--load-configuration", configPath], {
            stdio: 'inherit'
        })
        this._process.on('exit', this._onExit.bind(this))
        this._process.on('error', this._onError.bind(this))
        setTimeout(() => this.emit('ready'), 500) // TODO
    }

    terminate(): void {
        if (this._process) {
            this._process.kill('SIGTERM')
            this._process = null
            this.emit('terminate')
        }
    }

    _onExit(_: any): void {
        this._process = null
        this.emit('exit')
    }

    _onError(err: any): void {
        this.emit('error', err)
    }

    private _options: ApplicationOptions
    private _process: child_process.ChildProcess | null
}