export class EventEmitter<T> {
    subscribe(listener: (event: T) => void): () => void {
        const id: number = this._nextId++
        this._subscribers[id] = listener
        return () => {
            if (this._subscribers[id]) {
                delete this._subscribers[id]
            }
        }
    }

    asyncSubscribe(listener: (event: T) => Promise<void>): () => void {
        const id: number = this._nextId++
        this._asyncSubscribers[id] = listener
        return () => {
            if (this._asyncSubscribers[id]) {
                delete this._asyncSubscribers[id]
            }
        }
    }

    async emit(event: T): Promise<void> {
        Object.values(this._subscribers).forEach(listener => listener(event))
        for (let subs of Object.values(this._asyncSubscribers)) {
            await subs(event)
        }
    }

    private _nextId: number = 0
    private _subscribers: { [id: number]: ((event: T) => void) } = {}
    private _asyncSubscribers: { [id: number]: ((event: T) => Promise<void>) } = {}
}