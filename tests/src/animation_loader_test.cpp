#include "serenkai/base/raii.hpp"
#include "serenkai/resource/animation_data.hpp"
#include "serenkai/resource/animation_loader.hpp"
#include "serenkai/resource/asset_manager.hpp"
#include "serenkai/resource/asset_source.hpp"
#include "serenkai/resource/resource_location.hpp"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <utility>

namespace fs = std::filesystem;
using namespace serenkai;
using Catch::Approx;

namespace {

/// @brief Mock AssetSource for registering animation file paths in
/// AssetManager.
class MockAnimationAssetSource : public AssetSource {
public:
    explicit MockAnimationAssetSource(std::string name, AssetFileMap files = {})
        : m_name(std::move(name)), m_files(std::move(files)) {}

    AssetFileMap& get_asset_files() override { return m_files; }
    std::string source_name() const override { return m_name; }

private:
    std::string m_name;
    AssetFileMap m_files;
};

} // namespace

TEST_CASE("AnimationLoader error handling and edge cases",
          "[resource][animation_loader]") {
    AssetManager asset_manager;
    AnimationLoader loader(&asset_manager);

    SECTION("Null AssetManager returns nullopt") {
        AnimationLoader null_loader(nullptr);
        auto result = null_loader.load("test:animations/hero.json");
        CHECK_FALSE(result.has_value());
    }

    SECTION("Non-existent asset returns nullopt") {
        auto result = loader.load("test:animations/missing.json");
        CHECK_FALSE(result.has_value());
    }

    SECTION("Invalid resource location returns nullopt") {
        auto result = loader.load(":invalid_loc");
        CHECK_FALSE(result.has_value());
    }

    SECTION("Malformed JSON returns nullopt") {
        fs::path temp_dir =
            fs::temp_directory_path() / "serenkai_test_anim_corrupted";
        fs::create_directories(temp_dir);
        RaiiGuard cleanup_guard([]() {},
                                [&temp_dir]() {
                                    std::error_code ec;
                                    fs::remove_all(temp_dir, ec);
                                });

        fs::path anim_path = temp_dir / "bad.json";
        {
            std::ofstream out(anim_path);
            out << "{ not a valid json }";
        }

        auto loc = *ResourceLocation::parse("test:animations/bad.json");
        AssetFileMap files;
        files.emplace(loc, anim_path.string());
        asset_manager.merge_source(std::make_shared<MockAnimationAssetSource>(
            "MockCorrupt", std::move(files)));

        auto result = loader.load("test:animations/bad.json");
        CHECK_FALSE(result.has_value());
    }
}

TEST_CASE("AnimationLoader successfully loads valid Aseprite JSON",
          "[resource][animation_loader]") {
    fs::path temp_dir =
        fs::temp_directory_path() / "serenkai_test_anim_valid_load";
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
      "frame": {"x": 0, "y": 0, "w": 32, "h": 32},
      "rotated": false,
      "trimmed": false,
      "spriteSourceSize": {"x": 0, "y": 0, "w": 32, "h": 32},
      "sourceSize": {"w": 32, "h": 32},
      "duration": 100
    },
    {
      "filename": "hero 1.aseprite",
      "frame": {"x": 32, "y": 0, "w": 20, "h": 30},
      "rotated": false,
      "trimmed": true,
      "spriteSourceSize": {"x": 6, "y": 2, "w": 20, "h": 30},
      "sourceSize": {"w": 32, "h": 32},
      "duration": 150
    }
  ],
  "meta": {
    "image": "hero.png",
    "format": "RGBA8888",
    "size": {"w": 64, "h": 32},
    "scale": "1",
    "frameTags": [
      {"name": "hero/idle", "from": 0, "to": 0, "direction": "forward"},
      {"name": "hero/walk", "from": 0, "to": 1, "direction": "pingpong"},
      {"name": "custom:hero/reverse", "from": 0, "to": 1, "direction": "reverse"},
      {"name": "hero/pingpong_rev", "from": 0, "to": 1, "direction": "pingpong_reverse"}
    ]
  }
})";
    }

    AssetManager asset_manager;
    auto loc = *ResourceLocation::parse("test:animations/hero.json");
    AssetFileMap files;
    files.emplace(loc, anim_path.string());
    asset_manager.merge_source(std::make_shared<MockAnimationAssetSource>(
        "MockValid", std::move(files)));

    AnimationLoader loader(&asset_manager);
    auto anim_data = loader.load("test:animations/hero.json");
    REQUIRE(anim_data.has_value());

    // Verify resolved image ResourceLocation
    CHECK(anim_data->image ==
          *ResourceLocation::parse("test:animations/hero.png"));

    // Verify frames
    REQUIRE(anim_data->frames.size() == 2);

    // Frame 0: untrimmed
    const auto& f0 = anim_data->frames[0];
    CHECK(f0.src == glm::vec4(0.0f, 0.0f, 32.0f, 32.0f));
    CHECK(f0.offset == glm::vec2(0.0f, 0.0f));
    CHECK(f0.source_size == glm::vec2(32.0f, 32.0f));
    CHECK(f0.duration == Approx(0.1f));

    // Frame 1: trimmed
    const auto& f1 = anim_data->frames[1];
    CHECK(f1.src == glm::vec4(32.0f, 0.0f, 20.0f, 30.0f));
    CHECK(f1.offset == glm::vec2(6.0f, 2.0f));
    CHECK(f1.source_size == glm::vec2(32.0f, 32.0f));
    CHECK(f1.duration == Approx(0.15f));

    // Verify clips
    REQUIRE(anim_data->clips.size() == 4);

    // Clip 1: Forward
    auto idle_loc = *ResourceLocation::parse("test:hero/idle");
    REQUIRE(anim_data->clips.contains(idle_loc));
    const auto& idle_clip = anim_data->clips[idle_loc];
    CHECK(idle_clip.name == "hero/idle");
    CHECK(idle_clip.from == 0);
    CHECK(idle_clip.to == 0);
    CHECK(idle_clip.mode == PlaybackMode::Forward);

    // Clip 2: Pingpong
    auto walk_loc = *ResourceLocation::parse("test:hero/walk");
    REQUIRE(anim_data->clips.contains(walk_loc));
    const auto& walk_clip = anim_data->clips[walk_loc];
    CHECK(walk_clip.name == "hero/walk");
    CHECK(walk_clip.from == 0);
    CHECK(walk_clip.to == 1);
    CHECK(walk_clip.mode == PlaybackMode::Pingpong);

    // Clip 3: Reverse with explicit namespace
    auto rev_loc = *ResourceLocation::parse("custom:hero/reverse");
    REQUIRE(anim_data->clips.contains(rev_loc));
    const auto& rev_clip = anim_data->clips[rev_loc];
    CHECK(rev_clip.name == "custom:hero/reverse");
    CHECK(rev_clip.from == 0);
    CHECK(rev_clip.to == 1);
    CHECK(rev_clip.mode == PlaybackMode::Reverse);

    // Clip 4: PingpongReverse
    auto pprev_loc = *ResourceLocation::parse("test:hero/pingpong_rev");
    REQUIRE(anim_data->clips.contains(pprev_loc));
    const auto& pprev_clip = anim_data->clips[pprev_loc];
    CHECK(pprev_clip.name == "hero/pingpong_rev");
    CHECK(pprev_clip.from == 0);
    CHECK(pprev_clip.to == 1);
    CHECK(pprev_clip.mode == PlaybackMode::PingpongReverse);
}

TEST_CASE("AnimationLoader filters invalid frameTag ranges",
          "[resource][animation_loader]") {
    fs::path temp_dir =
        fs::temp_directory_path() / "serenkai_test_anim_invalid_tags";
    fs::create_directories(temp_dir);
    RaiiGuard cleanup_guard([]() {},
                            [&temp_dir]() {
                                std::error_code ec;
                                fs::remove_all(temp_dir, ec);
                            });

    fs::path anim_path = temp_dir / "tags.json";
    {
        std::ofstream out(anim_path);
        out << R"({
  "frames": [
    {
      "frame": {"x": 0, "y": 0, "w": 16, "h": 16},
      "trimmed": false,
      "spriteSourceSize": {"x": 0, "y": 0, "w": 16, "h": 16},
      "sourceSize": {"w": 16, "h": 16},
      "duration": 100
    },
    {
      "frame": {"x": 16, "y": 0, "w": 16, "h": 16},
      "trimmed": false,
      "spriteSourceSize": {"x": 0, "y": 0, "w": 16, "h": 16},
      "sourceSize": {"w": 16, "h": 16},
      "duration": 100
    }
  ],
  "meta": {
    "image": "tags.png",
    "frameTags": [
      {"name": "valid/clip", "from": 0, "to": 1, "direction": "forward"},
      {"name": "invalid/from_negative", "from": -1, "to": 1, "direction": "forward"},
      {"name": "invalid/to_oob", "from": 0, "to": 2, "direction": "forward"},
      {"name": "invalid/from_gt_to", "from": 1, "to": 0, "direction": "forward"},
      {"name": "invalid:with:multiple:colons", "from": 0, "to": 1, "direction": "forward"}
    ]
  }
})";
    }

    AssetManager asset_manager;
    auto loc = *ResourceLocation::parse("test:animations/tags.json");
    AssetFileMap files;
    files.emplace(loc, anim_path.string());
    asset_manager.merge_source(std::make_shared<MockAnimationAssetSource>(
        "MockTags", std::move(files)));

    AnimationLoader loader(&asset_manager);
    auto anim_data = loader.load("test:animations/tags.json");
    REQUIRE(anim_data.has_value());

    // Only "valid/clip" should be parsed
    CHECK(anim_data->clips.size() == 1);
    CHECK(
        anim_data->clips.contains(*ResourceLocation::parse("test:valid/clip")));
}
