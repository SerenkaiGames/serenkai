#pragma once

#include "serenkai/game/map.hpp"
#include "serenkai/resource/map_loader.hpp"
#include "serenkai/resource/resource_location.hpp"

#include <string_view>
#include <unordered_map>
namespace serenkai {
class AssetManager;
/// @brief Class for managing maps
///
/// Switches maps, loads and caches maps.
class MapManager {
public:
    MapManager(const MapManager&) = delete;
    MapManager(MapManager&&) = delete;
    MapManager& operator=(const MapManager&) = delete;
    MapManager& operator=(MapManager&&) = delete;

    explicit MapManager(AssetManager* asset_manager);
    ~MapManager();

    bool switch_map(std::string_view loc,
                    std::string_view spawn_point_name = "");

    void unload_current_map();

    std::shared_ptr<Map> current_map() const;

    bool has_map() const;

    void update(float dt);

private:
    AssetManager* m_asset_manager = nullptr;
    MapLoader m_loader;

    std::unordered_map<ResourceLocation, std::shared_ptr<Map>> m_map_cache;
    std::shared_ptr<Map> m_current_map = nullptr;
};
} // namespace serenkai