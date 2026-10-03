#pragma once

#include "serenkai/resource/asset_source.hpp"

#include <filesystem>

namespace serenkai {

/// @brief Asset source from a file directory
///
/// Loads all asset paths from the file directory during construction.
class DirectorySource : public AssetSource {
public:
    /// @brief Recursively searches the specified directory for all assets
    ///
    /// @note The directory must contain an assets.json file.
    DirectorySource(std::filesystem::path dir);

    AssetFileMap& get_asset_files() override;

    std::string source_name() const override;

private:
    const std::string m_dir_str;
    AssetFileMap m_files;

    /// @brief Recursively searches the given directory for assets
    ///
    /// @note May throw an exception.
    void search(std::filesystem::path dir);
};
} // namespace serenkai