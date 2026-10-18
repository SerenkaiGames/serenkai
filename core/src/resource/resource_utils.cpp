#include "serenkai/resource/resource_utils.hpp"

#include <filesystem>
#include <fmt/format.h>
#include <spdlog/spdlog.h>

namespace fs = std::filesystem;

namespace serenkai {

std::optional<ResourceLocation>
resolve_relative_resource_location(std::string_view target_path,
                                   std::string_view parent_disk_path,
                                   const ResourceLocation& parent_loc) {
    if (target_path.empty()) {
        return std::nullopt;
    }

    try {
        fs::path parent_dir = fs::path(parent_disk_path).parent_path();
        fs::path target_p(target_path);

        std::error_code ec;

        fs::path rel_path = target_p.is_relative()
                                ? target_p
                                : fs::relative(target_p, parent_dir, ec);

        if (ec || rel_path.empty()) {
            rel_path = target_p.lexically_relative(parent_dir);
        }

        fs::path parent_loc_dir = fs::path(parent_loc.path()).parent_path();

        fs::path normalized_res_path =
            (parent_loc_dir / rel_path).lexically_normal();

        std::string full_loc_str = fmt::format(
            "{}:{}", parent_loc.ns(), normalized_res_path.generic_string());

        auto rl = ResourceLocation::parse(full_loc_str);
        if (!rl) {
            spdlog::error("Failed to parse resource location for '{}'",
                          full_loc_str);
            return std::nullopt;
        }

        return rl;
    } catch (const std::exception& e) {
        spdlog::error("Failed to convert path '{}' to resource location: {}",
                      target_path, e.what());
        return std::nullopt;
    }
}

} // namespace serenkai