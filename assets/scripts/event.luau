export type Listener = (...any) -> ()

local listeners: { [string]: { Listener } } = {}

local Event = {}

--[[
    Registration function for events.
    Returns an unregister function.
    Supported engine events for registration:
    serenkai:events/update: (number) -> () called once per frame
]]
function Event.on(event: string, callback: Listener)
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

function Event.emit(event: string, ...: any)
	local list = listeners[event]

	if not list then
		return
	end
	local l = table.clone(list)
	for _, callback in l do
		local success, err = pcall(callback, ...)
		if not success then
			log.error(`Error in event '{event}' listener: {err}`)

			local idx = table.find(list, callback)
			if idx then
				table.remove(list, idx)
			end
		end
	end
end

log.info("Event initialization successful.")

return Event
