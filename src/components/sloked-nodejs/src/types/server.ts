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

import { Pipe } from './pipe'

export interface Service {
    attach(pipe: Pipe): Promise<boolean>
}

export interface Server<T> {
    connect(serviceName: T): Promise<Pipe>
    connector(serviceName: T): () => Promise<Pipe>
    register(serviceName: string, service: Service): Promise<void>
    registered(serviceName: T): Promise<boolean>
    deregister(serviceName: string): Promise<void>
}

export interface AuthServer<T> extends Server<T> {
    authorize(account: string): Promise<void>
    logout(): Promise<void>
}
