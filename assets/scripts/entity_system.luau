local Event = require("serenkai:scripts/event.luau")

local EntitySystem = {}
local cleanups_by_entity: { [EntityId]: { () -> () } } = {}

function EntitySystem.create(loc: string)
	return World.Entities.create_with_loc(loc)
end

--[[
	Automatically register events through EntitySystem, 
	and automatically manage and clean up events that have been registered.
]]
function EntitySystem.on(id: EntityId, event: string, callback: EventListener): () -> ()
	local unregister = Event.on(event, callback)

	local list = cleanups_by_entity[id]
	if not list then
		list = {}
		cleanups_by_entity[id] = list
	end
	table.insert(list, unregister)

	return unregister
end

function EntitySystem.cleanup(id: EntityId)
	local list = cleanups_by_entity[id]
	if list then
		for _, unregister in list do
			unregister()
		end
		cleanups_by_entity[id] = nil
	end
end

-- Automatically destroy the entity and handle event unregistration.
function EntitySystem.destroy(id: EntityId)
	EntitySystem.cleanup(id)
	World.Entities.destroy(id)
end

return EntitySystem
