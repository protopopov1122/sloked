local Connection = require 'connection'

-- sloked.sched:defer()
Connection:new(sloked.servers.main:connect('document::cursor'), function(conn)
    local cursor = conn
    if conn:receive({
        id=0,
        method='connect',
        params=1
    }) then
        Connection:new(sloked.servers.main:connect('document::notify'), function(conn)
            conn:write(1)
            local i = 0
            while i < 5 do
                conn:receive()
                i = i + 1
                sloked.sched:setTimeout(function()
                    cursor:write({
                        id=1,
                        method='insert',
                        params='Hello, world!'
                    })
                end, 1000)
                conn:skip(1)
            end
        end)
    end
end)
