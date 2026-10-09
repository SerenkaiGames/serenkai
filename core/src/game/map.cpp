#include "serenkai/game/map.hpp"

#include <glm/common.hpp>
#include <spdlog/spdlog.h>
#include <utility>
#include <variant>

namespace serenkai {
Map::Map(MapData data) : m_data(std::move(data)) {
    build_layer_index();
    build_object_index();
}

void Map::build_layer_index() {
    m_layer_index.clear();
    for (std::size_t i = 0; i < m_data.layers.size(); ++i) {
        std::visit(
            [&](const auto& l) {
                auto it = m_layer_index.find(l.name);
                if (it != m_layer_index.end()) {
                    spdlog::warn("Duplicate map layer name {}", l.name);
                }
                m_layer_index[l.name] = i;
            },
            m_data.layers[i]);
    }
}

void Map::build_object_index() {
    m_by_name.clear();
    m_by_type.clear();

    for (std::size_t li = 0; li < m_data.layers.size(); ++li) {
        const auto* og = std::get_if<ObjectGroup>(&m_data.layers[li]);
        if (!og) {
            continue;
        }

        for (std::size_t oi = 0; oi < og->objects.size(); ++oi) {
            const auto& obj = og->objects[oi];
            if (!obj.name.empty()) {
                m_by_name[obj.name].emplace_back(li, oi);
            }

            if (!obj.type.empty()) {
                m_by_type[obj.type].emplace_back(li, oi);
            }
        }
    }
}

glm::ivec2 Map::map_size() const { return m_data.map_size; }

glm::ivec2 Map::tile_size() const { return m_data.tile_size; }

glm::ivec2 Map::pixel_size() const {
    return m_data.tile_size * m_data.map_size;
}

bool Map::is_in_bounds(glm::ivec2 tile_pos) const {
    return tile_pos.x >= 0 && tile_pos.x < m_data.map_size.x &&
           tile_pos.y >= 0 && tile_pos.y < m_data.map_size.y;
}

glm::ivec2 Map::world_to_tile(glm::vec2 world_pos) const {
    const glm::vec2 ts = glm::vec2(m_data.tile_size);
    return glm::ivec2(glm::floor(world_pos / ts));
}

glm::vec2 Map::tile_to_world(glm::ivec2 tile_pos) const {
    return glm::vec2(tile_pos) * glm::vec2(m_data.tile_size);
}

const MapData& Map::data() const { return m_data; }

std::optional<Tile> Map::get_tile(std::string_view layer_name,
                                  glm::ivec2 tile_pos) const {

    auto it = m_layer_index.find(layer_name);
    if (it == m_layer_index.end()) {
        return std::nullopt;
    }

    const auto* tl = std::get_if<TileLayer>(&m_data.layers[it->second]);
    if (!tl) {
        return std::nullopt;
    }

    if (const Tile* t = tl->get_tile(tile_pos)) {
        return *t;
    }

    return std::nullopt;
}

std::vector<MapObject> Map::find_object(std::string_view name) const {
    std::vector<MapObject> result;

    auto it = m_by_name.find(name);
    if (it == m_by_name.end()) {
        return result;
    }

    result.reserve(it->second.size());
    for (auto [li, oi] : it->second) {
        if (const auto* og = std::get_if<ObjectGroup>(&m_data.layers[li])) {
            result.push_back(og->objects[oi]);
        }
    }
    return result;
}

std::vector<MapObject> Map::find_objects_by_type(std::string_view type) const {
    std::vector<MapObject> result;

    auto it = m_by_type.find(type);
    if (it == m_by_type.end()) {
        return result;
    }

    result.reserve(it->second.size());
    for (auto [li, oi] : it->second) {
        if (const auto* og = std::get_if<ObjectGroup>(&m_data.layers[li])) {
            result.push_back(og->objects[oi]);
        }
    }
    return result;
}

bool Map::has_layer(std::string_view name) const {
    auto it = m_layer_index.find(name);
    return it != m_layer_index.end();
}

void Map::set_layer_visible(std::string_view name, bool visible) {
    auto it = m_layer_index.find(name);
    if (it == m_layer_index.end()) {
        return;
    }

    std::visit([visible](auto& layer) { layer.visible = visible; },
               m_data.layers[it->second]);
}

bool Map::is_tile_dirty() const { return m_tile_dirty; }
bool Map::is_object_dirty() const { return m_object_dirty; }

void Map::mark_tile_dirty(bool dirty) { m_tile_dirty = dirty; }
void Map::mark_object_dirty(bool dirty) { m_object_dirty = dirty; }

void Map::update(float) {

    if (is_object_dirty()) {
        build_object_index();
        mark_object_dirty(false);
    }
}

} // namespace serenkai