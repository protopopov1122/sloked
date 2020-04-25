export interface CredentialAccount {
    getIdentifier(): string;
    getPassword(): string;
    deriveKey(salt: string): Promise<Buffer>;
    watch(callback: () => void): () => void
}

export interface CredentialStorage {
    hasAccount(id: string): boolean;
    getAccount(id: string): CredentialAccount;
}