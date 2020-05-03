import * as child_process from 'child_process'
import * as Configuration from './config.json'

const bootstrapCmd = process.argv[2]
const headlessApp = process.argv[3]

console.log(bootstrapCmd, headlessApp)
const editor = child_process.spawn(bootstrapCmd, ["--load-application", headlessApp])
editor.stdin.write(JSON.stringify(Configuration))
editor.stdin.end()

editor.stdout.on('readable', () => {
    console.log(editor.stdout.read().toString())
})
editor.stderr.on('readable', () => {
    console.log(editor.stderr.read().toString())
})

editor.on('error', err => console.log('Application error: ', err))

editor.on('exit', () => {
    console.log('Application exiting')
    process.exit(0)
})
