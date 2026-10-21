#pragma once

#include "serenkai/resource/po_data.hpp"
#include "serenkai/resource/resource_location.hpp"

#include <optional>
#include <string_view>
namespace serenkai {
class AssetManager;
class PoLoader {
public:
    explicit PoLoader(AssetManager* asset_manager);

    std::optional<PoData> load(std::string_view loc);
    std::optional<PoData> load(const ResourceLocation& loc);

private:
    AssetManager* m_asset_manager = nullptr;
};
} // namespace serenkai