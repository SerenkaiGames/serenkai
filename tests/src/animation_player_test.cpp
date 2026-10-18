#include "serenkai/game/animation_player.hpp"
#include "serenkai/resource/animation_data.hpp"
#include "serenkai/resource/resource_location.hpp"

#include <catch2/catch_test_macros.hpp>
#include <vector>

using namespace serenkai;

namespace {

/// @brief Helper to construct sample animation data for tests without loading
/// files.
SpriteAnimationData create_test_animation_data() {
    SpriteAnimationData data{};
    data.image = *ResourceLocation::parse("test:textures/sheet.png");

    for (int i = 0; i < 4; ++i) {
        SpriteFrame f{};
        f.src = glm::vec4(static_cast<float>(i * 16), 0.0f, 16.0f, 16.0f);
        f.offset = glm::vec2(0.0f);
        f.source_size = glm::vec2(16.0f);
        f.duration = 0.1f;
        data.frames.push_back(f);
    }

    data.clips[*ResourceLocation::parse("test:anim/forward")] = AnimationClip{
        .name = "anim/forward",
        .from = 0,
        .to = 3,
        .mode = PlaybackMode::Forward,
    };

    data.clips[*ResourceLocation::parse("test:anim/reverse")] = AnimationClip{
        .name = "anim/reverse",
        .from = 0,
        .to = 3,
        .mode = PlaybackMode::Reverse,
    };

    data.clips[*ResourceLocation::parse("test:anim/pingpong")] = AnimationClip{
        .name = "anim/pingpong",
        .from = 0,
        .to = 2,
        .mode = PlaybackMode::Pingpong,
    };

    const auto pingpong_rev = AnimationClip{
        .name = "anim/pingpong_reverse",
        .from = 0,
        .to = 2,
        .mode = PlaybackMode::PingpongReverse,
    };
    data.clips[*ResourceLocation::parse("test:anim/pingpong_reverse")] =
        pingpong_rev;

    const auto single_frame = AnimationClip{
        .name = "anim/single_frame",
        .from = 1,
        .to = 1,
        .mode = PlaybackMode::Forward,
    };
    data.clips[*ResourceLocation::parse("test:anim/single_frame")] =
        single_frame;

    return data;
}

} // namespace

TEST_CASE("AnimationPlayer initial and stop state",
          "[game][animation_player]") {
    AnimationPlayer player;

    CHECK_FALSE(player.playing());
    CHECK(player.frame() == nullptr);
    CHECK(player.image() == nullptr);
    CHECK(player.data() == nullptr);

    auto data = create_test_animation_data();
    auto clip_loc = *ResourceLocation::parse("test:anim/forward");

    REQUIRE(player.play(data, clip_loc));
    CHECK(player.playing());
    CHECK(player.frame() != nullptr);
    CHECK(player.image() != nullptr);
    CHECK(player.data() != nullptr);

    player.stop();
    CHECK_FALSE(player.playing());
    CHECK(player.frame() == nullptr);
    CHECK(player.image() == nullptr);
    CHECK(player.data() == nullptr);
}

TEST_CASE("AnimationPlayer clip selection and matching",
          "[game][animation_player]") {
    AnimationPlayer player;
    auto data = create_test_animation_data();

    SECTION("Non-existent clip returns false") {
        auto invalid_clip = *ResourceLocation::parse("test:anim/missing");
        CHECK_FALSE(player.play(data, invalid_clip));
        CHECK_FALSE(player.playing());
    }

    SECTION("Replaying same clip preserves playback progress") {
        auto clip_loc = *ResourceLocation::parse("test:anim/forward");
        REQUIRE(player.play(data, clip_loc));
        player.update(0.15f);
        const auto* frame_before = player.frame();

        REQUIRE(player.play(data, clip_loc));
        CHECK(player.frame() == frame_before);
    }
}

TEST_CASE("AnimationPlayer forward playback and looping",
          "[game][animation_player]") {
    AnimationPlayer player;
    auto data = create_test_animation_data();
    auto clip_loc = *ResourceLocation::parse("test:anim/forward");

    REQUIRE(player.play(data, clip_loc));
    CHECK(player.frame() == &data.frames[0]);

    // Partial elapsed time does not advance frame
    player.update(0.05f);
    CHECK(player.frame() == &data.frames[0]);

    // Cross duration threshold to frame 1
    player.update(0.06f);
    CHECK(player.frame() == &data.frames[1]);

    // Advance to frame 2 and frame 3
    player.update(0.1f);
    CHECK(player.frame() == &data.frames[2]);

    player.update(0.1f);
    CHECK(player.frame() == &data.frames[3]);

    // Loop back to frame 0
    player.update(0.1f);
    CHECK(player.frame() == &data.frames[0]);
}

TEST_CASE("AnimationPlayer reverse playback", "[game][animation_player]") {
    AnimationPlayer player;
    auto data = create_test_animation_data();
    auto clip_loc = *ResourceLocation::parse("test:anim/reverse");

    REQUIRE(player.play(data, clip_loc));
    // Starts at 'to' frame (frame 3)
    CHECK(player.frame() == &data.frames[3]);

    player.update(0.1f);
    CHECK(player.frame() == &data.frames[2]);

    player.update(0.1f);
    CHECK(player.frame() == &data.frames[1]);

    player.update(0.1f);
    CHECK(player.frame() == &data.frames[0]);

    // Loop back to 'to' frame (frame 3)
    player.update(0.1f);
    CHECK(player.frame() == &data.frames[3]);
}

TEST_CASE("AnimationPlayer pingpong playback", "[game][animation_player]") {
    AnimationPlayer player;
    auto data = create_test_animation_data();
    auto clip_loc = *ResourceLocation::parse("test:anim/pingpong");

    REQUIRE(player.play(data, clip_loc));
    // Frames sequence: 0 -> 1 -> 2 -> 1 -> 0 -> 1 -> 2 ...
    CHECK(player.frame() == &data.frames[0]);

    player.update(0.1f);
    CHECK(player.frame() == &data.frames[1]);

    player.update(0.1f);
    CHECK(player.frame() == &data.frames[2]);

    // Bounce back
    player.update(0.1f);
    CHECK(player.frame() == &data.frames[1]);

    player.update(0.1f);
    CHECK(player.frame() == &data.frames[0]);

    // Bounce forward
    player.update(0.1f);
    CHECK(player.frame() == &data.frames[1]);

    player.update(0.1f);
    CHECK(player.frame() == &data.frames[2]);
}

TEST_CASE("AnimationPlayer pingpong reverse playback",
          "[game][animation_player]") {
    AnimationPlayer player;
    auto data = create_test_animation_data();
    auto clip_loc = *ResourceLocation::parse("test:anim/pingpong_reverse");

    REQUIRE(player.play(data, clip_loc));
    // Frames sequence: 2 -> 1 -> 0 -> 1 -> 2 -> 1 -> 0 ...
    CHECK(player.frame() == &data.frames[2]);

    player.update(0.1f);
    CHECK(player.frame() == &data.frames[1]);

    player.update(0.1f);
    CHECK(player.frame() == &data.frames[0]);

    // Bounce forward
    player.update(0.1f);
    CHECK(player.frame() == &data.frames[1]);

    player.update(0.1f);
    CHECK(player.frame() == &data.frames[2]);

    // Bounce back
    player.update(0.1f);
    CHECK(player.frame() == &data.frames[1]);

    player.update(0.1f);
    CHECK(player.frame() == &data.frames[0]);
}

TEST_CASE("AnimationPlayer edge cases and protection",
          "[game][animation_player]") {
    AnimationPlayer player;
    auto data = create_test_animation_data();

    SECTION("Single-frame animation does not advance") {
        auto clip_loc = *ResourceLocation::parse("test:anim/single_frame");
        REQUIRE(player.play(data, clip_loc));
        CHECK(player.frame() == &data.frames[1]);

        player.update(1.0f);
        CHECK(player.frame() == &data.frames[1]);
    }

    SECTION("Non-positive dt does not advance") {
        auto clip_loc = *ResourceLocation::parse("test:anim/forward");
        REQUIRE(player.play(data, clip_loc));
        CHECK(player.frame() == &data.frames[0]);

        player.update(0.0f);
        CHECK(player.frame() == &data.frames[0]);

        player.update(-0.5f);
        CHECK(player.frame() == &data.frames[0]);
    }

    SECTION("Zero duration frame is capped and does not loop infinitely") {
        SpriteAnimationData zero_data{};
        zero_data.image = *ResourceLocation::parse("test:textures/sheet.png");
        zero_data.frames.push_back(SpriteFrame{.duration = 0.0f});
        zero_data.frames.push_back(SpriteFrame{.duration = 0.0f});

        const auto zero_clip = AnimationClip{
            .name = "anim/zero",
            .from = 0,
            .to = 1,
            .mode = PlaybackMode::Forward,
        };
        zero_data.clips[*ResourceLocation::parse("test:anim/zero")] = zero_clip;

        REQUIRE(
            player.play(zero_data, *ResourceLocation::parse("test:anim/zero")));
        // Must return in bounded time without hanging
        player.update(0.1f);
        CHECK(player.playing());
    }
}
