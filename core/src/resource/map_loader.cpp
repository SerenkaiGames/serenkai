#include "serenkai/resource/map_loader.hpp"

#include "serenkai/base/assert.hpp"
#include "serenkai/game/map_data.hpp"
#include "serenkai/resource/asset_manager.hpp"
#include "serenkai/resource/resource_location.hpp"

#include <exception>
#include <filesystem>
#include <fmt/format.h>
#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_float4.hpp>
#include <glm/ext/vector_int2.hpp>
#include <optional>
#include <spdlog/spdlog.h>
#include <string>
#include <string_view>
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
namespace fs = std::filesystem;
namespace serenkai {

namespace {
glm::ivec2 to_ivec2(const tmx::Vector2u& t) { return {t.x, t.y}; }
glm::vec2 to_vec(const tmx::Vector2f& t) { return {t.x, t.y}; }

/// @brief Convert the absolute path or relative path of an image in the map to
/// a ResourceLocation
/// @param image_path The image path parsed by tmxlite (may be an absolute or
/// relative path)
/// @param map_path The actual path of the map on disk
/// (e.g.,"/home/.../assets/maps/level1.tmx")
/// @param map_loc The resource location of the map (e.g.,
/// "serenkai:maps/level1.tmx")
std::optional<ResourceLocation>
convert_to_resource_location(std::string_view image_path,
                             std::string_view map_path,
                             const ResourceLocation& map_loc) {
    if (image_path.empty()) {
        return std::nullopt;
    }

    try {
        fs::path map_dir = fs::path(map_path).parent_path();
        fs::path img_p(image_path);

        fs::path rel_path;
        if (img_p.is_absolute()) {
            rel_path = fs::relative(img_p, map_dir);
        } else {
            rel_path = img_p;
        }

        fs::path map_loc_dir = fs::path(map_loc.path()).parent_path();

        fs::path normalized_res_path =
            (map_loc_dir / rel_path).lexically_normal();

        std::string full_loc_str = fmt::format(
            "{}:{}", map_loc.ns(), normalized_res_path.generic_string());

        auto rl = ResourceLocation::parse(full_loc_str);
        if (!rl) {
            spdlog::error("Failed to parse resource location for image: '{}'",
                          full_loc_str);
            return std::nullopt;
        }

        return rl;

    } catch (const std::exception& e) {
        spdlog::error(
            "Failed to convert image path '{}' to resource location: {}",
            image_path, e.what());

        return std::nullopt;
    }
}

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

        MapObject obj{};
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

void load_image_layer(MapData& data, const tmx::ImageLayer& layer,
                      std::string_view map_path,
                      const ResourceLocation& map_loc) {
    ImageLayer image{};
    image.name = layer.getName();
    image.visible = layer.getVisible();

    auto img_loc =
        convert_to_resource_location(layer.getImagePath(), map_path, map_loc);
    if (img_loc) {
        image.loc = *img_loc;
    } else {
        spdlog::warn("ImageLayer '{}' has invalid image path '{}'",
                     layer.getName(), layer.getImagePath());
    }

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

    auto map_loc = ResourceLocation::parse(loc);
    if (!map_loc) {
        return std::nullopt;
    }

    auto map_path = m_asset_manager->get(*map_loc);
    if (!map_path) {
        spdlog::error("Failed to resolve asset path for '{}'", loc);
        return std::nullopt;
    }

    tmx::Map map;

    if (!map.load(*map_path)) {
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
    load_layers(data, layers, tilesets, *map_path, *map_loc);

    for (auto& tileset : tilesets) {
        Tileset set{};
        set.name = tileset.getName();

        auto img_loc = convert_to_resource_location(tileset.getImagePath(),
                                                    *map_path, *map_loc);
        if (!img_loc) {
            spdlog::error("Tileset '{}' has invalid image path '{}'",
                          tileset.getName(), tileset.getImagePath());
            return std::nullopt;
        }
        set.loc = *img_loc;

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
                            const std::vector<tmx::Tileset>& tilesets,
                            std::string_view map_path,
                            const ResourceLocation& map_loc) const {
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
            load_image_layer(data, image, map_path, map_loc);
        } break;
        case Group: {
            const auto& group = layer->getLayerAs<tmx::LayerGroup>();
            load_layers(data, group.getLayers(), tilesets, map_path, map_loc);
        } break;
        }
    }
}

} // namespace serenkai