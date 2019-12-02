local pipe = require 'slokedlib/pipe'
local promise = require 'slokedlib/promise'
local async = require 'slokedlib/async'
local sched = require 'slokedlib/sched'
local mutex = require 'slokedlib/mutex'

local RpcResponse = {}
RpcResponse.__index = RpcResponse

function RpcResponse:new(client, id)
    local obj = {
        client=client,
        id=id,
        queue={}
    }
    setmetatable(obj, self)
    client.responses[id] = obj
    return obj
end

function RpcResponse:__gc()
    self:close()
end

function RpcResponse:close()
    if self.id ~= nil then
        table.remove(self.client.responses, self.id)
        self.id = nil
    end
end

function RpcResponse:receive()
    return async(function(await)
        local await_unwrap = async.unwrap(await)
        while #self.queue == 0 do
            await_unwrap(self.client.mtx:lock())
            if #self.queue == 0 then
                await(self.client:receive())
            end
            self.client.mtx:unlock()
        end
        return table.remove(self.queue, 1)
    end)
end

function RpcResponse:read()
    return async(function(await)
        local await_unwrap = async.unwrap(await)
        local result = await_unwrap(self:receive())
        self:close()
        return result
    end)
end

local RpcClient = {}
RpcClient.__index = RpcClient

function RpcClient:new(p)
    local obj = {
        pipe=pipe:promisify(p),
        nextId=0,
        responses={},
        mtx=mutex:new()
    }
    setmetatable(obj, self)
    return obj
end

function RpcClient:__call(method, params, detach)
    if type(detach) ~= 'boolean' then
        detach = false
    end
    local id = self.nextId
    self.nextId = id + 1
    local writeReq = self.pipe:write({
        id=id,
        method=method,
        params=params
    })
    if detach then
        return writeReq
    else
        return writeReq:next(function()
            return RpcResponse:new(self, id)
        end)
    end
end

function RpcClient:receive()
    return async(function(await)
        local await_unwrap = async.unwrap(await)
        repeat
            local msg = await_unwrap(self.pipe:read())
            if msg ~= nil and self.responses[msg.id] then
                local queue = self.responses[msg.id].queue
                queue[#queue + 1] = msg
            end
        until self.pipe:empty()
    end)
end

return RpcClient