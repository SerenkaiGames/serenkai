#pragma once

#include <cstdint>
#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_float4.hpp>
#include <glm/ext/vector_int2.hpp>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>
#include <string>
#include <variant>
#include <vector>
namespace serenkai {

struct Tile {
    enum class FlipFlag {
        Horizontal,
        Vertical,
        Diagonal,
        None,
    };

    // Global gid with the flag bits removed.
    std::uint32_t gid{0};
    // Flip information.
    FlipFlag flag{Tile::FlipFlag::None};
    // Tileset index
    uint16_t tileset{0};
    uint16_t local_id{0};
};

struct LayerProperty {
    std::string name;
    std::string value;
    int type = 0;
};

struct TileLayer {
    std::vector<Tile> tiles;
    glm::ivec2 size{0};
    bool visable{true};
    std::vector<LayerProperty> properties;
};

struct LayerObject {
    std::string name;
    std::string type;
    glm::vec2 pos{0};
    bool visable{true};
    std::vector<LayerProperty> properties;
    float rotation{0.0f};
    glm::vec4 aabb{0};
};

struct ObjectGroup {
    enum class DrawOrder {
        Index,  // draw in the order in which they appear
        TopDown // draw sorted by their Y position
    };
    DrawOrder order = ObjectGroup::DrawOrder::Index;
    std::vector<LayerObject> objects;
    std::vector<LayerProperty> properties;
    bool visable{true};
};

struct ImageLayer {
    std::string path;
    glm::ivec2 size;
    bool visable{true};
};

struct Tileset {
    std::string name;
    std::string path;
    glm::ivec2 total_size{0};
    glm::ivec2 tile_size{0};
    std::uint32_t spacing{0};
    std::uint32_t margin{0};
    std::uint32_t tile_count{0};
    std::uint32_t column_count{0};

    glm::vec4 get_rect(std::uint16_t local_id) {
        const auto column = local_id % column_count;
        const auto row = local_id / column_count;

        return {static_cast<float>(margin + column * (tile_size.x + spacing)),
                static_cast<float>(margin + row * (tile_size.y + spacing)),
                static_cast<float>(tile_size.x),
                static_cast<float>(tile_size.y)};
    }
};

using MapLayer = std::variant<TileLayer, ObjectGroup, ImageLayer>;
struct MapData {
    glm::ivec2 map_size{0};
    glm::ivec2 tile_size{0};
    glm::vec4 background_color{0.0f};
    std::vector<MapLayer> layers;
    std::vector<Tileset> tilesets;
};

} // namespace serenkai