
import { ScreenClient, ScreenSizeClient, ScreenSplitterDirection } from '../lib/clients/screen'
import { ScreenInputClient, KeyInputEvent, ControlKeyCode } from '../lib/clients/screenInput'
import { DocumentSetClient } from '../lib/clients/documents'
import * as net from 'net'
import { NetSlaveServer, AuthServer, BinarySerializer, EncryptedStream, Crypto,
    DefaultCrypto, ModifyableCredentialStorage,
    DefaultCredentialStorage, CompressedStream,
    Authenticator, SlaveAuthenticator, StreamEncryption } from 'sloked-nodejs'
import { Duplex } from 'stream'
import * as path from 'path'
import { ShutdownClient } from '../lib/clients/shutdown'
import { TextPaneClient, TextGraphics, BackgroundGraphics } from '../lib/clients/textPane'
import { NamespaceClient } from '../lib/clients/namespace'

function makeEncrypted(stream: Duplex, crypto: Crypto, key: Buffer): [Duplex, StreamEncryption] {
    const encrypted = new EncryptedStream(stream, crypto, key)
    return [encrypted, encrypted.getEncryption()]
}

function makeCompressed(stream: Duplex): Duplex {
    return new CompressedStream(stream)
}

async function setupConnection(rawStream: Duplex, configuration: any): Promise<[Duplex, Authenticator?]> {
    let stream: Duplex = rawStream
    let authenticator: Authenticator | undefined = undefined
    if (configuration.crypto) {
        const salt: string = configuration.crypto.salt
        const crypto: Crypto = new DefaultCrypto()
        const key: Buffer = await crypto.deriveKey(configuration.crypto.defaultKey.password, salt)
        const [cryptoStream, streamEncryption] = makeEncrypted(stream, crypto, key)
        const credentials: ModifyableCredentialStorage = new DefaultCredentialStorage(crypto)
        for (let user of configuration.crypto.authentication.slave.users) {
            credentials.newAccount(user.id, user.password)
        }
        stream = cryptoStream
        authenticator = new SlaveAuthenticator(crypto, credentials, salt, streamEncryption)
    }
    if (configuration.network.compression) {
        stream = makeCompressed(stream)
    }
    return [stream, authenticator]
}

async function initializeEditor(configuration: any): Promise<NetSlaveServer> {
    const socket = new net.Socket()
    await new Promise<any>(resolve => {
        socket.connect(configuration.containers.main.server.netServer.port, configuration.containers.main.server.netServer.host, resolve)
    })

    const [stream, authenticator] = await setupConnection(socket, configuration.containers.secondary)
    const slave = new NetSlaveServer(stream, new BinarySerializer(), authenticator)
    if (configuration.containers.secondary.crypto) {
        await slave.authorize(configuration.containers.secondary.crypto.authentication.slave.users[0].id)
    }
    return slave
}

async function initializeEditorScreen(editor: AuthServer<string>, argv: string[]): Promise<void> {
    const namespaceClient = new NamespaceClient(await editor.connect('/namespace/root'))
    const documentPath = await namespaceClient.fromNative(path.resolve(argv[0]))
    const documentSetClient = new DocumentSetClient(await editor.connect('/document/manager'))
    const docId = await documentSetClient.open(documentPath, 'system', 'system', 'default')
    console.log(docId)

    const screenSizeClient = new ScreenSizeClient()
    await screenSizeClient.connect(await editor.connect('/screen/size/notify'))
    const screenClient = new ScreenClient(await editor.connect('/screen/manager'))
    await screenClient.Handle.newMultiplexer('/')
    const mainWindow = await screenClient.Multiplexer.newWindow('/', {x: 0, y: 0}, screenSizeClient.size())
    screenSizeClient.on('resize', (size: {width: number, height: number}) => {
        screenClient.Multiplexer.resizeWidth(mainWindow, size)
    })
    await screenClient.Handle.newSplitter(mainWindow, ScreenSplitterDirection.Vertical)
    await screenClient.Splitter.newWindow(mainWindow, {
        dim: 1
    })
    await screenClient.Splitter.newWindow(mainWindow, {
        dim: 0,
        min: 1
    })
    await screenClient.Handle.newTabber('/0/0')
    await screenClient.Tabber.newWindow('/0/0')
    await screenClient.Handle.newTextEditor('/0/0/0', docId)

    const textPane = new TextPaneClient(await editor.connect('/screen/component/text/pane'))
    await textPane.connect('/0/1', [], false)
    const render = textPane.getRenderer()
    let counter = 0
    async function renderStatus() {
        render.setBackgroundMode(BackgroundGraphics.Blue);
        render.setTextMode(TextGraphics.Bold);
        render.setTextMode(TextGraphics.Underscore);
        render.setPosition(0, 0);
        render.clearArea(10, 1);
        render.write(`${counter++}`);
        await render.flush();
    }
    await renderStatus()

    const screenInput = new ScreenInputClient(await editor.connect('/screen/component/input/notify'))
    await screenInput.on('input', async (evt: KeyInputEvent) => {
        if (evt.key === ControlKeyCode.Escape && !evt.alt) {
            const targetPath = await namespaceClient.fromNative(path.resolve(argv[1]))
            await documentSetClient.saveAs(targetPath)
            const shutdown = new ShutdownClient(await editor.connect('/editor/shutdown'))
            await shutdown.requestShutdown()
        } else {
            await renderStatus()
        }
    }).listenAll('/', true)
}

export async function connectToEditor(configuration: any, argv: string[]) {
    const editor = await initializeEditor(configuration)
    await initializeEditorScreen(editor, argv)
    return editor
}