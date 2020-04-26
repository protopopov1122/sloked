export interface Pipe {
    isOpen(): boolean;
    available(): number;
    empty(): boolean;
    read(): Promise<any>;
    write(msg: any): void;
    listen(callback: () => void): void;
    close(): void;
}