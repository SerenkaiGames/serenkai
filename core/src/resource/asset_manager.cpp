#include "serenkai/resource/asset_manager.hpp"

#include "serenkai/resource/resource_location.hpp"

#include <optional>
#include <spdlog/spdlog.h>
#include <string>

namespace serenkai {

std::optional<std::string> AssetManager::get(std::string_view loc) const {
    auto resource = ResourceLocation::parse(loc);
    if (!resource) {
        return std::nullopt;
    }
    auto it = m_files.find(*resource);
    if (it == m_files.end()) {
        return std::nullopt;
    }

    return it->second;
}
void AssetManager::merge_source(std::shared_ptr<AssetSource> source) {

    auto& assets = source->get_asset_files();

    const size_t total_size = assets.size();

    m_files.merge(assets);
    if (assets.size()) {
        spdlog::warn("There are {} conflicting keys", assets.size());
    }

    spdlog::info("Added {} assets from {}", total_size - assets.size(),
                 source->source_name());
}
} // namespace serenkai