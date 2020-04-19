export default interface Pipe {
    isOpen(): boolean;
    available(): number;
    empty(): boolean;
    read(): any;
    write(msg: any): void;
    listen(callback: () => void): void;
    close(): void;
}