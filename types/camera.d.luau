-- camera.d.luau

export type WorldCamera = {
	pos: () -> vector,
	zoom: () -> number,
	set_pos: (pos: vector) -> (),
	set_zoom: (zoom: number) -> (),
	add_zoom: (zoom: number) -> (),
	move: (distance: vector) -> (),
}
