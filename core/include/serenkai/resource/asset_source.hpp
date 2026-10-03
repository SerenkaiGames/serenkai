#pragma once
#include "serenkai/resource/resource_location.hpp"

#include <unordered_map>
namespace serenkai {
using AssetFileMap = std::unordered_map<ResourceLocation, std::string>;

class AssetSource {
public:
    /// @brief Returns a reference to an AssetFileMap.
    virtual AssetFileMap& get_asset_files() = 0;

    virtual std::string source_name() const = 0;

protected:
};
} // namespace serenkai