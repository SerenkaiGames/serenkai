#pragma once

#include "serenkai/resource/resource_location.hpp"

#include <optional>
#include <string_view>

namespace serenkai {

/// @brief Converts a relative or absolute file path referenced within an asset
/// file into a ResourceLocation, preserving the parent asset's namespace and
/// directory hierarchy.
/// @param target_path The referenced file path (e.g. image path from JSON/TMX)
/// @param parent_disk_path The actual disk path of the parent asset file
/// @param parent_loc The ResourceLocation of the parent asset
/// @return The resolved ResourceLocation, or std::nullopt on failure
std::optional<ResourceLocation>
resolve_relative_resource_location(std::string_view target_path,
                                   std::string_view parent_disk_path,
                                   const ResourceLocation& parent_loc);

} // namespace serenkai