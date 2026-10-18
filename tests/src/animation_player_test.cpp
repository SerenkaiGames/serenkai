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

    REQUIRE(player.play(data, clip_loc, true));
    CHECK(player.playing());
    CHECK_FALSE(player.finished());
    CHECK(player.state() == PlayState::Playing);
    CHECK(player.frame() != nullptr);
    CHECK(player.image() != nullptr);
    CHECK(player.data() != nullptr);

    player.stop();
    CHECK_FALSE(player.playing());
    CHECK_FALSE(player.finished());
    CHECK(player.state() == PlayState::Stopped);
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
        CHECK_FALSE(player.play(data, invalid_clip, true));
        CHECK_FALSE(player.playing());
        CHECK(player.state() == PlayState::Stopped);
    }

    SECTION("Replaying same clip preserves playback progress") {
        auto clip_loc = *ResourceLocation::parse("test:anim/forward");
        REQUIRE(player.play(data, clip_loc, true));
        player.update(0.15f);
        const auto* frame_before = player.frame();

        REQUIRE(player.play(data, clip_loc, true));
        CHECK(player.frame() == frame_before);
    }
}

TEST_CASE("AnimationPlayer forward playback and looping",
          "[game][animation_player]") {
    AnimationPlayer player;
    auto data = create_test_animation_data();
    auto clip_loc = *ResourceLocation::parse("test:anim/forward");

    REQUIRE(player.play(data, clip_loc, true));
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

    REQUIRE(player.play(data, clip_loc, true));
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

    REQUIRE(player.play(data, clip_loc, true));
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

    REQUIRE(player.play(data, clip_loc, true));
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

    SECTION("Single-frame animation with loop=true stays at frame") {
        auto clip_loc = *ResourceLocation::parse("test:anim/single_frame");
        REQUIRE(player.play(data, clip_loc, true));
        CHECK(player.frame() == &data.frames[1]);
        CHECK(player.state() == PlayState::Playing);

        player.update(1.0f);
        CHECK(player.frame() == &data.frames[1]);
        CHECK(player.state() == PlayState::Playing);
    }

    SECTION(
        "Single-frame animation with loop=false completes and holds frame") {
        auto clip_loc = *ResourceLocation::parse("test:anim/single_frame");
        REQUIRE(player.play(data, clip_loc, false));
        CHECK(player.frame() == &data.frames[1]);
        CHECK(player.state() == PlayState::Playing);

        // Before duration expires
        player.update(0.05f);
        CHECK(player.frame() == &data.frames[1]);
        CHECK(player.state() == PlayState::Playing);

        // Cross duration (0.1f)
        player.update(0.06f);
        CHECK(player.finished());
        CHECK_FALSE(player.playing());
        CHECK(player.state() == PlayState::Completed);
        CHECK(player.frame() == &data.frames[1]);
    }

    SECTION("Non-positive dt does not advance") {
        auto clip_loc = *ResourceLocation::parse("test:anim/forward");
        REQUIRE(player.play(data, clip_loc, true));
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

        REQUIRE(player.play(zero_data,
                            *ResourceLocation::parse("test:anim/zero"), true));
        // Must return in bounded time without hanging
        player.update(0.1f);
        CHECK(player.playing());
    }
}

TEST_CASE("AnimationPlayer one-shot completion and state tracking",
          "[game][animation_player]") {
    AnimationPlayer player;
    auto data = create_test_animation_data();

    SECTION("Forward one-shot completes and holds final frame") {
        auto clip_loc = *ResourceLocation::parse("test:anim/forward");
        REQUIRE(player.play(data, clip_loc, false));
        CHECK(player.state() == PlayState::Playing);

        // Advance frames 0 -> 1 -> 2 -> 3
        player.update(0.31f);
        CHECK(player.frame() == &data.frames[3]);
        CHECK(player.state() == PlayState::Playing);

        // Cross end of frame 3
        player.update(0.1f);
        CHECK(player.finished());
        CHECK_FALSE(player.playing());
        CHECK(player.state() == PlayState::Completed);
        CHECK(player.frame() == &data.frames[3]);

        // Further update does not change state or frame
        player.update(1.0f);
        CHECK(player.finished());
        CHECK(player.frame() == &data.frames[3]);

        // Replaying same clip restarts playback
        REQUIRE(player.play(data, clip_loc, false));
        CHECK(player.state() == PlayState::Playing);
        CHECK_FALSE(player.finished());
        CHECK(player.frame() == &data.frames[0]);
    }

    SECTION("Reverse one-shot completes and holds final frame") {
        auto clip_loc = *ResourceLocation::parse("test:anim/reverse");
        REQUIRE(player.play(data, clip_loc, false));
        CHECK(player.frame() == &data.frames[3]);

        // Advance 3 -> 2 -> 1 -> 0
        player.update(0.31f);
        CHECK(player.frame() == &data.frames[0]);
        CHECK(player.state() == PlayState::Playing);

        // Cross end of frame 0
        player.update(0.1f);
        CHECK(player.finished());
        CHECK(player.state() == PlayState::Completed);
        CHECK(player.frame() == &data.frames[0]);
    }

    SECTION("Pingpong one-shot completes after return pass") {
        auto clip_loc = *ResourceLocation::parse("test:anim/pingpong");
        REQUIRE(player.play(data, clip_loc, false));
        // Frames: 0 -> 1 -> 2 -> 1 -> 0 -> Completed holding 0
        CHECK(player.frame() == &data.frames[0]);

        player.update(0.1f);
        CHECK(player.frame() == &data.frames[1]);

        player.update(0.1f);
        CHECK(player.frame() == &data.frames[2]);

        player.update(0.1f);
        CHECK(player.frame() == &data.frames[1]);

        player.update(0.1f);
        CHECK(player.frame() == &data.frames[0]);
        CHECK(player.state() == PlayState::Playing);

        // Return pass completes
        player.update(0.1f);
        CHECK(player.finished());
        CHECK(player.state() == PlayState::Completed);
        CHECK(player.frame() == &data.frames[0]);
    }
}
