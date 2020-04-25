export interface Authenticator {
    authenticate(account: string, nonce: number): Promise<string>
    close(): void
}