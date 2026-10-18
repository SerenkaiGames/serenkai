#include "serenkai/resource/resource_utils.hpp"

#include "serenkai/resource/resource_location.hpp"

#include <filesystem>
#include <fmt/format.h>
#include <optional>
#include <spdlog/spdlog.h>

namespace fs = std::filesystem;

namespace serenkai {

/// @brief Convert the absolute path or relative path of an image in the map to
/// a ResourceLocation
/// @param image_path The image path parsed by tmxlite (may be an absolute or
/// relative path)
/// @param map_path The actual path of the map on disk
/// (e.g.,"/home/.../assets/maps/level1.tmx")
/// @param map_loc The resource location of the map (e.g.,
/// "serenkai:maps/level1.tmx")
std::optional<ResourceLocation>
convert_tmx_to_location(std::string_view image_path, std::string_view map_path,
                        const ResourceLocation& map_loc) {
    if (image_path.empty()) {
        return std::nullopt;
    }

    try {
        fs::path map_dir = fs::path(map_path).parent_path();
        fs::path img_p(image_path);

        std::error_code ec;
        fs::path rel_path = fs::relative(img_p, map_dir, ec);
        if (ec || rel_path.empty()) {
            rel_path = img_p.lexically_relative(map_dir);
        }

        fs::path map_loc_dir = fs::path(map_loc.path()).parent_path();

        fs::path normalized_res_path =
            (map_loc_dir / rel_path).lexically_normal();

        std::string full_loc_str = fmt::format(
            "{}:{}", map_loc.ns(), normalized_res_path.generic_string());

        auto rl = ResourceLocation::parse(full_loc_str);
        if (!rl) {
            spdlog::error("Failed to parse resource location for image: '{}'",
                          full_loc_str);
            return std::nullopt;
        }

        return rl;

    } catch (const std::exception& e) {
        spdlog::error(
            "Failed to convert image path '{}' to resource location: {}",
            image_path, e.what());

        return std::nullopt;
    }
}

std::optional<ResourceLocation>
convert_aseprite_to_location(std::string_view image_file_name,
                             const ResourceLocation& json_loc) {
    if (image_file_name.empty()) {
        return std::nullopt;
    }
    try {
        fs::path json_p = fs::path(json_loc.path()).lexically_normal();
        fs::path parent = json_p.parent_path();

        fs::path normalized_res_path =
            (parent / image_file_name).lexically_normal();

        std::string full_loc_str = fmt::format(
            "{}:{}", json_loc.ns(), normalized_res_path.generic_string());

        auto rl = ResourceLocation::parse(full_loc_str);
        if (!rl) {
            spdlog::error("Failed to parse resource location for image: '{}'",
                          full_loc_str);
            return std::nullopt;
        }

        return rl;
    } catch (const std::exception& e) {
        spdlog::error(
            "Failed to convert image file '{}' to resource location: {}",
            image_file_name, e.what());

        return std::nullopt;
    }
}

} // namespace serenkai