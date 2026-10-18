#include "serenkai/base/raii.hpp"
#include "serenkai/resource/animation_data.hpp"
#include "serenkai/resource/animation_manager.hpp"
#include "serenkai/resource/asset_manager.hpp"
#include "serenkai/resource/asset_source.hpp"
#include "serenkai/resource/resource_location.hpp"

#include <catch2/catch_test_macros.hpp>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <utility>

namespace fs = std::filesystem;
using namespace serenkai;

namespace {

/// @brief Mock AssetSource for registering animation file paths in
/// AssetManager.
class MockAnimationManagerAssetSource : public AssetSource {
public:
    explicit MockAnimationManagerAssetSource(std::string name,
                                             AssetFileMap files = {})
        : m_name(std::move(name)), m_files(std::move(files)) {}

    AssetFileMap& get_asset_files() override { return m_files; }
    std::string source_name() const override { return m_name; }

private:
    std::string m_name;
    AssetFileMap m_files;
};

} // namespace

TEST_CASE("AnimationManager error handling and failure caching",
          "[resource][animation_manager]") {
    AssetManager asset_manager;
    AnimationManager anim_manager(&asset_manager);

    SECTION("Invalid resource location syntax returns nullptr") {
        CHECK(anim_manager.get(":invalid") == nullptr);
        CHECK(anim_manager.get("") == nullptr);
    }

    SECTION("Non-existent asset returns nullptr and caches failure") {
        auto* data1 = anim_manager.get("test:animations/nonexistent.json");
        CHECK(data1 == nullptr);

        // Subsequent call returns cached nullptr
        auto* data2 = anim_manager.get("test:animations/nonexistent.json");
        CHECK(data2 == nullptr);
    }
}

TEST_CASE("AnimationManager loads, caches and retrieves animation data",
          "[resource][animation_manager]") {
    fs::path temp_dir =
        fs::temp_directory_path() / "serenkai_test_animation_manager";
    fs::create_directories(temp_dir);
    RaiiGuard cleanup_guard([]() {},
                            [&temp_dir]() {
                                std::error_code ec;
                                fs::remove_all(temp_dir, ec);
                            });

    fs::path anim_path = temp_dir / "hero.json";
    {
        std::ofstream out(anim_path);
        out << R"({
  "frames": [
    {
      "filename": "hero 0.aseprite",
      "frame": {"x": 0, "y": 0, "w": 16, "h": 16},
      "rotated": false,
      "trimmed": false,
      "spriteSourceSize": {"x": 0, "y": 0, "w": 16, "h": 16},
      "sourceSize": {"w": 16, "h": 16},
      "duration": 100
    }
  ],
  "meta": {
    "image": "hero.png",
    "format": "RGBA8888",
    "size": {"w": 16, "h": 16},
    "scale": "1",
    "frameTags": [
      {"name": "hero/idle", "from": 0, "to": 0, "direction": "forward"}
    ]
  }
})";
    }

    AssetManager asset_manager;
    auto loc = *ResourceLocation::parse("test:animations/hero.json");
    AssetFileMap files;
    files.emplace(loc, anim_path.string());
    asset_manager.merge_source(
        std::make_shared<MockAnimationManagerAssetSource>("MockAnimSource",
                                                          std::move(files)));

    AnimationManager anim_manager(&asset_manager);

    SECTION("Loads valid animation and caches subsequent requests") {
        SpriteAnimationData* data1 =
            anim_manager.get("test:animations/hero.json");
        REQUIRE(data1 != nullptr);
        CHECK(data1->image ==
              *ResourceLocation::parse("test:animations/hero.png"));
        CHECK(data1->frames.size() == 1);
        CHECK(data1->clips.size() == 1);

        // Same string_view returns cached pointer
        SpriteAnimationData* data2 =
            anim_manager.get("test:animations/hero.json");
        CHECK(data2 == data1);

        // ResourceLocation overload returns the same cached pointer
        SpriteAnimationData* data_loc = anim_manager.get(loc);
        CHECK(data_loc == data1);
    }
}
