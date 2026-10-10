#include "serenkai/base/raii.hpp"
#include "serenkai/game/map.hpp"
#include "serenkai/game/map_data.hpp"
#include "serenkai/render/map_renderer.hpp"
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
#include <utility>
#include <vector>

namespace fs = std::filesystem;
using namespace serenkai;

namespace {

class MockRendererAssetSource : public AssetSource {
public:
    explicit MockRendererAssetSource(std::string name, AssetFileMap files = {})
        : m_name(std::move(name)), m_files(std::move(files)) {}

    AssetFileMap& get_asset_files() override { return m_files; }
    std::string source_name() const override { return m_name; }

private:
    std::string m_name;
    AssetFileMap m_files;
};

struct TestSdlContext {
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;

    TestSdlContext() {
        SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "dummy");
        if (SDL_Init(SDL_INIT_VIDEO)) {
            window = SDL_CreateWindow("MapRendererTest", 64, 64, 0);
            if (window) {
                renderer = SDL_CreateRenderer(window, nullptr);
            }
        }
    }

    ~TestSdlContext() {
        if (renderer) {
            SDL_DestroyRenderer(renderer);
        }
        if (window) {
            SDL_DestroyWindow(window);
        }
        SDL_Quit();
    }

    bool is_valid() const { return renderer != nullptr; }
};

} // namespace

TEST_CASE("MapRenderer null pointer and boundary safety",
          "[render][map_renderer]") {
    TestSdlContext ctx;
    REQUIRE(ctx.is_valid());

    AssetManager asset_manager;
    TextureManager texture_manager(&asset_manager, ctx.renderer);

    MapData empty_data{};
    empty_data.map_size = {2, 2};
    empty_data.tile_size = {16, 16};
    Map map(std::move(empty_data));

    SECTION("Null SDL_Renderer instance is safe") {
        MapRenderer null_renderer(nullptr);
        // Calling render with null renderer should cleanly return without crash
        null_renderer.render(&map, &texture_manager, {0.0f, 0.0f}, 1.0f);
    }

    SECTION("Null Map or TextureManager parameter is safe") {
        MapRenderer map_renderer(ctx.renderer);
        map_renderer.render(nullptr, &texture_manager, {0.0f, 0.0f}, 1.0f);
        map_renderer.render(&map, nullptr, {0.0f, 0.0f}, 1.0f);
        map_renderer.render(nullptr, nullptr, {0.0f, 0.0f}, 1.0f);
    }

    SECTION("Invalid zoom values are safely guarded") {
        MapRenderer map_renderer(ctx.renderer);
        map_renderer.render(&map, &texture_manager, {0.0f, 0.0f}, 0.0f);
        map_renderer.render(&map, &texture_manager, {0.0f, 0.0f}, -1.0f);
    }
}

TEST_CASE("MapRenderer renders tile and image layers",
          "[render][map_renderer]") {
    TestSdlContext ctx;
    REQUIRE(ctx.is_valid());

    fs::path temp_dir =
        fs::temp_directory_path() / "serenkai_test_map_renderer";
    fs::create_directories(temp_dir);
    RaiiGuard cleanup_guard([]() {},
                            [&temp_dir]() {
                                std::error_code ec;
                                fs::remove_all(temp_dir, ec);
                            });

    // Generate a 32x32 tileset png and a 64x64 background image png
    fs::path tileset_path = temp_dir / "tileset.png";
    const std::vector<uint8_t> tileset_pixels(32 * 32 * 4, 255);
    REQUIRE(stbi_write_png(tileset_path.string().c_str(), 32, 32, 4,
                           tileset_pixels.data(), 32 * 4) != 0);

    fs::path bg_path = temp_dir / "bg.png";
    const std::vector<uint8_t> bg_pixels(64 * 64 * 4, 200);
    REQUIRE(stbi_write_png(bg_path.string().c_str(), 64, 64, 4,
                           bg_pixels.data(), 64 * 4) != 0);

    AssetManager asset_manager;
    auto tileset_loc = *ResourceLocation::parse("test:tilesets/tileset.png");
    auto bg_loc = *ResourceLocation::parse("test:images/bg.png");

    AssetFileMap files;
    files.emplace(tileset_loc, tileset_path.string());
    files.emplace(bg_loc, bg_path.string());
    asset_manager.merge_source(std::make_shared<MockRendererAssetSource>(
        "MockRendererSource", std::move(files)));

    TextureManager texture_manager(&asset_manager, ctx.renderer);

    // Build test map data
    MapData data{};
    data.map_size = {4, 4};
    data.tile_size = {16, 16};

    Tileset ts{};
    ts.name = "test_tileset";
    ts.loc = tileset_loc;
    ts.tile_size = {16, 16};
    ts.total_size = {32, 32};
    ts.column_count = 2;
    ts.tile_count = 4;
    data.tilesets.push_back(ts);

    // Visible TileLayer with various flip configurations
    TileLayer visible_tiles{};
    visible_tiles.name = "Ground";
    visible_tiles.size = {4, 4};
    visible_tiles.visible = true;

    // Tile 0: normal
    visible_tiles.tiles.emplace_back(1, Tile::FlipFlag::None, 0, 0);
    // Tile 1: empty
    visible_tiles.tiles.emplace_back();
    // Tile 2: horizontal flip
    visible_tiles.tiles.emplace_back(1, Tile::FlipFlag::Horizontal, 0, 0);
    // Tile 3: vertical flip
    visible_tiles.tiles.emplace_back(1, Tile::FlipFlag::Vertical, 0, 0);
    // Tile 4: diagonal flip
    visible_tiles.tiles.emplace_back(1, Tile::FlipFlag::Diagonal, 0, 0);
    // Tile 5: diagonal + horizontal flip (90 deg CW in Tiled)
    visible_tiles.tiles.emplace_back(
        1, Tile::FlipFlag::Diagonal | Tile::FlipFlag::Horizontal, 0, 0);
    // Tile 6: diagonal + vertical flip (270 deg CW in Tiled)
    visible_tiles.tiles.emplace_back(
        1, Tile::FlipFlag::Diagonal | Tile::FlipFlag::Vertical, 0, 0);
    // Tile 7: diagonal + horizontal + vertical flip
    visible_tiles.tiles.emplace_back(1,
                                     Tile::FlipFlag::Diagonal |
                                         Tile::FlipFlag::Horizontal |
                                         Tile::FlipFlag::Vertical,
                                     0, 0);
    // Fill remaining tiles
    while (visible_tiles.tiles.size() < 16) {
        visible_tiles.tiles.emplace_back(2, Tile::FlipFlag::None, 0, 1);
    }
    data.layers.push_back(std::move(visible_tiles));

    // Invisible TileLayer
    TileLayer hidden_tiles{};
    hidden_tiles.name = "Hidden";
    hidden_tiles.size = {4, 4};
    hidden_tiles.visible = false;
    hidden_tiles.tiles.resize(16);
    data.layers.push_back(std::move(hidden_tiles));

    // Visible ImageLayer
    ImageLayer visible_img{};
    visible_img.name = "Background";
    visible_img.loc = bg_loc;
    visible_img.size = {64, 64};
    visible_img.visible = true;
    data.layers.push_back(std::move(visible_img));

    // Invisible ImageLayer
    ImageLayer hidden_img{};
    hidden_img.name = "HiddenBg";
    hidden_img.loc = bg_loc;
    hidden_img.size = {64, 64};
    hidden_img.visible = false;
    data.layers.push_back(std::move(hidden_img));

    Map map(std::move(data));
    MapRenderer renderer(ctx.renderer);

    SECTION("Renders at default camera and zoom") {
        renderer.render(&map, &texture_manager, {0.0f, 0.0f}, 1.0f);
    }

    SECTION("Renders with 2x zoom and offset camera") {
        renderer.render(&map, &texture_manager, {16.0f, 16.0f}, 2.0f);
    }

    SECTION("Camera clamping with extreme values") {
        // Negative coordinates clamp to 0
        renderer.render(&map, &texture_manager, {-100.0f, -100.0f}, 1.0f);
        // Exceeding world size clamps to upper bound
        renderer.render(&map, &texture_manager, {1000.0f, 1000.0f}, 1.0f);
    }

    SECTION("Small map clamping when map is smaller than viewport") {
        MapData small_data{};
        small_data.map_size = {1, 1};
        small_data.tile_size = {16, 16};
        Map small_map(std::move(small_data));

        renderer.render(&small_map, &texture_manager, {10.0f, 10.0f}, 1.0f);
    }
}
