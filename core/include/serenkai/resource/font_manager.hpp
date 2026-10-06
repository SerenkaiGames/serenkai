#pragma once

#include "serenkai/base/math.hpp"
#include "serenkai/resource/font.hpp"
#include "serenkai/resource/resource_location.hpp"

#include <cstddef>
#include <memory>
#include <string_view>
#include <unordered_map>
#include <utility>
namespace serenkai {
class AssetManager;

/// @brief Manager class for all fonts
///
/// Manages the lifecycle of all fonts.
class FontManager {
public:
    static constexpr size_t DEFAULT_PIXEL_SIZE = 12;
    static constexpr const char* DEFAULT_FONT =
        "serenkai:fonts/unifont_t-17.0.05.otf";
    explicit FontManager(AssetManager* asset_manager);

    Font* get(std::string_view font, size_t pixel_size = DEFAULT_PIXEL_SIZE);
    Font* get_scaled(const Font& font, size_t scale);

private:
    struct Key {
        ResourceLocation font;
        size_t pixel_size{DEFAULT_PIXEL_SIZE};

        Key(ResourceLocation font, size_t pixel_size = DEFAULT_PIXEL_SIZE)
            : font(std::move(font)), pixel_size(pixel_size) {}

        bool operator==(const Key&) const = default;
        struct Hash {
            std::size_t operator()(const Key& k) const noexcept {
                std::uint32_t h = 0;
                h = combine32(
                    h, hash_to_32(std::hash<ResourceLocation>{}(k.font)));
                h = combine32(h, hash_to_32(std::hash<size_t>{}(k.pixel_size)));
                return fmix32(h);
            }
        };
    };

    FtLibWrapper m_ft_lib_wrapper;
    AssetManager* m_asset_manager = nullptr;
    std::unordered_map<Key, std::unique_ptr<Font>, Key::Hash> m_fonts;
};
} // namespace serenkai