#pragma once

#include "serenkai/resource/resource_location.hpp"

#include <optional>
#include <string_view>

namespace serenkai {
/// @brief Convert the absolute path or relative path of an image in the map to
/// a ResourceLocation
///
/// @warning image path is a filesystem path
std::optional<ResourceLocation>
convert_tmx_to_location(std::string_view image_path, std::string_view tmx_path,
                        const ResourceLocation& tmx_loc);

/// @brief Convert an Aseprite relative path to a ResourceLocation
///
/// @warning The image path must be in the same directory as the json path.
std::optional<ResourceLocation>
convert_aseprite_to_location(std::string_view image_file_name,
                             const ResourceLocation& json_loc);

} // namespace serenkai