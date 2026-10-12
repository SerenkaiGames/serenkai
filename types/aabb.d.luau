-- aabb.d.luau

export type AABB = {
	left: (self: AABB) -> number,
	top: (self: AABB) -> number,
	right: (self: AABB) -> number,
	bottom: (self: AABB) -> number,

	width: (self: AABB) -> number,
	height: (self: AABB) -> number,

	top_left: (self: AABB) -> vector,
	top_right: (self: AABB) -> vector,
	bottom_left: (self: AABB) -> vector,
	bottom_right: (self: AABB) -> vector,
	center: (self: AABB) -> vector,

	min: (self: AABB) -> vector,
	max: (self: AABB) -> vector,

	normalize: (self: AABB) -> (),
	translate: (self: AABB, offset: vector) -> (),
	scale: (self: AABB, factor: vector) -> (),
	set_center: (self: AABB, c: vector) -> (),

	contains: ((self: AABB, p: vector) -> boolean)
		| ((self: AABB, other: AABB) -> boolean),

	intersects: (self: AABB, other: AABB) -> boolean,
	intersection: (self: AABB, other: AABB) -> AABB,
	united: (self: AABB, other: AABB) -> AABB,
	expanded: (self: AABB, amount: number) -> AABB,
	translated: (self: AABB, offset: vector) -> AABB,
	area: (self: AABB) -> number,
	is_empty: (self: AABB) -> boolean,
}

type AABBConstructor =
    (() -> AABB)
    & ((x: number, y: number, w: number, h: number) -> AABB)
    & ((pos: vector, size: vector) -> AABB)

export type AABBStatic = {
    new: AABBConstructor,
    from_center: (center: vector, size: vector) -> AABB,
    from_min_max: (min_p: vector, max_p: vector) -> AABB,
} & AABBConstructor

declare AABB: AABBStatic