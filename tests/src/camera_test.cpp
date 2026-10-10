#include "serenkai/game/camera.hpp"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

using namespace serenkai;

TEST_CASE("Camera default state and properties", "[game][camera]") {
    Camera camera;

    SECTION("Default position is at origin") {
        auto pos = camera.pos();
        CHECK(pos.x == Catch::Approx(0.0F));
        CHECK(pos.y == Catch::Approx(0.0F));
    }

    SECTION("Default zoom is 1.0") {
        CHECK(camera.zoom() == Catch::Approx(1.0F));
    }
}
