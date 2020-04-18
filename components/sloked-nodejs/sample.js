const SlaveServer = require('./slave')
const { BinarySerializer } = require('./serialize')
const net = require('net')

const socket = new net.Socket()
socket.connect(1234, '::1', async () => {
    const slave = new SlaveServer(socket, new BinarySerializer())
    const root = await slave.connect('/namespace/root')
    root.write({
        id: 1000,
        method: 'uri',
        params: '/usr/bin/bash'
    })
    const resp = await root.read()
    root.close()
    const cursor = await slave.connect('/document/cursor')
    cursor.write({
        id: 0,
        method: 'connect',
        params: 1
    })
    cursor.write({
        id: 1,
        method: 'insert',
        params: resp.result
    })
    
    setTimeout(() => cursor.close(), 1000)
    setTimeout(() => slave.close(), 1500)
})