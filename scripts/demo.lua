local pipe = require 'slokedlib/pipe'
local promise = require 'slokedlib/promise'
local async = require 'slokedlib/async'
local sched = require 'slokedlib/sched'
local rpc = require 'slokedlib/rpc'

async(function(await)
    local await_unwrap = async.unwrap(await)
    local cursor = rpc:new(sloked.servers.main:connect('document::cursor'))
    local search = rpc:new(sloked.servers.main:connect('document::search'))
    await_unwrap(search:send('connect', 1))
    await_unwrap(search:send('matcher', 'plain'))
    await_unwrap(search:send('match', {
        query='directory',
        flags=0
    }))
    local results = await_unwrap(search('get', nil))

    if await_unwrap(cursor('connect', 1)) then
        local notifier = pipe:promisify(sloked.servers.main:connect('document::notify'))
        await_unwrap(notifier:write(1))
        while true do
            await_unwrap(notifier:drop(1))
            await_unwrap(async(function(await)
                await_unwrap(sched.sleep(500))
                await_unwrap(cursor:send('insert', 'Hello, world!'))
                await_unwrap(notifier:drop(1))
            end))
        end
    end
end):unwrapError()
