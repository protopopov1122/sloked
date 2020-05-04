import * as child_process from 'child_process'
import * as Configuration from './config.json'

const bootstrapCmd = process.argv[2]
const headlessApp = process.argv[3]

console.log(bootstrapCmd, headlessApp)
const editor = child_process.spawn(bootstrapCmd, ["--load-application", headlessApp])
editor.stdin.write(JSON.stringify(Configuration))
editor.stdin.end()

editor.stdout.on('readable', () => {
    let chunk;
    while (null !== (chunk = editor.stdout.read())) {
        console.log(chunk.toString())
    }
})

editor.stderr.on('readable', () => {
    let chunk;
    while (null !== (chunk = editor.stderr.read())) {
        console.error(chunk.toString())
    }
})

editor.on('error', err => console.log('Application error: ', err))

editor.on('exit', () => {
    console.log('Application exiting')
    process.exit(0)
})

process.on('SIGINT', function() {
    editor.kill('SIGTERM')
});
