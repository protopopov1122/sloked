export default interface Serializer {
    serialize(msg: any): Buffer;
    deserialize(buffer: Buffer): any;
}