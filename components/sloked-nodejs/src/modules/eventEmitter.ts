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

export class EventEmitter<T> {
    subscribe(listener: (event: T) => void): () => void {
        const id: number = this._nextId++
        this._subscribers[id] = listener
        return () => {
            if (this._subscribers[id]) {
                delete this._subscribers[id]
            }
        }
    }

    asyncSubscribe(listener: (event: T) => Promise<void>): () => void {
        const id: number = this._nextId++
        this._asyncSubscribers[id] = listener
        return () => {
            if (this._asyncSubscribers[id]) {
                delete this._asyncSubscribers[id]
            }
        }
    }

    async emit(event: T): Promise<void> {
        Object.values(this._subscribers).forEach(listener => listener(event))
        for (let subs of Object.values(this._asyncSubscribers)) {
            await subs(event)
        }
    }

    private _nextId: number = 0
    private _subscribers: { [id: number]: ((event: T) => void) } = {}
    private _asyncSubscribers: { [id: number]: ((event: T) => Promise<void>) } = {}
}
