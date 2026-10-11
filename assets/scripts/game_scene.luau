--!nolint FunctionUnused

local map_mgr: MapManager? = nil

local Event = require("serenkai:scripts/event.luau")

function on_enter(mgr: MapManager)
	map_mgr = mgr
	if map_mgr then
		map_mgr:switch_map("serenkai:maps/tea_house.tmx")
	end

	Event.emit("serenkai:events/enter_game")
end

function on_update(dt: number)
	Event.emit("serenkai:events/update_game")
end

function on_leave()
	Event.emit("serenkai:events/leave_game")
end
