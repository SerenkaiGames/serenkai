#include "serenkai/base/raii.hpp"
#include "serenkai/resource/asset_manager.hpp"
#include "serenkai/resource/asset_source.hpp"
#include "serenkai/resource/resource_location.hpp"
#include "serenkai/resource/texture_manager.hpp"

#include <SDL3/SDL.h>
#include <catch2/catch_test_macros.hpp>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <stb_image_write.h>
#include <string>
#include <vector>

namespace fs = std::filesystem;
using namespace serenkai;

namespace {

/// @brief Mock AssetSource for registering file paths in AssetManager.
class MockTextureSource : public AssetSource {
public:
    explicit MockTextureSource(std::string name, AssetFileMap files = {})
        : m_name(std::move(name)), m_files(std::move(files)) {}

    AssetFileMap& get_asset_files() override { return m_files; }
    std::string source_name() const override { return m_name; }

private:
    std::string m_name;
    AssetFileMap m_files;
};

} // namespace

TEST_CASE("TextureManager error handling and caching of failures",
          "[resource][texture_manager]") {
    SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "dummy");
    REQUIRE(SDL_Init(SDL_INIT_VIDEO));

    SDL_Window* window = SDL_CreateWindow("Test", 64, 64, 0);
    REQUIRE(window != nullptr);

    SDL_Renderer* renderer = SDL_CreateRenderer(window, nullptr);
    REQUIRE(renderer != nullptr);

    RaiiGuard sdl_guard([]() {},
                        [&]() {
                            SDL_DestroyRenderer(renderer);
                            SDL_DestroyWindow(window);
                            SDL_Quit();
                        });

    AssetManager asset_manager;
    TextureManager texture_manager(&asset_manager, renderer);

    SECTION("Invalid resource location syntax returns nullptr") {
        CHECK(texture_manager.get(":invalid") == nullptr);
        CHECK(texture_manager.get("") == nullptr);
    }

    SECTION("Missing asset returns nullptr and caches failure") {
        auto* tex1 = texture_manager.get("test:textures/nonexistent.png");
        CHECK(tex1 == nullptr);

        // Calling again should return cached nullptr
        auto* tex2 = texture_manager.get("test:textures/nonexistent.png");
        CHECK(tex2 == nullptr);
    }
}

TEST_CASE("TextureManager loads, caches, and clears textures",
          "[resource][texture_manager]") {
    SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "dummy");
    REQUIRE(SDL_Init(SDL_INIT_VIDEO));

    SDL_Window* window = SDL_CreateWindow("Test", 64, 64, 0);
    REQUIRE(window != nullptr);

    SDL_Renderer* renderer = SDL_CreateRenderer(window, nullptr);
    REQUIRE(renderer != nullptr);

    RaiiGuard sdl_guard([]() {},
                        [&]() {
                            SDL_DestroyRenderer(renderer);
                            SDL_DestroyWindow(window);
                            SDL_Quit();
                        });

    fs::path temp_dir =
        fs::temp_directory_path() / "serenkai_test_texture_manager";
    fs::create_directories(temp_dir);
    RaiiGuard cleanup_guard([]() {},
                            [&temp_dir]() {
                                std::error_code ec;
                                fs::remove_all(temp_dir, ec);
                            });

    const int width = 4;
    const int height = 4;
    const int channels = 4;
    const std::vector<uint8_t> pixels(width * height * channels, 255);

    fs::path png_path = temp_dir / "white4x4.png";
    int write_res = stbi_write_png(png_path.string().c_str(), width, height,
                                   channels, pixels.data(), width * channels);
    REQUIRE(write_res != 0);

    AssetManager asset_manager;
    auto loc = *ResourceLocation::parse("test:textures/white4x4.png");
    AssetFileMap files;
    files.emplace(loc, png_path.string());
    asset_manager.merge_source(
        std::make_shared<MockTextureSource>("MockSource", std::move(files)));

    TextureManager texture_manager(&asset_manager, renderer);

    SECTION("Loads valid texture and caches subsequent requests") {
        SDL_Texture* tex1 = texture_manager.get("test:textures/white4x4.png");
        REQUIRE(tex1 != nullptr);

        float w = 0.0f;
        float h = 0.0f;
        CHECK(SDL_GetTextureSize(tex1, &w, &h));
        CHECK(w == 4.0f);
        CHECK(h == 4.0f);

        // Second call should return the exact same cached pointer
        SDL_Texture* tex2 = texture_manager.get("test:textures/white4x4.png");
        CHECK(tex2 == tex1);

        // ResourceLocation overload returns the same cached pointer
        SDL_Texture* tex_loc = texture_manager.get(loc);
        CHECK(tex_loc == tex1);
    }

    SECTION("Clear invalidates cache and allows reload") {
        SDL_Texture* tex1 = texture_manager.get("test:textures/white4x4.png");
        REQUIRE(tex1 != nullptr);

        texture_manager.clear();

        SDL_Texture* tex2 = texture_manager.get("test:textures/white4x4.png");
        REQUIRE(tex2 != nullptr);
        float w = 0.0f;
        float h = 0.0f;
        CHECK(SDL_GetTextureSize(tex2, &w, &h));
        CHECK(w == 4.0f);
        CHECK(h == 4.0f);
    }

    SECTION("Measure size returns correct dimensions") {
        auto size = texture_manager.measure_size("test:textures/white4x4.png");
        CHECK(size == glm::ivec2{4, 4});

        auto missing_size =
            texture_manager.measure_size("test:textures/missing.png");
        CHECK(missing_size == glm::ivec2{0, 0});

        auto invalid_size = texture_manager.measure_size(":invalid");
        CHECK(invalid_size == glm::ivec2{0, 0});
    }
}
