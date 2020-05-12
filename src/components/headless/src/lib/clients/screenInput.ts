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

import { Pipe } from 'sloked-nodejs'
import { EventEmitter } from 'events'

export enum ControlKeyCode {
    ArrowUp = 0,
    ArrowDown,
    ArrowLeft,
    ArrowRight,
    Backspace,
    Delete,
    Insert,
    Escape,
    PageUp,
    PageDown,
    Home,
    End,
    Enter,
    Tab,
    F1,
    F2,
    F3,
    F4,
    F5,
    F6,
    F7,
    F8,
    F9,
    F10,
    F11,
    F12,
    CtrlSpace,
    CtrlA,
    CtrlB,
    CtrlD,
    CtrlE,
    CtrlF,
    CtrlG,
    CtrlH,
    CtrlK,
    CtrlL,
    CtrlN,
    CtrlO,
    CtrlP,
    CtrlR,
    CtrlT,
    CtrlU,
    CtrlV,
    CtrlW,
    CtrlX,
    CtrlY
}

export interface ControlKey {
    readonly code: ControlKeyCode
    readonly alt: boolean
}

export interface KeyInputEvent {
    readonly key?: ControlKeyCode
    readonly text?: string
    readonly alt: boolean
}

export class ScreenInputClient extends EventEmitter {
    constructor(pipe: Pipe) {
        super()
        this._pipe = pipe
    }

    async listen(path: string, keys: ControlKey[], text: boolean, forward: boolean): Promise<void> {
        await this._pipe.write({
            path,
            keys,
            text,
            forward
        })
        await this._pipe.read()
        this._setupListeners()
    }

    async listenAll(path: string, forward: boolean): Promise<void> {
        await this._pipe.write({
            path,
            all: true,
            forward
        })
        await this._pipe.read()
        this._setupListeners()
    }

    async close(): Promise<void> {
        this._pipe.listen(() => {})
        return this._pipe.close()
    }

    private _setupListeners(): void {
        this._pipe.listen(async () => {
            while (this._pipe.available() > 0) {
                const value = await this._pipe.read()
                console.log(value)
                this.emit('input', value)
            }
        })
    }

    private _pipe: Pipe
}