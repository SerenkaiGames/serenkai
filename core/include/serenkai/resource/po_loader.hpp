#pragma once

#include "serenkai/resource/po_data.hpp"
#include "serenkai/resource/resource_location.hpp"

#include <string_view>
namespace serenkai {
class AssetManager;
class PoLoader {
public:
    PoLoader(AssetManager* asset_manager);

    std::optional<PoData> parse(std::string_view loc);
    std::optional<PoData> parse(const ResourceLocation& loc);

private:
    AssetManager* m_asset_manager = nullptr;
};
} // namespace serenkai