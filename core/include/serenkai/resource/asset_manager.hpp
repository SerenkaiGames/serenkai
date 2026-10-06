#pragma once

#include "serenkai/resource/asset_source.hpp"
#include "serenkai/resource/resource_location.hpp"

#include <memory>
#include <optional>
#include <string>
#include <string_view>
namespace serenkai {
class AssetManager {
public:
    std::optional<std::string> get(std::string_view loc) const;
    std::optional<std::string> get(const ResourceLocation& loc) const;
    void merge_source(std::shared_ptr<AssetSource> source);

private:
    AssetFileMap m_files;
};
} // namespace serenkai