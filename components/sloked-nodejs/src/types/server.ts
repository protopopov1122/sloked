import { Pipe } from '../types/pipe'

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