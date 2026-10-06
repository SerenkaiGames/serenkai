#include "serenkai/base/raii.hpp"
#include "serenkai/resource/asset_manager.hpp"
#include "serenkai/resource/asset_source.hpp"
#include "serenkai/resource/image_loader.hpp"
#include "serenkai/resource/resource_location.hpp"

#include <catch2/catch_test_macros.hpp>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <memory>
#include <stb_image_write.h>
#include <string>
#include <vector>

namespace fs = std::filesystem;
using namespace serenkai;

namespace {

/// @brief Mock AssetSource for registering file paths in AssetManager.
class MockImageSource : public AssetSource {
public:
    explicit MockImageSource(std::string name, AssetFileMap files = {})
        : m_name(std::move(name)), m_files(std::move(files)) {}

    AssetFileMap& get_asset_files() override { return m_files; }
    std::string source_name() const override { return m_name; }

private:
    std::string m_name;
    AssetFileMap m_files;
};

} // namespace

TEST_CASE("ImageLoader error handling and edge cases",
          "[resource][image_loader]") {
    SECTION("Non-existent asset returns empty image wrapper") {
        AssetManager asset_manager;
        ImageLoader loader(&asset_manager);
        auto img = loader.load("serenkai:textures/missing.png");
        CHECK(img->data == nullptr);
        CHECK(img->width == 0);
        CHECK(img->height == 0);
    }

    SECTION("Invalid resource location returns empty image wrapper") {
        AssetManager asset_manager;
        ImageLoader loader(&asset_manager);
        auto img = loader.load(":invalid");
        CHECK(img->data == nullptr);
    }

    SECTION("Corrupted or non-image file returns empty image wrapper") {
        fs::path temp_dir =
            fs::temp_directory_path() / "serenkai_test_image_loader_corrupt";
        fs::create_directories(temp_dir);
        RaiiGuard cleanup_guard([]() {},
                                [&temp_dir]() {
                                    std::error_code ec;
                                    fs::remove_all(temp_dir, ec);
                                });

        fs::path bad_file = temp_dir / "bad.png";
        {
            std::ofstream out(bad_file);
            out << "This is not a valid PNG or image file.";
        }

        AssetManager asset_manager;
        auto loc = *ResourceLocation::parse("test:bad.png");
        AssetFileMap files;
        files.emplace(loc, bad_file.string());
        asset_manager.merge_source(std::make_shared<MockImageSource>(
            "MockBadSource", std::move(files)));

        ImageLoader loader(&asset_manager);
        auto img = loader.load("test:bad.png");
        CHECK(img->data == nullptr);
    }
}

TEST_CASE("ImageLoader loads valid RGBA image", "[resource][image_loader]") {
    fs::path temp_dir =
        fs::temp_directory_path() / "serenkai_test_image_loader_valid";
    fs::create_directories(temp_dir);
    RaiiGuard cleanup_guard([]() {},
                            [&temp_dir]() {
                                std::error_code ec;
                                fs::remove_all(temp_dir, ec);
                            });

    // Create a 2x2 RGBA PNG image:
    // (0,0): Red (255, 0, 0, 255)
    // (1,0): Green (0, 255, 0, 255)
    // (0,1): Blue (0, 0, 255, 255)
    // (1,1): White (255, 255, 255, 255)
    const int width = 2;
    const int height = 2;
    const int channels = 4;
    const std::vector<uint8_t> pixels = {
        255, 0,   0,   255, // Row 0, Col 0: Red
        0,   255, 0,   255, // Row 0, Col 1: Green
        0,   0,   255, 255, // Row 1, Col 0: Blue
        255, 255, 255, 255  // Row 1, Col 1: White
    };

    fs::path png_path = temp_dir / "valid.png";
    int write_res = stbi_write_png(png_path.string().c_str(), width, height,
                                   channels, pixels.data(), width * channels);
    REQUIRE(write_res != 0);

    AssetManager asset_manager;
    auto loc = *ResourceLocation::parse("test:textures/valid.png");
    AssetFileMap files;
    files.emplace(loc, png_path.string());
    asset_manager.merge_source(
        std::make_shared<MockImageSource>("MockValidSource", std::move(files)));

    ImageLoader loader(&asset_manager);
    auto img = loader.load("test:textures/valid.png");

    REQUIRE(img->data != nullptr);
    CHECK(img->width == 2);
    CHECK(img->height == 2);
    CHECK(img->channels == 4);

    // Verify pixel bytes in RGBA order (row-major, top-to-bottom)
    const uint8_t* data = img->data;

    // (0,0): Red
    CHECK(data[0] == 255);
    CHECK(data[1] == 0);
    CHECK(data[2] == 0);
    CHECK(data[3] == 255);

    // (1,0): Green
    CHECK(data[4] == 0);
    CHECK(data[5] == 255);
    CHECK(data[6] == 0);
    CHECK(data[7] == 255);

    // (0,1): Blue
    CHECK(data[8] == 0);
    CHECK(data[9] == 0);
    CHECK(data[10] == 255);
    CHECK(data[11] == 255);

    // (1,1): White
    CHECK(data[12] == 255);
    CHECK(data[13] == 255);
    CHECK(data[14] == 255);
    CHECK(data[15] == 255);

    SECTION("ImageLoader load with ResourceLocation overload") {
        auto img_rl = loader.load(loc);
        REQUIRE(img_rl->data != nullptr);
        CHECK(img_rl->width == 2);
        CHECK(img_rl->height == 2);
        CHECK(img_rl->channels == 4);

        auto img_lit = loader.load("test:textures/valid.png"_rl);
        REQUIRE(img_lit->data != nullptr);
        CHECK(img_lit->width == 2);
    }

    SECTION("ImageWrapper move semantics") {
        ImageWrapper moved = std::move(img);
        CHECK(static_cast<bool>(moved));
        CHECK(moved->data != nullptr);
        CHECK(moved->width == 2);
        CHECK(moved->height == 2);
        CHECK_FALSE(static_cast<bool>(img));
    }
}
