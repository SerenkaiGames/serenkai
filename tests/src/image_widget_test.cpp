#include "serenkai/gui/anchor.hpp"
#include "serenkai/gui/image_widget.hpp"
#include "serenkai/gui/widget.hpp"

#include <catch2/catch_test_macros.hpp>

using namespace serenkai;

TEST_CASE("ImageWidget default state and property setters",
          "[gui][image_widget]") {
    ImageWidget widget("test_image", nullptr);

    CHECK(widget.name() == "test_image");
    CHECK(widget.get_image().empty());
    CHECK(widget.size() == glm::ivec2{0, 0});
    CHECK(widget.parent() == nullptr);
    CHECK_FALSE(widget.has_parent());

    SECTION("Image location mutation") {
        widget.set_image("game:textures/player.png");
        CHECK(widget.get_image() == "game:textures/player.png");
    }

    SECTION("Explicit size mutation") {
        widget.set_size({64, 64});
        CHECK(widget.size() == glm::ivec2{64, 64});
    }

    SECTION("Anchor and offset mutation") {
        widget.set_anchor(Anchor::Center);
        widget.set_offset({10, -5});
        CHECK(widget.anchor() == Anchor::Center);
        CHECK(widget.offset() == glm::ivec2{10, -5});
    }
}

TEST_CASE("ImageWidget hierarchical tree construction", "[gui][image_widget]") {
    Widget parent("parent", nullptr);
    parent.set_size({200, 200});

    auto& child = parent.create_child<ImageWidget>("child_image");
    child.set_image("game:textures/icon.png");
    child.set_size({32, 32});

    CHECK(child.has_parent());
    CHECK(child.parent() == &parent);
    CHECK(child.get_image() == "game:textures/icon.png");

    auto* fetched = parent.fetch_child<ImageWidget>("child_image");
    REQUIRE(fetched != nullptr);
    CHECK(fetched == &child);
}

TEST_CASE("ImageWidget render safety", "[gui][image_widget]") {
    ImageWidget widget("test_image", nullptr);
    widget.set_image("game:textures/icon.png");

    SECTION("Render safety without context") { widget.render(nullptr); }

    SECTION("Invisible image skips render safely") {
        widget.set_visible(false);
        widget.render(nullptr);
    }
}
