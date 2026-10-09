#include "serenkai/game/map_manager.hpp"

#include "serenkai/game/map.hpp"
#include "serenkai/resource/map_loader.hpp"
#include "serenkai/resource/resource_location.hpp"

#include <memory>
#include <spdlog/spdlog.h>
#include <utility>

namespace serenkai {
MapManager::MapManager(AssetManager* asset_manager) : m_loader(asset_manager) {}

bool MapManager::switch_map(std::string_view loc, std::string_view) {
    auto res = ResourceLocation::parse(loc);
    if (!res) {
        spdlog::error("Invalid map loc {}", loc);
        return false;
    }

    auto it = m_map_cache.find(*res);

    if (it == m_map_cache.end()) {
        auto map_data = m_loader.load(loc);
        if (!map_data) {
            spdlog::error("Failed to load map data {}", loc);
            return false;
        }
        auto map = std::make_shared<Map>(std::move(*map_data));
        it = m_map_cache.try_emplace(*res, map).first;
    }

    m_current_map = it->second;
    return true;
}

std::shared_ptr<Map> MapManager::current_map() const { return m_current_map; }

bool MapManager::has_map() const { return m_current_map != nullptr; }

void MapManager::update(float dt) {
    if (m_current_map) {
        m_current_map->update(dt);
    }
}
} // namespace serenkai