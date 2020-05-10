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

export interface ServiceResponse {
    next(): Promise<any>
    last(): Promise<any>
    close(): Promise<void>
}

export interface ServiceClient {
    invoke(method: string, params: any): Promise<ServiceResponse>
    call(method: string, params: any): Promise<any>
    send(method: string, params: any): Promise<void>
    close(): Promise<void>
}