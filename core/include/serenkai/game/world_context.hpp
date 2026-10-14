#pragma once

namespace serenkai {
class Camera;
class MapManager;
class EntityManager;

struct WorldContext {
    Camera* camera = nullptr;
    MapManager* map_mgr = nullptr;
    EntityManager* entity_mgr = nullptr;
};
} // namespace serenkai