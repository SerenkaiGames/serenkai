#include "serenkai/base/aabb.hpp"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

using namespace serenkai;

TEST_CASE("AABB construction and normalization", "[base][aabb]") {
    SECTION("Default constructor initializes to zero") {
        AABB box;
        CHECK(box.position.x == 0.0f);
        CHECK(box.position.y == 0.0f);
        CHECK(box.size.x == 0.0f);
        CHECK(box.size.y == 0.0f);
        CHECK(box.is_empty());
    }

    SECTION("Component constructor with positive dimensions") {
        AABB box(10.0f, 20.0f, 30.0f, 40.0f);
        CHECK(box.left() == 10.0f);
        CHECK(box.top() == 20.0f);
        CHECK(box.width() == 30.0f);
        CHECK(box.height() == 40.0f);
        CHECK_FALSE(box.is_empty());
    }

    SECTION("glm::vec2 constructor") {
        glm::vec2 pos{5.0f, 15.0f};
        glm::vec2 size{25.0f, 35.0f};
        AABB box(pos, size);
        CHECK(box.position == pos);
        CHECK(box.size == size);
    }

    SECTION("Explicit glm::vec4 constructor") {
        glm::vec4 vec{1.0f, 2.0f, 3.0f, 4.0f};
        AABB box(vec);
        CHECK(box.left() == 1.0f);
        CHECK(box.top() == 2.0f);
        CHECK(box.width() == 3.0f);
        CHECK(box.height() == 4.0f);
    }

    SECTION("Factory from_center") {
        auto box =
            AABB::from_center(glm::vec2{50.0f, 50.0f}, glm::vec2{20.0f, 10.0f});
        CHECK(box.left() == 40.0f);
        CHECK(box.top() == 45.0f);
        CHECK(box.width() == 20.0f);
        CHECK(box.height() == 10.0f);
        CHECK(box.center().x == 50.0f);
        CHECK(box.center().y == 50.0f);
    }

    SECTION("Factory from_min_max") {
        auto box = AABB::from_min_max(glm::vec2{10.0f, 20.0f},
                                      glm::vec2{30.0f, 50.0f});
        CHECK(box.left() == 10.0f);
        CHECK(box.top() == 20.0f);
        CHECK(box.width() == 20.0f);
        CHECK(box.height() == 30.0f);
    }

    SECTION("Negative dimensions are normalized by flipping position and "
            "negating size") {
        AABB box(50.0f, 60.0f, -20.0f, -30.0f);
        CHECK(box.left() == 30.0f);
        CHECK(box.top() == 30.0f);
        CHECK(box.width() == 20.0f);
        CHECK(box.height() == 30.0f);
    }
}

TEST_CASE("AABB accessors and geometry points", "[base][aabb]") {
    AABB box(10.0f, 20.0f, 30.0f, 40.0f);

    SECTION("Boundary accessors") {
        CHECK(box.left() == 10.0f);
        CHECK(box.top() == 20.0f);
        CHECK(box.right() == 40.0f);
        CHECK(box.bottom() == 60.0f);
        CHECK(box.width() == 30.0f);
        CHECK(box.height() == 40.0f);
    }

    SECTION("Corner vertices") {
        CHECK(box.top_left() == glm::vec2{10.0f, 20.0f});
        CHECK(box.top_right() == glm::vec2{40.0f, 20.0f});
        CHECK(box.bottom_left() == glm::vec2{10.0f, 60.0f});
        CHECK(box.bottom_right() == glm::vec2{40.0f, 60.0f});
    }

    SECTION("Center, min, and max") {
        CHECK(box.center() == glm::vec2{25.0f, 40.0f});
        CHECK(box.min() == glm::vec2{10.0f, 20.0f});
        CHECK(box.max() == glm::vec2{40.0f, 60.0f});
    }

    SECTION("Area and emptiness") {
        CHECK(box.area() == 1200.0f);
        CHECK_FALSE(box.is_empty());

        AABB zero_box(0.0f, 0.0f, 0.0f, 10.0f);
        CHECK(zero_box.is_empty());
    }
}

TEST_CASE("AABB containment tests", "[base][aabb]") {
    AABB box(10.0f, 20.0f, 30.0f, 40.0f);

    SECTION("Point containment inside and outside") {
        CHECK(box.contains(glm::vec2{20.0f, 30.0f}));
        CHECK_FALSE(box.contains(glm::vec2{5.0f, 30.0f}));
        CHECK_FALSE(box.contains(glm::vec2{45.0f, 30.0f}));
        CHECK_FALSE(box.contains(glm::vec2{20.0f, 15.0f}));
        CHECK_FALSE(box.contains(glm::vec2{20.0f, 65.0f}));
    }

    SECTION("Point containment on boundaries is inclusive") {
        CHECK(box.contains(glm::vec2{10.0f, 20.0f}));
        CHECK(box.contains(glm::vec2{40.0f, 20.0f}));
        CHECK(box.contains(glm::vec2{10.0f, 60.0f}));
        CHECK(box.contains(glm::vec2{40.0f, 60.0f}));
    }

    SECTION("AABB box containment") {
        AABB inner(15.0f, 25.0f, 10.0f, 10.0f);
        CHECK(box.contains(inner));

        AABB touching(10.0f, 20.0f, 30.0f, 40.0f);
        CHECK(box.contains(touching));

        AABB partial(5.0f, 25.0f, 15.0f, 10.0f);
        CHECK_FALSE(box.contains(partial));

        AABB outside(100.0f, 100.0f, 10.0f, 10.0f);
        CHECK_FALSE(box.contains(outside));
    }
}

TEST_CASE("AABB intersection and union", "[base][aabb]") {
    AABB a(0.0f, 0.0f, 20.0f, 20.0f);

    SECTION("Overlapping boxes intersect") {
        AABB b(10.0f, 10.0f, 20.0f, 20.0f);
        CHECK(a.intersects(b));
        CHECK(b.intersects(a));

        AABB isect = a.intersection(b);
        CHECK(isect.left() == 10.0f);
        CHECK(isect.top() == 10.0f);
        CHECK(isect.width() == 10.0f);
        CHECK(isect.height() == 10.0f);
    }

    SECTION("Touching edges count as intersecting") {
        AABB b(20.0f, 0.0f, 10.0f, 20.0f);
        CHECK(a.intersects(b));

        AABB isect = a.intersection(b);
        CHECK(isect.left() == 20.0f);
        CHECK(isect.top() == 0.0f);
        CHECK(isect.width() == 0.0f);
        CHECK(isect.height() == 20.0f);
    }

    SECTION("Disjoint boxes do not intersect") {
        AABB b(30.0f, 30.0f, 10.0f, 10.0f);
        CHECK_FALSE(a.intersects(b));

        AABB isect = a.intersection(b);
        CHECK(isect == AABB(0.0f, 0.0f, 0.0f, 0.0f));
    }

    SECTION("Union creates enclosing bounding box") {
        AABB b(30.0f, 40.0f, 10.0f, 10.0f);
        AABB u = a.united(b);
        CHECK(u.left() == 0.0f);
        CHECK(u.top() == 0.0f);
        CHECK(u.right() == 40.0f);
        CHECK(u.bottom() == 50.0f);
    }
}

TEST_CASE("AABB transformations", "[base][aabb]") {
    AABB box(10.0f, 20.0f, 30.0f, 40.0f);

    SECTION("In-place translation") {
        box.translate(glm::vec2{5.0f, -5.0f});
        CHECK(box.left() == 15.0f);
        CHECK(box.top() == 15.0f);
        CHECK(box.width() == 30.0f);
        CHECK(box.height() == 40.0f);
    }

    SECTION("Const translated method") {
        auto shifted = box.translated(glm::vec2{10.0f, 10.0f});
        CHECK(shifted.left() == 20.0f);
        CHECK(shifted.top() == 30.0f);
        CHECK(shifted.width() == 30.0f);
        CHECK(shifted.height() == 40.0f);
        CHECK(box.left() == 10.0f);
    }

    SECTION("Scale modifies dimensions") {
        box.scale(glm::vec2{2.0f, 0.5f});
        CHECK(box.width() == 60.0f);
        CHECK(box.height() == 20.0f);
    }

    SECTION("set_center repositions around center point") {
        box.set_center(glm::vec2{100.0f, 100.0f});
        CHECK(box.center() == glm::vec2{100.0f, 100.0f});
        CHECK(box.width() == 30.0f);
        CHECK(box.height() == 40.0f);
        CHECK(box.left() == 85.0f);
        CHECK(box.top() == 80.0f);
    }

    SECTION("Expanded expands all sides") {
        auto exp = box.expanded(5.0f);
        CHECK(exp.left() == 5.0f);
        CHECK(exp.top() == 15.0f);
        CHECK(exp.width() == 40.0f);
        CHECK(exp.height() == 50.0f);
    }
}

TEST_CASE("AABB equality operator", "[base][aabb]") {
    AABB a(10.0f, 20.0f, 30.0f, 40.0f);
    AABB b(10.0f, 20.0f, 30.0f, 40.0f);
    AABB c(11.0f, 20.0f, 30.0f, 40.0f);
    AABB d(10.0f, 20.0f, 31.0f, 40.0f);

    CHECK(a == b);
    CHECK(a != c);
    CHECK(a != d);
}
