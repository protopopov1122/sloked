export interface CredentialAccount {
    getIdentifier(): string;
    getPassword(): string;
    deriveKey(salt: string): Promise<Buffer>;
}

export interface CredentialStorage {
    hasAccount(id: string): boolean;
    getAccount(id: string): CredentialAccount;
}