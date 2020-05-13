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

import { Pipe, ServiceClient, DefaultServiceClient } from "sloked-nodejs";
import { KeyInputEvent, ControlKey } from "./screenInput";

export interface FontProperties {
    getHeight(): Promise<number>
    getWidth(grapheme: string): Promise<number>
}

export enum TextGraphics {
    Off = 0,
    Bold,
    Underscore,
    Blink,
    Reverse,
    Concealed
}

export enum BackgroundGraphics {
    Black = 0,
    Red,
    Green,
    Yellow,
    Blue,
    Magenta,
    Cyan,
    White
}

export enum ForegroundGraphics {
    Black = 0,
    Red,
    Green,
    Yellow,
    Blue,
    Magenta,
    Cyan,
    White
}

export interface TextPaneRenderer {
    setPosition(x: number, y: number): void
    moveUp(y: number): void
    moveDown(y: number): void
    moveBackward(x: number): void
    moveForward(x: number): void

    showCursor(show: boolean): void
    clearScreen(): void
    clearArea(x: number, y: number): void
    getMaxWidth(): Promise<number>
    getHeight(): Promise<number>
    getFontProperties(): FontProperties

    write(content: string): void

    setTextMode(mode: TextGraphics): void
    setBackgroundMode(mode: BackgroundGraphics): void
    setForegroundMode(mode: ForegroundGraphics): void

    reset(): void
    flush(): Promise<void>
}

enum RenderCommand {
    SetPosition,
    MoveUp,
    MoveDown,
    MoveBackward,
    MoveForward,
    ShowCursor,
    ClearScreen,
    ClearChars,
    Write,
    SetTextGraphics,
    SetForegroundGraphics,
    SetBackgroundGraphics
};

class TextPaneFontProperties implements FontProperties {
    constructor(client: ServiceClient) {
        this._client = client
    }

    getHeight(): Promise<number> {
        return this._client.call('getCharHeight', null)
    }

    getWidth(grapheme: string): Promise<number> {
        const utf16Codepoints: string[] = Array.from(grapheme)
        const codepoints = utf16Codepoints.map(codepoint => {
            if (codepoint.length) {
                return codepoint
            } else if (codepoint.length === 2) { // Surrogate pair
                const highSurrogate = codepoint.codePointAt(0)
                const lowSurrogate = codepoint.codePointAt(0)
                if (typeof highSurrogate === 'undefined' ||
                    typeof lowSurrogate === 'undefined') {
                    // Unexpected
                    throw new Error()
                }
                const high = ((highSurrogate - 0xD800) * 0x400) >>> 0
                const low = (lowSurrogate - 0xD800) >>> 0
                return high + low
            } else {
                // Unexpected
                throw new Error()
            }
        })
        return this._client.call('getCharWidth', codepoints)
    }

    private _client: ServiceClient
}

class TextPaneRendererClient implements TextPaneRenderer {
    constructor(client: ServiceClient) {
        this._client = client
        this._fontProperties = new TextPaneFontProperties(this._client)
        this._buffer = []
    }

    setPosition(x: number, y: number): void {
        this._buffer.push({
            command: RenderCommand.SetPosition,
            line: y,
            column: x
        })
    }

    moveUp(y: number): void {
        this._buffer.push({
            command: RenderCommand.MoveUp,
            line: y,
        })
    }
    
    moveDown(y: number): void {
        this._buffer.push({
            command: RenderCommand.MoveDown,
            line: y,
        })
    }

    moveBackward(x: number): void {
        this._buffer.push({
            command: RenderCommand.MoveBackward,
            column: x,
        })
    }

    moveForward(x: number): void {
        this._buffer.push({
            command: RenderCommand.MoveForward,
            column: x,
        })
    }

    showCursor(show: boolean): void {
        this._buffer.push({
            command: RenderCommand.ShowCursor,
            show
        })
    }

    clearScreen(): void {
        this._buffer.push({
            command: RenderCommand.ClearScreen
        })
    }

    clearArea(x: number, y: number): void {
        this._buffer.push({
            command: RenderCommand.ClearChars,
            line: y,
            column: x
        })
    }

    getMaxWidth(): Promise<number> {
        return this._client.call('getMaxWidth', null)
    }

    getHeight(): Promise<number> {
        return this._client.call('getHeight', null)
    }

    getFontProperties(): FontProperties {
        return this._fontProperties
    }

    write(content: string): void {
        this._buffer.push({
            command: RenderCommand.Write,
            text: content
        })
    }

    setTextMode(mode: TextGraphics): void {
        this._buffer.push({
            command: RenderCommand.SetTextGraphics,
            mode
        })
    }

    setBackgroundMode(mode: BackgroundGraphics): void {
        this._buffer.push({
            command: RenderCommand.SetBackgroundGraphics,
            mode
        })
    }

    setForegroundMode(mode: ForegroundGraphics): void {
        this._buffer.push({
            command: RenderCommand.SetForegroundGraphics,
            mode
        })
    }

    reset(): void {
        this._buffer = []
    }

    async flush(): Promise<void> {
        await this._client.send('render', this._buffer)
        this._buffer = []
    }

    private _client: ServiceClient
    private _fontProperties: FontProperties
    private _buffer: any[]
}

export class TextPaneClient {
    constructor(pipe: Pipe) {
        this._client = new DefaultServiceClient(pipe)
        this._rederer = new TextPaneRendererClient(this._client)
    }

    async connect(path: string, keys: ControlKey[], text: boolean): Promise<boolean> {
        return this._client.call('connect', {
            path,
            keys,
            text
        })
    }

    getRenderer(): TextPaneRenderer {
        return this._rederer
    }

    async getInput(): Promise<KeyInputEvent[]> {
        return this._client.call('getInput', {})
    }

    close(): Promise<void> {
        return this._client.close()
    }

    private _client: ServiceClient
    private _rederer: TextPaneRenderer
}