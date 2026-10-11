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

    SECTION("Default zoom is 3.0") {
        CHECK(camera.zoom() == Catch::Approx(3.0F));
    }

    SECTION("set_pos updates camera position") {
        camera.set_pos({100.0F, 200.0F});
        auto pos = camera.pos();
        CHECK(pos.x == Catch::Approx(100.0F));
        CHECK(pos.y == Catch::Approx(200.0F));
    }

    SECTION("set_zoom updates camera zoom and validates boundaries") {
        camera.set_zoom(2.5F);
        CHECK(camera.zoom() == Catch::Approx(2.5F));

        camera.set_zoom(1.0F);
        CHECK(camera.zoom() == Catch::Approx(1.0F));

        camera.set_zoom(Camera::MAX_ZOOM);
        CHECK(camera.zoom() == Catch::Approx(Camera::MAX_ZOOM));

        // Values strictly outside [1.0, MAX_ZOOM] are ignored
        camera.set_zoom(0.5F);
        CHECK(camera.zoom() == Catch::Approx(Camera::MAX_ZOOM));

        camera.set_zoom(Camera::MAX_ZOOM + 1.0F);
        CHECK(camera.zoom() == Catch::Approx(Camera::MAX_ZOOM));
    }

    SECTION("add_zoom adjusts zoom and clamps to [1.0, MAX_ZOOM]") {
        camera.set_zoom(3.0F);
        camera.add_zoom(2.0F);
        CHECK(camera.zoom() == Catch::Approx(5.0F));

        camera.add_zoom(-1.5F);
        CHECK(camera.zoom() == Catch::Approx(3.5F));

        // Clamps to MAX_ZOOM
        camera.add_zoom(100.0F);
        CHECK(camera.zoom() == Catch::Approx(Camera::MAX_ZOOM));

        // Clamps to 1.0F
        camera.add_zoom(-100.0F);
        CHECK(camera.zoom() == Catch::Approx(1.0F));
    }

    SECTION("move offsets camera position") {
        camera.set_pos({50.0F, 50.0F});
        camera.move({10.0F, -20.0F});
        auto pos = camera.pos();
        CHECK(pos.x == Catch::Approx(60.0F));
        CHECK(pos.y == Catch::Approx(30.0F));
    }
}
