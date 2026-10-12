-- map.d.luau

export type MapPropertyValue = boolean | number | string | vector

export type Tile = {
	gid: number,
	local_id: number,
	is_empty: (self: Tile) -> boolean,
}

export type MapObject = {
	name: string,
	type: string,
	pos: vector,
	aabb: AABB,
	visible: boolean,
	get_property: (self: MapObject, key: string) -> MapPropertyValue?,
}

export type Map = {
	map_size: (self: Map) -> vector,
	tile_size: (self: Map) -> vector,
	pixel_size: (self: Map) -> vector,
	is_in_bounds: (self: Map, tile_pos: vector) -> boolean,
	world_to_tile: (self: Map, world_pos: vector) -> vector,
	tile_to_world: (self: Map, tile_pos: vector) -> vector,
	has_layer: (self: Map, name: string) -> boolean,
	set_layer_visible: (self: Map, name: string, visible: boolean) -> (),
	find_object: (self: Map, name: string) -> { MapObject },
	find_objects_by_type: (self: Map, type: string) -> { MapObject },
	get_spawn_point: (self: Map, name: string) -> vector?,
}

export type MapManager = {
	switch_map: (self: MapManager, loc: string, spawn_point_name: string?) -> boolean,
	current_map: (self: MapManager) -> Map?,
	has_map: (self: MapManager) -> boolean,
}
