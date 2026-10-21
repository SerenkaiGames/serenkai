#include "serenkai/application/localization_manager.hpp"
#include "serenkai/base/language.hpp"
#include "serenkai/base/raii.hpp"
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

class MockLocalizationAssetSource : public AssetSource {
public:
    explicit MockLocalizationAssetSource(std::string name,
                                         AssetFileMap files = {})
        : m_name(std::move(name)), m_files(std::move(files)) {}

    AssetFileMap& get_asset_files() override { return m_files; }
    std::string source_name() const override { return m_name; }

private:
    std::string m_name;
    AssetFileMap m_files;
};

fs::path create_temp_file(const fs::path& dir, std::string_view filename,
                          std::string_view content) {
    fs::path file_path = dir / filename;
    std::ofstream out(file_path);
    out << content;
    return file_path;
}

} // namespace

TEST_CASE("LocalizationManager state and language configuration",
          "[application][localization_manager]") {
    AssetManager asset_manager;
    LocalizationManager loc_mgr(&asset_manager);

    SECTION("Default languages") {
        CHECK(loc_mgr.current_language() == Language::en_US);
        CHECK(loc_mgr.fallback_language() == Language::en_US);
    }

    SECTION("Change current and fallback language") {
        loc_mgr.set_language(Language::zh_CN);
        CHECK(loc_mgr.current_language() == Language::zh_CN);

        loc_mgr.set_fallback_language(Language::en_US);
        CHECK(loc_mgr.fallback_language() == Language::en_US);
    }
}

TEST_CASE("LocalizationManager loading and error handling",
          "[application][localization_manager]") {
    AssetManager asset_manager;
    LocalizationManager loc_mgr(&asset_manager);

    fs::path temp_dir =
        fs::temp_directory_path() / "serenkai_test_loc_mgr_loading";
    fs::create_directories(temp_dir);
    RaiiGuard cleanup_guard([]() {},
                            [&temp_dir]() {
                                std::error_code ec;
                                fs::remove_all(temp_dir, ec);
                            });

    SECTION("Non-existent resource location returns false") {
        CHECK_FALSE(
            loc_mgr.load_language(Language::en_US, "test:locales/missing.po"));
    }

    SECTION("Invalid resource location returns false") {
        CHECK_FALSE(loc_mgr.load_language(Language::en_US, ":invalid_loc"));
    }

    SECTION("Language mismatch returns false") {
        std::string content = R"(msgid ""
msgstr ""
"Language: en_US\n"

msgctxt "ui:start"
msgid "Start"
msgstr "Start Game"
)";
        fs::path po_path = create_temp_file(temp_dir, "en_test.po", content);
        auto loc = *ResourceLocation::parse("test:locales/en_test.po");
        AssetFileMap files;
        files.emplace(loc, po_path.string());
        asset_manager.merge_source(
            std::make_shared<MockLocalizationAssetSource>("MockMismatch",
                                                          std::move(files)));

        // Expect zh_CN but file specifies en_US
        CHECK_FALSE(loc_mgr.load_language(Language::zh_CN, loc));
    }
}

TEST_CASE("LocalizationManager translation, fallback and formatting",
          "[application][localization_manager]") {
    AssetManager asset_manager;
    LocalizationManager loc_mgr(&asset_manager);

    fs::path temp_dir =
        fs::temp_directory_path() / "serenkai_test_loc_mgr_features";
    fs::create_directories(temp_dir);
    RaiiGuard cleanup_guard([]() {},
                            [&temp_dir]() {
                                std::error_code ec;
                                fs::remove_all(temp_dir, ec);
                            });

    std::string en_content = R"(msgid ""
msgstr ""
"Language: en_US\n"

msgctxt "ui:start_game"
msgid "Start"
msgstr "Start Game"

msgctxt "ui:quit_game"
msgid "Quit"
msgstr "Quit Game"

msgctxt "ui:credits_only_en"
msgid "Credits"
msgstr "Credits Roll"

msgctxt "ui:greeting"
msgid "Greeting"
msgstr "Hello, {name}!"

msgctxt "ui:stats"
msgid "Stats"
msgstr "Level: {level}, HP: {hp}"
)";

    std::string zh_content = R"(msgid ""
msgstr ""
"Language: zh_CN\n"

msgctxt "ui:start_game"
msgid "Start"
msgstr "开始游戏"

msgctxt "ui:quit_game"
msgid "Quit"
msgstr "退出游戏"

msgctxt "ui:greeting"
msgid "Greeting"
msgstr "你好，{name}！"
)";

    fs::path en_path = create_temp_file(temp_dir, "en.po", en_content);
    fs::path zh_path = create_temp_file(temp_dir, "zh.po", zh_content);

    auto en_loc = *ResourceLocation::parse("test:locales/en.po");
    auto zh_loc = *ResourceLocation::parse("test:locales/zh.po");

    AssetFileMap files;
    files.emplace(en_loc, en_path.string());
    files.emplace(zh_loc, zh_path.string());
    asset_manager.merge_source(std::make_shared<MockLocalizationAssetSource>(
        "MockLocFiles", std::move(files)));

    REQUIRE(loc_mgr.load_language(Language::en_US, en_loc));
    REQUIRE(loc_mgr.load_language(Language::zh_CN, zh_loc));

    SECTION("Translate in English") {
        loc_mgr.set_language(Language::en_US);
        CHECK(loc_mgr.translate("ui:start_game") == "Start Game");
        CHECK(loc_mgr.translate(*ResourceLocation::parse("ui:quit_game")) ==
              "Quit Game");
    }

    SECTION("Translate after language switch to Chinese") {
        loc_mgr.set_language(Language::zh_CN);
        CHECK(loc_mgr.translate("ui:start_game") == "开始游戏");
        CHECK(loc_mgr.translate(*ResourceLocation::parse("ui:quit_game")) ==
              "退出游戏");
    }

    SECTION("Fallback to fallback language when key missing in current") {
        loc_mgr.set_language(Language::zh_CN);
        loc_mgr.set_fallback_language(Language::en_US);

        // ui:credits_only_en only exists in en_US
        CHECK(loc_mgr.has("ui:credits_only_en"));
        CHECK(loc_mgr.has(*ResourceLocation::parse("ui:credits_only_en")));
        CHECK(loc_mgr.translate("ui:credits_only_en") == "Credits Roll");
        CHECK(loc_mgr.translate(*ResourceLocation::parse(
                  "ui:credits_only_en")) == "Credits Roll");
    }

    SECTION("Missing key returns key itself as fallback") {
        loc_mgr.set_language(Language::zh_CN);
        loc_mgr.set_fallback_language(Language::en_US);

        CHECK_FALSE(loc_mgr.has("ui:missing_key"));
        CHECK_FALSE(loc_mgr.has(*ResourceLocation::parse("ui:missing_key")));
        CHECK_FALSE(loc_mgr.has(":invalid_key"));

        CHECK(loc_mgr.translate("ui:missing_key") == "ui:missing_key");
        CHECK(loc_mgr.translate(*ResourceLocation::parse("ui:missing_key")) ==
              "ui:missing_key");
        CHECK(loc_mgr.translate(*ResourceLocation::parse("missing_key")) ==
              "serenkai:missing_key");
    }

    SECTION("Translate with formatted named arguments") {
        loc_mgr.set_language(Language::en_US);
        CHECK(loc_mgr.translate("ui:greeting",
                                LocalizationManager::arg("name", "Alice")) ==
              "Hello, Alice!");

        CHECK(loc_mgr.translate(
                  "ui:stats", LocalizationManager::arg("level", 42),
                  LocalizationManager::arg("hp", 100)) == "Level: 42, HP: 100");

        loc_mgr.set_language(Language::zh_CN);
        CHECK(loc_mgr.translate("ui:greeting",
                                LocalizationManager::arg("name", "小明")) ==
              "你好，小明！");
    }
}
