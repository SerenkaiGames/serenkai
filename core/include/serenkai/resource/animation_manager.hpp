#pragma once

#include "serenkai/resource/animation_data.hpp"
#include "serenkai/resource/animation_loader.hpp"
#include "serenkai/resource/resource_location.hpp"

#include <memory>
#include <string_view>
#include <unordered_map>
namespace serenkai {
class AssetManager;
/// @brief Load and manage animation resources
///
/// Internally caches animation resources that have already been loaded.
class AnimationManager {
public:
    AnimationManager(AssetManager* asset_manager);

    SpriteAnimationData* get(std::string_view loc);
    SpriteAnimationData* get(const ResourceLocation& loc);

private:
    AnimationLoader m_loader;
    std::unordered_map<ResourceLocation, std::unique_ptr<SpriteAnimationData>>
        m_datas;
};
} // namespace serenkai