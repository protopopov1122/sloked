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

import { SlokedStandaloneApplication } from './lib/standaloneApplication'
import { connectToEditor } from './sample/init'
import Configuration from './config.json'

const bootstrap = process.argv[2]
const applicationLibrary = process.argv[3]

function bootstrapEditor(): SlokedStandaloneApplication {
    const editor: SlokedStandaloneApplication = new SlokedStandaloneApplication({
        bootstrap,
        applicationLibrary
    })

    editor.on('error', err => console.log('Application error: ', err))

    editor.on('exit', () => {
        console.log('Application exiting')
        process.exit(0)
    })

    process.on('SIGINT', function () {
        editor.terminate()
    })
    return editor
}



bootstrapEditor().once('ready', async () => {
    await connectToEditor(Configuration.containers.main.server.netServer.host, Configuration.containers.main.server.netServer.port, process.argv.slice(4))
}).start(Configuration)
