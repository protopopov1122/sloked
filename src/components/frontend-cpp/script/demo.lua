local pipe = require 'slokedlib/pipe'
local promise = require 'slokedlib/promise'
local async = require 'slokedlib/async'
local sched = require 'slokedlib/sched'
local rpc = require 'slokedlib/rpc'

sloked.logger:debug('Lua started')
async(function(await)
    local await_unwrap = async.unwrap(await)
    local cursor = rpc:new(sloked.editors.main.server:connect('/document/cursor'))
    -- local search = rpc:new(sloked.editors.main.server:connect('/document/search'))
    -- local root = rpc:new(sloked.editors.main.server:connect('/namespace/root'))
    -- await_unwrap(search:send('connect', 1))
    -- await_unwrap(search:send('matcher', 'plain'))
    -- await_unwrap(search:send('match', {
    --     query='/',
    --     flags=0
    -- }))
    -- await_unwrap(search:send('replace', {
    --     occurence=0,
    --     by='|'
    -- }))
    -- await_unwrap(root('mount', {
    --     mountpoint='/test/test2',
    --     uri='root:///usr/bin'
    -- }))
    -- await_unwrap(root('mounted'))
    -- await_unwrap(root('uri', '/test/test2/bash'))

    if await_unwrap(cursor('connect', {
        documentId=1
    })) then
        local notifier = pipe:promisify(sloked.editors.main.server:connect('/document/notify'))
        await_unwrap(notifier:write({
            document=1,
            tagger=false
        }))
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


function method1(params)
    return params.x + params.y
end