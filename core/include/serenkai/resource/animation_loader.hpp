#pragma once

#include "serenkai/resource/animation_data.hpp"
#include "serenkai/resource/resource_location.hpp"

#include <optional>
#include <string_view>
namespace serenkai {
class AssetManager;
class AnimationLoader {
public:
    explicit AnimationLoader(AssetManager* asset_manager);

    std::optional<SpriteAnimationData> load(std::string_view loc);
    std::optional<SpriteAnimationData> load(const ResourceLocation& loc);

private:
    AssetManager* m_asset_manager = nullptr;
};
} // namespace serenkai