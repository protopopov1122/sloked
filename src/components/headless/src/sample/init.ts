
import { ScreenClient, ScreenSizeClient, ScreenSplitterDirection } from '../lib/clients/screen'
import { ScreenInputClient, KeyInputEvent, ControlKeyCode } from '../lib/clients/screenInput'
import { DocumentSetClient } from '../lib/clients/documents'
import * as net from 'net'
import { NetSlaveServer, AuthServer, BinarySerializer, EncryptedStream, Crypto,
    DefaultCrypto, EncryptedDuplexStream, ModifyableCredentialStorage,
    DefaultCredentialStorage, CompressedStream,
    Authenticator, SlaveAuthenticator } from 'sloked-nodejs'
import { Duplex } from 'stream'
import * as path from 'path'
import { ShutdownClient } from '../lib/clients/shutdown'
import { TextPaneClient, TextGraphics, BackgroundGraphics } from '../lib/clients/textPane'

async function initializeEditor(host: string, port: number): Promise<NetSlaveServer> {
    const socket = new net.Socket()
    await new Promise<any>(resolve => {
        socket.connect(port, host, resolve)
    })

    const crypto: Crypto = new DefaultCrypto()
    const key: Buffer = await crypto.deriveKey('password', 'salt')
    const cryptoStream: EncryptedDuplexStream = new EncryptedStream(socket, crypto, key)
    const stream: Duplex = new CompressedStream(cryptoStream)
    const credentials: ModifyableCredentialStorage = new DefaultCredentialStorage(crypto)
    credentials.newAccount('user1', 'password1')
    const authenticator: Authenticator = new SlaveAuthenticator(crypto, credentials, 'salt', cryptoStream.getEncryption())
    const slave = new NetSlaveServer(stream, new BinarySerializer(), authenticator)
    await slave.authorize('user1')
    return slave
}

async function initializeEditorScreen(editor: AuthServer<string>, argv: string[]): Promise<void> {
    const documentSetClient = new DocumentSetClient(await editor.connect('/document/manager'))
    const docId = await documentSetClient.open(path.resolve(argv[0]), 'system', 'system', 'default')
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
            await documentSetClient.saveAs(path.resolve(argv[1]))
            const shutdown = new ShutdownClient(await editor.connect('/editor/shutdown'))
            await shutdown.requestShutdown()
        } else {
            await renderStatus()
        }
    }).listenAll('/', true)
}

export async function connectToEditor(host: string, port: number, argv: string[]) {
    const editor = await initializeEditor(host, port)
    await initializeEditorScreen(editor, argv)
    return editor
}