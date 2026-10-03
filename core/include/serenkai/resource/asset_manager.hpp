#pragma once

#include "serenkai/resource/asset_source.hpp"

#include <memory>
#include <optional>
#include <string>
#include <string_view>
namespace serenkai {
class AssetManager {
public:
    std::optional<std::string> get(std::string_view loc) const;
    void add(std::shared_ptr<AssetSource> source);

private:
    AssetFileMap m_files;
};
} // namespace serenkai