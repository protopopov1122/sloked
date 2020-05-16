import { ConfigurationServer } from './configServer'

export function applicationStartupServer(host: string, port: number, key: string, config: any): Promise<void> {
    return new Promise<void>((resolve, reject) => {
        const configServer = new ConfigurationServer()
        configServer.attach(`${key}:load`, () => JSON.stringify(config))
        configServer.attach(`${key}:ready`, () => {
            console.log('READY')
            resolve()
            configServer.close()
            return ''
        })
        configServer.start(host, port).catch(reject)
    })
}