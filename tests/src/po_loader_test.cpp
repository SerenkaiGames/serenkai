#include "serenkai/base/raii.hpp"
#include "serenkai/resource/asset_manager.hpp"
#include "serenkai/resource/asset_source.hpp"
#include "serenkai/resource/po_data.hpp"
#include "serenkai/resource/po_loader.hpp"
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

class MockPoAssetSource : public AssetSource {
public:
    explicit MockPoAssetSource(std::string name, AssetFileMap files = {})
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

TEST_CASE("PoLoader error handling and edge cases", "[resource][po_loader]") {
    AssetManager asset_manager;
    PoLoader loader(&asset_manager);

    SECTION("Null AssetManager returns nullopt") {
        PoLoader null_loader(nullptr);
        auto result = null_loader.load("test:locales/zh_CN.po");
        CHECK_FALSE(result.has_value());
    }

    SECTION("Non-existent asset returns nullopt") {
        auto result = loader.load("test:locales/missing.po");
        CHECK_FALSE(result.has_value());
    }

    SECTION("Invalid resource location returns nullopt") {
        auto result = loader.load(":invalid_loc");
        CHECK_FALSE(result.has_value());
    }

    SECTION("Empty PO file returns nullopt") {
        fs::path temp_dir =
            fs::temp_directory_path() / "serenkai_test_po_empty";
        fs::create_directories(temp_dir);
        RaiiGuard cleanup_guard([]() {},
                                [&temp_dir]() {
                                    std::error_code ec;
                                    fs::remove_all(temp_dir, ec);
                                });

        fs::path po_path = create_temp_file(temp_dir, "empty.po", "");
        auto loc = *ResourceLocation::parse("test:locales/empty.po");
        AssetFileMap files;
        files.emplace(loc, po_path.string());
        asset_manager.merge_source(
            std::make_shared<MockPoAssetSource>("MockEmpty", std::move(files)));

        auto result = loader.load(loc);
        CHECK_FALSE(result.has_value());
    }

    SECTION("Missing Language field returns nullopt") {
        fs::path temp_dir =
            fs::temp_directory_path() / "serenkai_test_po_no_lang";
        fs::create_directories(temp_dir);
        RaiiGuard cleanup_guard([]() {},
                                [&temp_dir]() {
                                    std::error_code ec;
                                    fs::remove_all(temp_dir, ec);
                                });

        std::string content = R"(msgid ""
msgstr ""
"Project-Id-Version: Serenkai\n"
"Content-Type: text/plain; charset=UTF-8\n"
)";
        fs::path po_path = create_temp_file(temp_dir, "no_lang.po", content);
        auto loc = *ResourceLocation::parse("test:locales/no_lang.po");
        AssetFileMap files;
        files.emplace(loc, po_path.string());
        asset_manager.merge_source(std::make_shared<MockPoAssetSource>(
            "MockNoLang", std::move(files)));

        auto result = loader.load(loc);
        CHECK_FALSE(result.has_value());
    }

    SECTION("Unsupported Language field returns nullopt") {
        fs::path temp_dir =
            fs::temp_directory_path() / "serenkai_test_po_unsupported_lang";
        fs::create_directories(temp_dir);
        RaiiGuard cleanup_guard([]() {},
                                [&temp_dir]() {
                                    std::error_code ec;
                                    fs::remove_all(temp_dir, ec);
                                });

        std::string content = R"(msgid ""
msgstr ""
"Language: fr_FR\n"
)";
        fs::path po_path = create_temp_file(temp_dir, "fr.po", content);
        auto loc = *ResourceLocation::parse("test:locales/fr.po");
        AssetFileMap files;
        files.emplace(loc, po_path.string());
        asset_manager.merge_source(std::make_shared<MockPoAssetSource>(
            "MockUnsupported", std::move(files)));

        auto result = loader.load(loc);
        CHECK_FALSE(result.has_value());
    }
}

TEST_CASE("PoLoader valid parsing and features", "[resource][po_loader]") {
    AssetManager asset_manager;
    PoLoader loader(&asset_manager);

    fs::path temp_dir = fs::temp_directory_path() / "serenkai_test_po_valid";
    fs::create_directories(temp_dir);
    RaiiGuard cleanup_guard([]() {},
                            [&temp_dir]() {
                                std::error_code ec;
                                fs::remove_all(temp_dir, ec);
                            });

    SECTION("Basic PO file parsing with multiple entries") {
        std::string content = R"(# Test translation
msgid ""
msgstr ""
"Project-Id-Version: Serenkai\n"
"Language: zh_CN\n"
"Content-Type: text/plain; charset=UTF-8\n"

msgctxt "ui:start_game"
msgid "Start"
msgstr "开始游戏"

msgctxt "ui:quit_game"
msgid "Quit"
msgstr "退出游戏"
)";
        fs::path po_path = create_temp_file(temp_dir, "valid.po", content);
        auto loc = *ResourceLocation::parse("test:locales/valid.po");
        AssetFileMap files;
        files.emplace(loc, po_path.string());
        asset_manager.merge_source(
            std::make_shared<MockPoAssetSource>("MockValid", std::move(files)));

        auto result = loader.load(loc);
        REQUIRE(result.has_value());
        CHECK(result->lang == Language::zh_CN);
        REQUIRE(result->entries.size() == 2);
        CHECK(result->entries[0].msgctxt ==
              *ResourceLocation::parse("ui:start_game"));
        CHECK(result->entries[0].msgstr == "开始游戏");
        CHECK(result->entries[1].msgctxt ==
              *ResourceLocation::parse("ui:quit_game"));
        CHECK(result->entries[1].msgstr == "退出游戏");
    }

    SECTION("Language code with hyphen normalization") {
        std::string content = R"(msgid ""
msgstr ""
"Language: en-US\n"

msgctxt "ui:welcome"
msgid "Welcome"
msgstr "Welcome!"
)";
        fs::path po_path = create_temp_file(temp_dir, "en_us.po", content);
        auto loc = *ResourceLocation::parse("test:locales/en_us.po");
        AssetFileMap files;
        files.emplace(loc, po_path.string());
        asset_manager.merge_source(std::make_shared<MockPoAssetSource>(
            "MockHyphen", std::move(files)));

        auto result = loader.load(loc);
        REQUIRE(result.has_value());
        CHECK(result->lang == Language::en_US);
        REQUIRE(result->entries.size() == 1);
        CHECK(result->entries[0].msgstr == "Welcome!");
    }

    SECTION("Multi-line continuation strings") {
        std::string content = R"(msgid ""
msgstr ""
"Language: zh_CN\n"

msgctxt "dialog:intro"
msgid "Intro"
msgstr ""
"Line 1\n"
"Line 2"
)";
        fs::path po_path = create_temp_file(temp_dir, "multi.po", content);
        auto loc = *ResourceLocation::parse("test:locales/multi.po");
        AssetFileMap files;
        files.emplace(loc, po_path.string());
        asset_manager.merge_source(
            std::make_shared<MockPoAssetSource>("MockMulti", std::move(files)));

        auto result = loader.load(loc);
        REQUIRE(result.has_value());
        REQUIRE(result->entries.size() == 1);
        CHECK(result->entries[0].msgstr == "Line 1\nLine 2");
    }

    SECTION("Escape sequence handling") {
        std::string content = R"(msgid ""
msgstr ""
"Language: en_US\n"

msgctxt "ui:escaped"
msgid "Escaped"
msgstr "Quotes: \"Hello\"\nTab:\tDone\nPath: C:\\Game"
)";
        fs::path po_path = create_temp_file(temp_dir, "escaped.po", content);
        auto loc = *ResourceLocation::parse("test:locales/escaped.po");
        AssetFileMap files;
        files.emplace(loc, po_path.string());
        asset_manager.merge_source(std::make_shared<MockPoAssetSource>(
            "MockEscaped", std::move(files)));

        auto result = loader.load(loc);
        REQUIRE(result.has_value());
        REQUIRE(result->entries.size() == 1);
        CHECK(result->entries[0].msgstr ==
              "Quotes: \"Hello\"\nTab:\tDone\nPath: C:\\Game");
    }
}
