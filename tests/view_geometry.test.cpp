#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "view/geometry.hpp"

using moba_sim::view::length;
using moba_sim::view::normalized;
using moba_sim::view::Rect;
using moba_sim::view::Vec2;

TEST_CASE("Vec2 arithmetic") {
    const Vec2 a{1.0f, 2.0f};
    const Vec2 b{3.0f, -4.0f};

    CHECK((a + b) == Vec2{4.0f, -2.0f});
    CHECK((a - b) == Vec2{-2.0f, 6.0f});
    CHECK((a * 2.0f) == Vec2{2.0f, 4.0f});
    CHECK((2.0f * a) == Vec2{2.0f, 4.0f});

    Vec2 c = a;
    c += b;
    CHECK(c == Vec2{4.0f, -2.0f});
    c *= 0.5f;
    CHECK(c == Vec2{2.0f, -1.0f});
}

TEST_CASE("Vec2 length and normalization") {
    CHECK(length(Vec2{3.0f, 4.0f}) == Catch::Approx(5.0f));
    CHECK(length(Vec2{}) == Catch::Approx(0.0f));

    const Vec2 n = normalized(Vec2{0.0f, 10.0f});
    CHECK(n.x == Catch::Approx(0.0f));
    CHECK(n.y == Catch::Approx(1.0f));

    CHECK(normalized(Vec2{}) == Vec2{});
}

TEST_CASE("Rect contains and center") {
    const Rect rect{10.0f, 20.0f, 30.0f, 40.0f};

    CHECK(rect.contains({10.0f, 20.0f}));
    CHECK(rect.contains({40.0f, 60.0f}));
    CHECK(rect.contains({25.0f, 40.0f}));
    CHECK_FALSE(rect.contains({9.0f, 40.0f}));
    CHECK_FALSE(rect.contains({25.0f, 61.0f}));

    CHECK(rect.center() == Vec2{25.0f, 40.0f});
}
