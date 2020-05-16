import { EventEmitter } from "events";
import * as crypto from 'crypto'
import { applicationStartupServer } from './startup'

export class SlokedDetachedApplication extends EventEmitter {
    constructor(key?: string) {
        super()
        this._key = key
    }

    async start(config: any): Promise<void> {
        const ConfigurationHost = config.configurationServer.host
        const ConfigurationPort = config.configurationServer.port
        const ConfigurationKey = typeof this._key !== 'undefined' ? this._key : crypto.randomBytes(32).toString('base64')
        this.emit('wait', ConfigurationKey)
        try {
            await applicationStartupServer(ConfigurationHost, ConfigurationPort, ConfigurationKey, config)
            this.emit('ready')
        } catch (err) {
            this.emit('error', err)
        }
    }

    private _key?: string
}