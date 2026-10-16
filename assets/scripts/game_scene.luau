--!nolint FunctionUnused

local Event = require("serenkai:scripts/event.luau")
local EntitySystem = require("serenkai:scripts/entity_system.luau")

function on_enter()
	init_map()
	init_player()
	Event.emit("serenkai:events/enter_game")
end

function on_update(dt: number)
	Event.emit("serenkai:events/update_game")
end

function on_leave()
	Event.emit("serenkai:events/leave_game")
end

function init_map()
	World.Map.switch_map("serenkai:maps/tea_house.tmx")

	local map = World.Map.current_map()
	if map then
		local point = map:get_spawn_point("spawn_point")
		if point then
			World.Camera.set_pos(point)
		end
	end
end

function init_player()
	EntitySystem.create("serenkai:entities/player.json")
end
