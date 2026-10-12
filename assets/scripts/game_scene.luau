--!nolint FunctionUnused

local map_mgr: MapManager
local camera: Camera
local speed: number = 100
local Event = require("serenkai:scripts/event.luau")

function on_enter(mgr: MapManager, cam: Camera)
	map_mgr = mgr
	camera = cam

	init_map()

	Event.emit("serenkai:events/enter_game")
end

function on_update(dt: number)
	update_move(dt)
	update_zoom()
	Event.emit("serenkai:events/update_game")
end

function on_leave()
	Event.emit("serenkai:events/leave_game")
end

function init_map()
	map_mgr:switch_map("serenkai:maps/tea_house.tmx")

	local map = map_mgr:current_map()
	if map then
		local point = map:get_spawn_point("spawn_point")
		if point then
			camera:set_pos(point)
		end
	end
end

function update_move(dt: number)
	local dx = 0
	local dy = 0

	if Input.is_down(Input.Action["inputs/move_up"]) then
		dy -= dt * speed
	end

	if Input.is_down(Input.Action["inputs/move_down"]) then
		dy += dt * speed
	end

	if Input.is_down(Input.Action["inputs/move_left"]) then
		dx -= dt * speed
	end

	if Input.is_down(Input.Action["inputs/move_right"]) then
		dx += dt * speed
	end

	if dx ~= 0 or dy ~= 0 then
		camera:move(vector.create(dx, dy, 0))
	end
end

function update_zoom()
	if Input.is_pressed(Input.Action["inputs/zoom_in"]) then
		camera:add_zoom(1)
	end

	if Input.is_pressed(Input.Action["inputs/zoom_out"]) then
		camera:add_zoom(-1)
	end
end
