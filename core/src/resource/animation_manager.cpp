#include "serenkai/resource/animation_manager.hpp"

#include "serenkai/resource/animation_data.hpp"
#include "serenkai/resource/animation_loader.hpp"
#include "serenkai/resource/resource_location.hpp"

#include <memory>
#include <spdlog/spdlog.h>
#include <utility>

namespace serenkai {
AnimationManager::AnimationManager(AssetManager* asset_manager)
    : m_loader(asset_manager) {}

SpriteAnimationData* AnimationManager::get(std::string_view loc) {
    auto res = ResourceLocation::parse(loc);
    if (res) {
        spdlog::error("Invalid resource location {}", loc);
        return nullptr;
    }

    return get(*res);
}

SpriteAnimationData* AnimationManager::get(const ResourceLocation& loc) {
    auto it = m_datas.find(loc);
    if (it != m_datas.end()) {
        return it->second.get();
    }

    auto data = m_loader.load(loc);

    if (!data) {
        spdlog::error("Failed to loader animation {}", loc.str());
        m_datas.try_emplace(loc, nullptr); // Prevent repeated loading.
        return nullptr;
    }

    auto [d, _] = m_datas.try_emplace(
        loc, std::make_unique<SpriteAnimationData>(std::move(*data)));
    return d->second.get();
}

} // namespace serenkai