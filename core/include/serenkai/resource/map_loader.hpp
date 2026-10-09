#pragma once

#include <memory>
#include <optional>
#include <string_view>
#include <vector>

namespace tmx {
class Layer;
class Tileset;
} // namespace tmx

namespace serenkai {
class AssetManager;
struct MapData;
class MapLoader {
public:
    MapLoader(AssetManager* asset_manager);
    std::optional<MapData> load(std::string_view loc) const;

private:
    AssetManager* m_asset_manager;

    void load_layers(MapData& data,
                     const std::vector<std::unique_ptr<tmx::Layer>>& layers,
                     const std::vector<tmx::Tileset>& tilesets) const;
};
} // namespace serenkai