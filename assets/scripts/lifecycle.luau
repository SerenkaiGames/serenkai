export type Listener = (...any) -> ()

local listeners: { [string]: { Listener } } = {}

local Lifecycle = {}

--[[
    Registration function for lifecycle events.
    Returns an unregister function.
    Supported events for registration:
    update: (number) -> () called once per frame
]]
function Lifecycle.on(event: string, callback: Listener)
	local list = listeners[event]
	if not list then
		list = {}
		listeners[event] = list
	end

	table.insert(list, callback)

	return function()
		local idx = table.find(list, callback)
		if idx then
			table.remove(list, idx)
		end
	end
end

function Lifecycle.emit(event: string, ...: any)
	local list = listeners[event]

	if not list then
		return
	end

	for _, callback in list do
		local sucess, err = pcall(callback, ...)
		if not sucess then
			log.error(`Error in event '{event}' listener: {err}`)
		end
	end
end

print("Lifecycle initialization successful.")

return Lifecycle
