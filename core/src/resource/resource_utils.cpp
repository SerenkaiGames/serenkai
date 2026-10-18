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
        fs::path parent_loc_dir = fs::path(parent_loc.path()).parent_path();

        fs::path rel_path;

        if (target_p.is_absolute()) {
            rel_path = target_p.lexically_relative(parent_dir);
        } else {

            bool share_root = false;
            if (!parent_dir.empty()) {
                auto t_it = target_p.begin();
                auto p_it = parent_dir.begin();
                if (t_it != target_p.end() && p_it != parent_dir.end() &&
                    *t_it == *p_it) {
                    share_root = true;
                }
            }

            if (share_root) {
                rel_path = target_p.lexically_relative(parent_dir);
            } else {
                rel_path = target_p;
            }
        }

        fs::path normalized = (parent_loc_dir / rel_path).lexically_normal();
        std::string norm_str = normalized.generic_string();

        if (norm_str.starts_with("..") || norm_str.starts_with("/")) {
            return std::nullopt;
        }

        std::string full_loc_str =
            fmt::format("{}:{}", parent_loc.ns(), norm_str);

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