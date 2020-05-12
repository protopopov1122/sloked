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

export class DocumentSetClient {
    constructor(pipe: Pipe) {
        this._client = new DefaultServiceClient(pipe)
    }

    open(path: string, encoding: string, newline: string, tagger: string): Promise<number> {
        return this._client.call('open', {
            path,
            encoding,
            newline,
            tagger
        })
    }

    saveAs(path: string): Promise<boolean> {
        return this._client.call('saveAs', path)
    }

    getId(): Promise<number> {
        return this._client.call('getId', null)
    }

    async close(): Promise<void> {
        return this._client.close()
    }

    private _client: ServiceClient
}