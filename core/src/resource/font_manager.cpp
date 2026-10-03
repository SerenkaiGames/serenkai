#include "serenkai/resource/font_manager.hpp"

#include "serenkai/resource/asset_manager.hpp"
#include "serenkai/resource/font.hpp"
#include "serenkai/resource/resource_location.hpp"

#include <memory>
#include <utility>
namespace serenkai {
FontManager::FontManager(AssetManager* asset_manager)
    : m_ft_lib_wrapper(make_raii<FT_Library>(ft_lib_init, ft_lib_cleanup)),
      m_asset_manager(asset_manager) {}
Font* FontManager::get(std::string_view font, size_t pixel_size) {

    if (!m_asset_manager) {
        return nullptr;
    }

    auto loc = ResourceLocation::parse(font);
    if (!loc) {
        return nullptr;
    }

    auto it = m_fonts.find(Key{*loc, pixel_size});
    if (it != m_fonts.end()) {
        return it->second.get();
    }

    auto path = m_asset_manager->get(font);
    if (!path) {
        return nullptr;
    }
    auto f = std::make_unique<Font>(*path, pixel_size, m_ft_lib_wrapper.get());

    auto [p, _] = m_fonts.try_emplace(Key{*loc, pixel_size}, std::move(f));
    return p->second.get();
}
} // namespace serenkai