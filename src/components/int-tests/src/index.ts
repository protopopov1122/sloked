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

import { SlokedApplication } from './application'
import * as Configuration from './config.json'

const bootstrap = process.argv[2]
const applicationLibrary = process.argv[3]

const editor = new SlokedApplication({
    bootstrap,
    applicationLibrary
})
editor.start(Configuration)

editor.on('stdout', chunk => {
    console.log(chunk.toString())
})

editor.on('stderr', chunk => {
    console.error(chunk.toString())
})

editor.on('error', err => console.log('Application error: ', err))

editor.on('exit', () => {
    console.log('Application exiting')
    process.exit(0)
})

process.on('SIGINT', function() {
    editor.terminate()
});
