#include "serenkai/application/event.hpp"
#include "serenkai/scenes/game_scene.hpp"
#include "serenkai/scenes/scene.hpp"
#include "serenkai/scenes/scene_manager.hpp"
#include "serenkai/scenes/title_scene.hpp"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <memory>
#include <utility>

using namespace serenkai;

namespace {

/// @brief Mock scene that tracks lifecycle calls for testing.
class MockScene : public Scene {
public:
    struct Stats {
        int enter_count{0};
        int leave_count{0};
        int update_count{0};
        int render_count{0};
        float last_dt{0.0F};
        bool is_destroyed{false};
    };

    explicit MockScene(std::shared_ptr<Stats> stats)
        : m_stats(std::move(stats)) {}

    ~MockScene() override {
        if (m_stats) {
            m_stats->is_destroyed = true;
        }
    }

    MockScene(const MockScene&) = delete;
    MockScene(MockScene&&) = delete;
    MockScene& operator=(const MockScene&) = delete;
    MockScene& operator=(MockScene&&) = delete;

    void on_enter() override {
        if (m_stats) {
            ++m_stats->enter_count;
        }
    }

    void on_leave() override {
        if (m_stats) {
            ++m_stats->leave_count;
        }
    }

    void update(float dt) override {
        if (m_stats) {
            ++m_stats->update_count;
            m_stats->last_dt = dt;
        }
    }

    void render(GuiContext*) override {
        if (m_stats) {
            ++m_stats->render_count;
        }
    }

    bool handle_event(const Event&) override { return true; }

    [[nodiscard]] std::shared_ptr<Stats> stats() const { return m_stats; }

private:
    std::shared_ptr<Stats> m_stats;
};

/// @brief Testable SceneManager that produces MockScene instances.
class TestSceneManager : public SceneManager {
public:
    std::shared_ptr<MockScene::Stats> title_stats =
        std::make_shared<MockScene::Stats>();
    std::shared_ptr<MockScene::Stats> game_stats =
        std::make_shared<MockScene::Stats>();

protected:
    std::unique_ptr<Scene> create_scene(SceneType type) override {
        switch (type) {
        case SceneType::Title:
            return std::make_unique<MockScene>(title_stats);
        case SceneType::Game:
            return std::make_unique<MockScene>(game_stats);
        }
        return nullptr;
    }
};

/// @brief Exposes create_scene to test the default scene factory.
class DefaultFactorySceneManager : public SceneManager {
public:
    using SceneManager::create_scene;
};

} // namespace

TEST_CASE("SceneManager initial state and empty stack safety", "[scene]") {
    TestSceneManager manager;

    CHECK(manager.empty());
    CHECK(manager.size() == 0);
    CHECK(manager.current_scene() == nullptr);

    SECTION("Update on empty scene manager does not crash") {
        manager.update(0.016F);
        CHECK(manager.empty());
    }

    SECTION("Pop on empty scene manager is a safe no-op") {
        manager.request_pop();
        manager.update(0.016F);
        CHECK(manager.empty());
        CHECK(manager.size() == 0);
    }
}

TEST_CASE("SceneManager push operation and lifecycle", "[scene]") {
    TestSceneManager manager;

    SECTION("request_push is deferred until update is called") {
        manager.request_push(SceneType::Title);

        CHECK(manager.empty());
        CHECK(manager.size() == 0);
        CHECK(manager.title_stats->enter_count == 0);

        manager.update(0.016F);

        CHECK_FALSE(manager.empty());
        CHECK(manager.size() == 1);
        CHECK(manager.current_scene() != nullptr);
        CHECK(manager.title_stats->enter_count == 1);
        CHECK(manager.title_stats->update_count == 1);
        CHECK(manager.title_stats->last_dt == Catch::Approx(0.016F));
    }
}

TEST_CASE("SceneManager pop operation and deferred destruction", "[scene]") {
    TestSceneManager manager;

    manager.request_push(SceneType::Title);
    manager.update(0.016F);
    REQUIRE(manager.size() == 1);

    SECTION("request_pop is deferred and keeps scene alive during pop frame") {
        manager.request_pop();

        CHECK(manager.size() == 1);

        manager.update(0.016F);

        CHECK(manager.empty());
        CHECK(manager.size() == 0);
        CHECK(manager.current_scene() == nullptr);
        CHECK(manager.title_stats->leave_count == 1);
        // The popped scene is kept alive in m_pending_delete_scene until the
        // next update.
        CHECK_FALSE(manager.title_stats->is_destroyed);

        // Next frame clears pending deleted scenes.
        manager.update(0.016F);
        CHECK(manager.title_stats->is_destroyed);
    }
}

TEST_CASE("SceneManager change operation transitions between scenes",
          "[scene]") {
    TestSceneManager manager;

    manager.request_push(SceneType::Title);
    manager.update(0.016F);
    REQUIRE(manager.size() == 1);

    manager.request_change(SceneType::Game);

    CHECK(manager.size() == 1);
    CHECK(manager.title_stats->leave_count == 0);
    CHECK(manager.game_stats->enter_count == 0);

    manager.update(0.016F);

    CHECK(manager.size() == 1);
    CHECK(manager.title_stats->leave_count == 1);
    CHECK(manager.game_stats->enter_count == 1);
    CHECK(manager.game_stats->update_count == 1);
    CHECK(manager.current_scene() != nullptr);
}

TEST_CASE("SceneManager operation conflict prevention", "[scene]") {
    TestSceneManager manager;

    SECTION("Subsequent request in the same frame is rejected") {
        manager.request_push(SceneType::Title);
        // Second operation should be rejected and not overwrite the first.
        manager.request_pop();

        manager.update(0.016F);

        CHECK(manager.size() == 1);
        CHECK(manager.title_stats->enter_count == 1);
    }
}

TEST_CASE("SceneManager default factory creates concrete scenes", "[scene]") {
    DefaultFactorySceneManager manager;

    auto title_scene = manager.create_scene(SceneType::Title);
    REQUIRE(title_scene != nullptr);
    CHECK(dynamic_cast<TitleScene*>(title_scene.get()) != nullptr);

    auto game_scene = manager.create_scene(SceneType::Game);
    REQUIRE(game_scene != nullptr);
    CHECK(dynamic_cast<GameScene*>(game_scene.get()) != nullptr);
}
