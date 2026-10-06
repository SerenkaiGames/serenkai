#include "serenkai/resource/image_loader.hpp"

#include "serenkai/base/assert.hpp"
#include "serenkai/resource/asset_manager.hpp"

#include <spdlog/spdlog.h>
#include <stb_image.h>
#include <string_view>

namespace serenkai {

ImageLoader::ImageLoader(AssetManager* asset_manager)
    : m_asset_manager(asset_manager) {}

void ImageLoader::init_image_wrapper(ImageData&) {}
void ImageLoader::cleanup_image_wrapper(ImageData& image) {
    if (image.data) {
        stbi_image_free(image.data);
    }
}

ImageWrapper ImageLoader::load(std::string_view loc) {
    ImageWrapper image{init_image_wrapper, cleanup_image_wrapper};
    if (!m_asset_manager) {
        SE_ASSERT(false);
        return image;
    }

    auto path = m_asset_manager->get(loc);
    if (!path) {
        spdlog::error("Failed to load image {}", loc);
        return image;
    }

    return load_internal(*path);
}

ImageWrapper ImageLoader::load(const ResourceLocation& loc) {
    ImageWrapper image{init_image_wrapper, cleanup_image_wrapper};
    if (!m_asset_manager) {
        SE_ASSERT(false);
        return image;
    }

    auto path = m_asset_manager->get(loc);
    if (!path) {
        spdlog::error("Failed to load image {}", loc.str());
        return image;
    }

    return load_internal(*path);
}

ImageWrapper ImageLoader::load_internal(const std::string& path) {
    ImageWrapper image{init_image_wrapper, cleanup_image_wrapper};
    int width = 0, height = 0, channels = 0;
    unsigned char* data =
        stbi_load(path.c_str(), &width, &height, &channels, STBI_rgb_alpha);

    if (!data) {
        spdlog::error("Failed to load image {}", path);
        return image;
    }

    image->height = height;
    image->width = width;
    image->data = data;

    return image;
}

} // namespace serenkai