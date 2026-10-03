#include "serenkai/base/raii.hpp"
#include "serenkai/resource/asset_manager.hpp"
#include "serenkai/resource/asset_source.hpp"
#include "serenkai/resource/directory_source.hpp"
#include "serenkai/resource/resource_location.hpp"

#include <catch2/catch_test_macros.hpp>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>

namespace fs = std::filesystem;
using namespace serenkai;

namespace {

/// @brief Mock AssetSource for in-memory testing.
class MockAssetSource : public AssetSource {
public:
    explicit MockAssetSource(std::string name, AssetFileMap files = {})
        : m_name(std::move(name)), m_files(std::move(files)) {}

    AssetFileMap& get_asset_files() override { return m_files; }

    std::string source_name() const override { return m_name; }

private:
    std::string m_name;
    AssetFileMap m_files;
};

} // namespace

TEST_CASE("ResourceLocation parsing and formatting", "[resource]") {
    SECTION("Valid explicit namespace and path") {
        auto loc = ResourceLocation::parse("mygame:textures/player.png");
        REQUIRE(loc.has_value());
        CHECK(loc->ns == "mygame");
        CHECK(loc->path == "textures/player.png");
        CHECK(loc->to_string() == "mygame:textures/player.png");
    }

    SECTION("Default namespace when colon is omitted") {
        auto loc = ResourceLocation::parse("textures/player.png");
        REQUIRE(loc.has_value());
        CHECK(loc->ns == ResourceLocation::DEFAULT_NAMESPACE);
        CHECK(loc->path == "textures/player.png");
        CHECK(loc->to_string() == "serenkai:textures/player.png");
    }

    SECTION("Valid characters in path and namespace") {
        auto loc = ResourceLocation::parse("mod01:ui/hud_v2.0-beta.json");
        REQUIRE(loc.has_value());
        CHECK(loc->ns == "mod01");
        CHECK(loc->path == "ui/hud_v2.0-beta.json");
    }

    SECTION("Invalid locations are rejected") {
        CHECK_FALSE(ResourceLocation::parse("").has_value());
        CHECK_FALSE(
            ResourceLocation::parse("/textures/player.png").has_value());
        CHECK_FALSE(
            ResourceLocation::parse(":textures/player.png").has_value());
        CHECK_FALSE(ResourceLocation::parse("serenkai:").has_value());
        CHECK_FALSE(ResourceLocation::parse("serenkai::player").has_value());
        CHECK_FALSE(ResourceLocation::parse("a:b:c").has_value());
        CHECK_FALSE(ResourceLocation::parse("serenkai:../secrets").has_value());
        CHECK_FALSE(ResourceLocation::parse("serenkai:foo..bar").has_value());
        CHECK_FALSE(
            ResourceLocation::parse("serenkai:bad space.png").has_value());
        CHECK_FALSE(
            ResourceLocation::parse("serenkai:bad@char.png").has_value());
        CHECK_FALSE(ResourceLocation::parse("invalid$ns:test.png").has_value());
    }

    SECTION("Equality and hash support") {
        auto loc1 = ResourceLocation::parse("serenkai:texture.png");
        auto loc2 = ResourceLocation::parse("serenkai:texture.png");
        auto loc3 = ResourceLocation::parse("other:texture.png");

        REQUIRE(loc1.has_value());
        REQUIRE(loc2.has_value());
        REQUIRE(loc3.has_value());

        CHECK(*loc1 == *loc2);
        CHECK_FALSE(*loc1 == *loc3);

        std::unordered_set<ResourceLocation> set;
        set.insert(*loc1);
        CHECK(set.contains(*loc2));
        CHECK_FALSE(set.contains(*loc3));
    }
}

TEST_CASE("AssetManager source merging and asset lookup", "[resource]") {
    AssetManager manager;

    auto loc_a = *ResourceLocation::parse("game:textures/a.png");
    auto loc_b = *ResourceLocation::parse("game:textures/b.png");
    auto loc_conflict = *ResourceLocation::parse("game:textures/a.png");

    AssetFileMap files1;
    files1.emplace(loc_a, "/virtual/path/a.png");

    AssetFileMap files2;
    files2.emplace(loc_b, "/virtual/path/b.png");
    files2.emplace(loc_conflict, "/virtual/conflict/a.png");

    auto source1 =
        std::make_shared<MockAssetSource>("Source1", std::move(files1));
    auto source2 =
        std::make_shared<MockAssetSource>("Source2", std::move(files2));

    manager.merge_source(source1);
    CHECK(source1->get_asset_files().empty());

    CHECK(manager.get("game:textures/a.png") == "/virtual/path/a.png");
    CHECK(manager.get("game:textures/b.png") == std::nullopt);
    CHECK(manager.get("nonexistent:path") == std::nullopt);
    CHECK(manager.get("") == std::nullopt);

    // Merge source2 with a conflicting key.
    manager.merge_source(source2);
    // The non-conflicting asset is imported.
    CHECK(manager.get("game:textures/b.png") == "/virtual/path/b.png");
    // Original asset remains unchanged.
    CHECK(manager.get("game:textures/a.png") == "/virtual/path/a.png");
    // Conflicting entry is retained in source2.
    CHECK(source2->get_asset_files().size() == 1);
    CHECK(source2->get_asset_files().contains(loc_conflict));
}

TEST_CASE("DirectorySource scanning and asset discovery", "[resource]") {
    fs::path temp_dir = fs::temp_directory_path() / "serenkai_test_dir_source";
    fs::remove_all(temp_dir);
    fs::create_directories(temp_dir / "textures" / "sub");
    fs::create_directories(temp_dir / "sounds");

    // Clean up temporary directory when test finishes.
    RaiiGuard cleanup_guard([]() {},
                            [&temp_dir]() {
                                std::error_code ec;
                                fs::remove_all(temp_dir, ec);
                            });

    // Create assets.json config.
    {
        std::ofstream config_file(temp_dir / "assets.json");
        config_file << R"({"ns": "testmod"})";
    }

    // Create dummy asset files.
    {
        std::ofstream file(temp_dir / "textures" / "sub" / "icon.png");
        file << "dummy png";
    }
    {
        std::ofstream file(temp_dir / "sounds" / "jump.wav");
        file << "dummy wav";
    }

    DirectorySource source(temp_dir);

    CHECK(source.source_name().contains("Directory Source:"));

    auto& assets = source.get_asset_files();
    // Should have 2 assets (textures/sub/icon.png and sounds/jump.wav).
    // assets.json must NOT be registered as an asset.
    CHECK(assets.size() == 2);

    auto icon_loc = ResourceLocation::parse("testmod:textures/sub/icon.png");
    REQUIRE(icon_loc.has_value());
    CHECK(assets.contains(*icon_loc));

    auto sound_loc = ResourceLocation::parse("testmod:sounds/jump.wav");
    REQUIRE(sound_loc.has_value());
    CHECK(assets.contains(*sound_loc));

    auto config_loc = ResourceLocation::parse("testmod:assets.json");
    REQUIRE(config_loc.has_value());
    CHECK_FALSE(assets.contains(*config_loc));

    SECTION("Non-existent or empty directories are handled safely") {
        DirectorySource invalid_source(temp_dir / "does_not_exist");
        CHECK(invalid_source.get_asset_files().empty());

        fs::path empty_dir = temp_dir / "empty_dir";
        fs::create_directories(empty_dir);
        DirectorySource missing_config_source(empty_dir);
        CHECK(missing_config_source.get_asset_files().empty());
    }
}

TEST_CASE("DirectorySource integration with project assets", "[resource]") {
    fs::path asset_dir;
#ifdef SERENKAI_TEST_ASSET_DIR
    if (fs::exists(SERENKAI_TEST_ASSET_DIR)) {
        asset_dir = SERENKAI_TEST_ASSET_DIR;
    }
#endif
    if (asset_dir.empty()) {
        if (fs::exists("assets")) {
            asset_dir = "assets";
        } else if (fs::exists("../../assets")) {
            asset_dir = "../../assets";
        }
    }

    REQUIRE_FALSE(asset_dir.empty());

    auto source = std::make_shared<DirectorySource>(asset_dir);
    AssetManager manager;
    manager.merge_source(source);

    auto font_path = manager.get("serenkai:fonts/unifont_t-17.0.05.otf");
    REQUIRE(font_path.has_value());
    CHECK(fs::exists(*font_path));
}
