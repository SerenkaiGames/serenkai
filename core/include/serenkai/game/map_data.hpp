#pragma once

#include "serenkai/resource/resource_location.hpp"

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
    enum class FlipFlag : std::uint8_t {
        None = 0,
        Horizontal = 1 << 0,
        Vertical = 1 << 1,
        Diagonal = 1 << 2,
    };

    // Global gid with the flag bits removed.
    std::uint32_t gid{0};
    // Flip information.
    FlipFlag flag{Tile::FlipFlag::None};
    // Tileset index
    std::uint16_t tileset{0};
    std::uint16_t local_id{0};

    constexpr bool is_empty() const noexcept { return gid == 0; }

    /// @brief Check if a specific flip flag is set.
    constexpr bool has_flip(FlipFlag f) const noexcept {
        return (static_cast<std::uint8_t>(flag) &
                static_cast<std::uint8_t>(f)) != 0;
    }

    /// @brief Convenient helpers for renderer
    constexpr bool flip_horizontal() const noexcept {
        return has_flip(FlipFlag::Horizontal);
    }
    constexpr bool flip_vertical() const noexcept {
        return has_flip(FlipFlag::Vertical);
    }
    constexpr bool flip_diagonal() const noexcept {
        return has_flip(FlipFlag::Diagonal);
    }
};

constexpr Tile::FlipFlag operator|(Tile::FlipFlag a,
                                   Tile::FlipFlag b) noexcept {
    return static_cast<Tile::FlipFlag>(static_cast<std::uint8_t>(a) |
                                       static_cast<std::uint8_t>(b));
}

constexpr Tile::FlipFlag operator&(Tile::FlipFlag a,
                                   Tile::FlipFlag b) noexcept {
    return static_cast<Tile::FlipFlag>(static_cast<std::uint8_t>(a) &
                                       static_cast<std::uint8_t>(b));
}

constexpr Tile::FlipFlag& operator|=(Tile::FlipFlag& a,
                                     Tile::FlipFlag b) noexcept {
    a = a | b;
    return a;
}

using MapPropertyValue =
    std::variant<std::monostate, bool, int, float, std::string, glm::vec4>;

struct MapProperty {
    std::string name;
    MapPropertyValue value;

    template <typename T> bool is() const {
        return std::holds_alternative<T>(value);
    }

    template <typename T> const T* get_if() const {
        return std::get_if<T>(&value);
    }

    template <typename T> T get_or(const T& default_value) const {
        if (const auto* val = get_if<T>()) {
            return *val;
        }
        return default_value;
    }
};

struct TileLayer {
    std::string name;
    std::vector<Tile> tiles;
    glm::ivec2 size{0};
    bool visible{true};
    std::vector<MapProperty> properties;

    /// @brief Get tile at 2D coordinate (x, y), or nullptr if out of bounds.
    const Tile* get_tile(int x, int y) const noexcept {
        if (x < 0 || x >= size.x || y < 0 || y >= size.y) {
            return nullptr;
        }
        return &tiles[static_cast<std::size_t>(y * size.x + x)];
    }

    const Tile* get_tile(glm::ivec2 pos) const noexcept {
        return get_tile(pos.x, pos.y);
    }
};

struct MapObject {
    std::string name;
    std::string type;
    glm::vec2 pos{0};
    bool visible{true};
    std::vector<MapProperty> properties;
    float rotation{0.0f};
    glm::vec4 aabb{0};
};

struct ObjectGroup {
    enum class DrawOrder {
        Index,  // draw in the order in which they appear
        TopDown // draw sorted by their Y position
    };
    std::string name;
    DrawOrder order = ObjectGroup::DrawOrder::Index;
    std::vector<MapObject> objects;
    std::vector<MapProperty> properties;
    bool visible{true};
};

struct ImageLayer {
    std::string name;
    ResourceLocation loc;
    glm::ivec2 size;
    bool visible{true};
};

struct Tileset {
    std::string name;
    ResourceLocation loc;
    glm::ivec2 total_size{0};
    glm::ivec2 tile_size{0};
    std::uint32_t spacing{0};
    std::uint32_t margin{0};
    std::uint32_t tile_count{0};
    std::uint32_t column_count{0};

    glm::vec4 get_rect(std::uint16_t local_id) const {

        if (!column_count) {
            return glm::vec4{0.0f};
        }

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