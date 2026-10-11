--!nolint FunctionUnused

local map_mgr: MapManager
local camera: Camera
local speed: number = 1
local Event = require("serenkai:scripts/event.luau")

function on_enter(mgr: MapManager, cam: Camera)
	map_mgr = mgr
	camera = cam

	map_mgr:switch_map("serenkai:maps/tea_house.tmx")

	Event.emit("serenkai:events/enter_game")
end

function on_update(dt: number)
	update_move(dt)
	Event.emit("serenkai:events/update_game")
end

function on_leave()
	Event.emit("serenkai:events/leave_game")
end

function update_move(dt: number)
	local distance = vector.create(0, 0)

	if Input.is_down(Input.Action["inputs/move_up"]) then
		distance.y -= dt * speed
	end

	if Input.is_down(Input.Action["inputs/move_down"]) then
		distance.y += dt * speed
	end

	if Input.is_down(Input.Action["inputs/move_left"]) then
		distance.x -= dt * speed
	end

	if Input.is_down(Input.Action["inputs/move_right"]) then
		distance.x += dt * speed
	end

	camera:move(distance)
end
