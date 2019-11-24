cursor = sloked.servers.main:connect('document::cursor')
cursor:write({
    id=0,
    method='connect',
    params=1
})
cursor:wait()
cursor:drop()
pipe = sloked.servers.main:connect('document::notify')
skip = false
pipe:listen(function ()
    pipe:drop()
    if not skip then
        skip = true
        cursor:write({
            id=1,
            method='insert',
            params='Hello, world!'
        })
    else
        skip = false
    end
end)
pipe:write(1)
