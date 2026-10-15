-- globals.d.luau

declare log: {
    info: (...any) -> (),
    warn: (...any) -> (),
    error: (...any) -> (),
    debug: (...any) -> (),
}

declare print: (...any) -> ()

declare require: (string) -> any

 export type World = {
    Camera: WorldCamera,
    Map: WorldMap,
    Entities: WorldEntities,
}

declare World: World