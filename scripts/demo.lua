local pipe = require 'pipe'
local promise = require 'promise'
local async = require 'async'

async(function(await)
    local cursor = pipe:promisify(sloked.servers.main:connect('document::cursor'))
    await(cursor:write({
        id=0,
        method='connect',
        params=1
    }))

    if await(cursor:read()) then
        local notifier = pipe:promisify(sloked.servers.main:connect('document::notify'))
        await(notifier:write(1))
        while true do
            await(async(function(await)
                await(notifier:drop(1))
                await(promise:new(function(resolve, reject)
                    sloked.sched:setTimeout(resolve, 500)
                end))
                await(cursor:write({
                    id=1,
                    method='insert',
                    params='Hello, world'
                }))
                await(notifier:drop(1))
            end))
        end
    end
end):unwrapError()
