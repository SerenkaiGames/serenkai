-- camera.d.luau

export type Camera = {
	pos: vector,
	zoom: number,
	set_pos: (self: Camera, pos: vector) -> (),
	set_zoom: (self: Camera, zoom: number) -> (),
	add_zoom: (self: Camera, zoom: number) -> (),
	move: (self: Camera, distance: vector) -> (),
}
