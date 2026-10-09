#pragma once

#include "map_data.hpp"

#include <optional>
namespace serenkai {
/// @brief Manager class for the game's map scene
///
/// Used to manage the map's tiles and objects on the map.
class Map {
public:
    Map(const Map&) = delete;
    Map(Map&&) noexcept = default;
    Map& operator=(const Map&) = delete;
    Map& operator=(Map&&) noexcept = default;

    /// @brief Construct a Map from parsed data.
    explicit Map(MapData data);
    ~Map() = default;

    glm::ivec2 map_size() const;

    glm::ivec2 tile_size() const;

    /// @brief Total map dimensions in pixels (map_size * tile_size).
    glm::ivec2 pixel_size() const;

    /// @brief Check whether the tile is on the edge.
    bool is_in_bounds(glm::ivec2 tile_pos) const;

    glm::ivec2 world_to_tile(glm::vec2 world_pos) const;

    glm::vec2 tile_to_world(glm::ivec2 tile_pos) const;

    const MapData& data() const;

    std::optional<Tile> get_tile(std::string_view layer_name,
                                 glm::ivec2 tile_pos) const;

    bool set_tile(std::string_view layer_name, glm::ivec2 tile_pos,
                  const Tile& tile); // todo

    bool clear_tile(std::string_view layer_name, glm::ivec2 tile_pos); // todo

    void fill_tiles(std::string_view layer_name, glm::ivec2 min_pos,
                    glm::ivec2 max_pos, const Tile& tile); // todo

    std::vector<MapObject> find_object(std::string_view name) const;

    std::vector<MapObject> find_objects_by_type(std::string_view type) const;

    bool add_object(std::string_view group_name, const MapObject& object);

    bool remove_object(std::string_view group_name,
                       std::string_view object_name);

    /// @brief The player's initial position when entering the map; may be null.
    std::optional<glm::vec2> get_spawn_point(std::string_view name) const;

    bool has_layer(std::string_view name) const;

    void set_layer_visible(std::string_view name, bool visible);

    bool is_solid(glm::ivec2 tile_pos) const;

    bool check_collision(const glm::vec4& aabb) const;

    bool is_dirty() const;

    void mark_dirty(bool dirty = true);

    void update(float dt);

    void render() const; // todo

private:
    MapData m_data;
    bool m_dirty = false;
};
} // namespace serenkai