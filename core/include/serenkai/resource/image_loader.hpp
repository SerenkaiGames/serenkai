#pragma once
#include "serenkai/base/raii.hpp"
#include "serenkai/resource/resource_location.hpp"

#include <string_view>
namespace serenkai {
class AssetManager;

struct ImageData {
    int width{};
    int height{};
    int channels{4};
    unsigned char* data{};
};

using ImageWrapper =
    RaiiWrapper<ImageData, void (*)(ImageData&), void (*)(ImageData&)>;

/// @brief Provides a class for loading images
///
/// Loads an image and returns a raw pointer to the pixel bytes.
/// Memory is released automatically; no manual release is needed.
class ImageLoader {
public:
    explicit ImageLoader(AssetManager* asset_manager);

    static void init_image_wrapper(ImageData& image);
    static void cleanup_image_wrapper(ImageData& image);

    /// @brief Load an image into bytes, always as 4-channel RGBA.
    ImageWrapper load(std::string_view loc);
    ImageWrapper load(const ResourceLocation& loc);

private:
    AssetManager* m_asset_manager = nullptr;

    ImageWrapper load_internal(const std::string& path);
};

} // namespace serenkai