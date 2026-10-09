#include "serenkai/resource/map_loader.hpp"

#include "serenkai/base/assert.hpp"
#include "serenkai/game/map_data.hpp"
#include "serenkai/resource/asset_manager.hpp"

#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_float4.hpp>
#include <glm/ext/vector_int2.hpp>
#include <optional>
#include <spdlog/spdlog.h>
#include <tmxlite/ImageLayer.hpp>
#include <tmxlite/Layer.hpp>
#include <tmxlite/LayerGroup.hpp>
#include <tmxlite/Map.hpp>
#include <tmxlite/Object.hpp>
#include <tmxlite/ObjectGroup.hpp>
#include <tmxlite/Property.hpp>
#include <tmxlite/TileLayer.hpp>
#include <tmxlite/Types.hpp>
#include <utility>
#include <vector>

namespace serenkai {

namespace {
glm::ivec2 to_ivec2(const tmx::Vector2u& t) { return {t.x, t.y}; }
glm::vec2 to_vec(const tmx::Vector2f& t) { return {t.x, t.y}; }

std::ptrdiff_t find_tileset_index(const std::vector<tmx::Tileset>& tilesets,
                                  uint32_t gid) {
    auto it = std::upper_bound(tilesets.begin(), tilesets.end(), gid,
                               [](uint32_t g, const tmx::Tileset& ts) {
                                   return g < ts.getFirstGID();
                               });

    if (it == tilesets.begin()) {
        return -1;
    }
    return (it - 1) - tilesets.begin();
}

void convert_properties(const std::vector<tmx::Property>& src,
                        std::vector<MapProperty>& out) {
    out.reserve(out.size() + src.size());
    for (auto& p : src) {
        MapProperty prop;
        prop.name = p.getName();
        switch (p.getType()) {
        case tmx::Property::Type::Boolean:
            prop.value = p.getBoolValue();
            break;
        case tmx::Property::Type::Float:
            prop.value = p.getFloatValue();
            break;
        case tmx::Property::Type::Int:
            prop.value = p.getIntValue();
            break;
        case tmx::Property::Type::String:
            prop.value = p.getStringValue();
            break;
        case tmx::Property::Type::Colour: {
            const auto& c = p.getColourValue();
            prop.value = glm::vec4(c.r / 255.0f, c.g / 255.0f, c.b / 255.0f,
                                   c.a / 255.0f);
            break;
        }
        case tmx::Property::Type::File:
            prop.value = p.getFileValue();
            break;
        case tmx::Property::Type::Object:
            prop.value = p.getObjectValue();
            break;
        default:
            break;
        }

        out.push_back(std::move(prop));
    }
}

void load_tile_layer(MapData& data, const tmx::TileLayer& layer,
                     const std::vector<tmx::Tileset>& tilesets) {
    TileLayer t{};
    t.name = layer.getName();
    t.size = to_ivec2(layer.getSize());
    t.visible = layer.getVisible();
    const auto& raw_tiles = layer.getTiles();
    t.tiles.reserve(raw_tiles.size());

    for (auto& tile : raw_tiles) {
        std::uint32_t gid = tile.ID & 0x1FFFFFFF;
        if (gid == 0) {
            // Insert empty tiles directly.
            t.tiles.emplace_back();
            continue;
        }

        std::uint8_t flags = tile.flipFlags;
        Tile::FlipFlag f = Tile::FlipFlag::None;

        if (flags & tmx::TileLayer::FlipFlag::Horizontal) {
            f |= Tile::FlipFlag::Horizontal;
        }
        if (flags & tmx::TileLayer::FlipFlag::Vertical) {
            f |= Tile::FlipFlag::Vertical;
        }
        if (flags & tmx::TileLayer::FlipFlag::Diagonal) {
            f |= Tile::FlipFlag::Diagonal;
        }

        auto idx = find_tileset_index(tilesets, gid);
        if (idx < 0) {
            spdlog::error("Failed to find tileset index for tile gid {}", gid);
            // find failed; insert an empty tile.
            t.tiles.emplace_back();
            continue;
        }
        auto tileset_idx = static_cast<std::uint16_t>(idx);
        auto local_id =
            static_cast<std::uint16_t>(gid - tilesets[idx].getFirstGID());

        t.tiles.emplace_back(gid, f, tileset_idx, local_id);
    }
    convert_properties(layer.getProperties(), t.properties);
    data.layers.emplace_back(std::move(t));
}

void load_object_group(MapData& data, const tmx::ObjectGroup& group) {
    ObjectGroup g{};
    g.name = group.getName();
    g.visible = group.getVisible();
    switch (group.getDrawOrder()) {
    case tmx::ObjectGroup::DrawOrder::Index:
        g.order = ObjectGroup::DrawOrder::Index;
        break;
    case tmx::ObjectGroup::DrawOrder::TopDown:
        g.order = ObjectGroup::DrawOrder::TopDown;
        break;
    }

    convert_properties(group.getProperties(), g.properties);

    for (auto& object : group.getObjects()) {
        if (object.getShape() != tmx::Object::Shape::Rectangle) {
            spdlog::info("Unsupported shape for object '{}'; only rectangles "
                         "are supported",
                         object.getName());
            continue;
        }

        LayerObject obj{};
        obj.name = object.getName();
        obj.type = object.getType();
        obj.pos = to_vec(object.getPosition());
        obj.visible = object.visible();
        obj.rotation = object.getRotation();
        auto& aabb = object.getAABB();
        obj.aabb = {aabb.left, aabb.top, aabb.width, aabb.height};
        convert_properties(object.getProperties(), obj.properties);

        g.objects.emplace_back(std::move(obj));
    }

    data.layers.emplace_back(std::move(g));
}

void load_image_layer(MapData& data, const tmx::ImageLayer& layer) {
    ImageLayer image{};
    image.name = layer.getName();
    image.visible = layer.getVisible();
    image.path = layer.getImagePath();
    image.size = to_ivec2(layer.getImageSize());
    data.layers.emplace_back(std::move(image));
}

} // namespace

MapLoader::MapLoader(AssetManager* asset_manager)
    : m_asset_manager(asset_manager) {}

std::optional<MapData> MapLoader::load(std::string_view loc) const {
    if (!m_asset_manager) {
        SE_ASSERT(false);
        return std::nullopt;
    }

    auto path = m_asset_manager->get(loc);
    if (!path) {
        spdlog::error("Failed to resolve asset path for '{}'", loc);
        return std::nullopt;
    }

    tmx::Map map;

    if (!map.load(*path)) {
        spdlog::error("Failed to load map '{}'", loc);
        return std::nullopt;
    }

    if (map.getOrientation() != tmx::Orientation::Orthogonal) {
        spdlog::error(
            "Map '{}' is not orthogonal; only orthogonal maps are supported",
            loc);
        return std::nullopt;
    }

    if (map.isInfinite()) {
        spdlog::error("Infinite map '{}' is not supported", loc);
        return std::nullopt;
    }

    MapData data{};

    data.map_size = to_ivec2(map.getTileCount());
    data.tile_size = to_ivec2(map.getTileSize());

    auto c = map.getBackgroundColour();
    glm::vec4 color(c.r / 255.f, c.g / 255.f, c.b / 255.f, c.a / 255.f);
    data.background_color = color;

    const auto& layers = map.getLayers();
    const auto& tilesets = map.getTilesets();
    load_layers(data, layers, tilesets);

    for (auto& tileset : tilesets) {
        Tileset set{};
        set.name = tileset.getName();
        set.path = tileset.getImagePath();
        set.column_count = tileset.getColumnCount();
        set.tile_count = tileset.getTileCount();
        set.tile_size = to_ivec2(tileset.getTileSize());
        set.total_size = to_ivec2(tileset.getImageSize());
        set.margin = tileset.getMargin();
        set.spacing = tileset.getSpacing();
        data.tilesets.emplace_back(std::move(set));
    }

    return data;
}

void MapLoader::load_layers(MapData& data,
                            const std::vector<tmx::Layer::Ptr>& layers,
                            const std::vector<tmx::Tileset>& tilesets) const {
    for (auto& layer : layers) {
        using enum tmx::Layer::Type;
        switch (layer->getType()) {
        case Tile: {
            const auto& tile = layer->getLayerAs<tmx::TileLayer>();
            load_tile_layer(data, tile, tilesets);
        } break;
        case Object: {
            const auto& group = layer->getLayerAs<tmx::ObjectGroup>();
            load_object_group(data, group);
        } break;
        case Image: {
            const auto& image = layer->getLayerAs<tmx::ImageLayer>();
            load_image_layer(data, image);
        } break;
        case Group: {
            const auto& group = layer->getLayerAs<tmx::LayerGroup>();
            load_layers(data, group.getLayers(), tilesets);
        } break;
        }
    }
}

} // namespace serenkai