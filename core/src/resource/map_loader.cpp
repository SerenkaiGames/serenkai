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

void convert_properties(const std::vector<tmx::Property>& dist,
                        std::vector<LayerProperty>& out) {
    for (auto& p : dist) {
        LayerProperty mp;
        mp.name = p.getName();
        mp.type = static_cast<int>(p.getType());
        mp.value = p.getStringValue();
        out.push_back(std::move(mp));
    }
}

void load_tile_layer(MapData& data, const tmx::TileLayer& layer,
                     const std::vector<tmx::Tileset>& tilesets) {
    TileLayer t{};
    t.size = to_ivec2(layer.getSize());
    t.visable = layer.getVisible();
    for (auto& tile : layer.getTiles()) {
        std::uint32_t gid = tile.ID & 0x1FFFFFFF;
        std::uint8_t flags = tile.flipFlags;
        Tile::FlipFlag f = Tile::FlipFlag::None;
        if (flags & tmx::TileLayer::FlipFlag::Horizontal) {
            f = Tile::FlipFlag::Horizontal;
        }
        if (flags & tmx::TileLayer::FlipFlag::Vertical) {
            f = Tile::FlipFlag::Vertical;
        }
        if (flags & tmx::TileLayer::FlipFlag::Diagonal) {
            f = Tile::FlipFlag::Diagonal;
        }

        auto idx = find_tileset_index(tilesets, gid);
        if (idx < 0) {
            spdlog::error("Failed to find tile gid {} tileset index", gid);
            continue;
        }

        uint32_t local_id = gid - tilesets[idx].getFirstGID();

        t.tiles.emplace_back(gid, f, idx, local_id);
    }
    convert_properties(layer.getProperties(), t.properties);
    data.layers.emplace_back(std::move(t));
}

void load_object_group(MapData& data, const tmx::ObjectGroup& group) {
    ObjectGroup g{};
    g.visable = group.getVisible();
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
            spdlog::info("Don't support object {} shape, the game is only "
                         "support rectangle",
                         object.getName());
            continue;
        }

        LayerObject obj{};
        obj.name = object.getName();
        obj.type = object.getType();
        obj.pos = to_vec(object.getPosition());
        obj.visable = object.visible();
        obj.rotation = object.getRotation();
        auto& aabb = object.getAABB();
        obj.aabb = {aabb.left, aabb.top, aabb.width, aabb.height};
        convert_properties(object.getProperties(), obj.properties);
    }

    data.layers.emplace_back(std::move(g));
}

void load_image_layer(MapData& data, const tmx::ImageLayer& layer) {
    ImageLayer image{};
    image.visable = layer.getVisible();
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
        spdlog::error("Failed to get {} asset path", loc);
        return std::nullopt;
    }

    tmx::Map map;

    if (!map.load(*path)) {
        spdlog::error("Failed to load {}", loc);
        return std::nullopt;
    }

    if (map.getOrientation() != tmx::Orientation::Orthogonal) {
        spdlog::error("Map {} is not orthogonal, can't load", loc);
        return std::nullopt;
    }

    if (map.isInfinite()) {
        spdlog::error("Don't support infinite map {}", loc);
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