local pipe = require 'slokedlib/pipe'
local promise = require 'slokedlib/promise'
local async = require 'slokedlib/async'
local sched = require 'slokedlib/sched'

async(function(await)
    local await_unwrap = async.unwrap(await)
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
            await(notifier:drop(1))
            await_unwrap(async(function(await)
                await(sched.sleep(500))
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
