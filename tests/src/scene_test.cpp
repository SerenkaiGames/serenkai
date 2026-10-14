#include "serenkai/application/app_context.hpp"
#include "serenkai/application/event.hpp"
#include "serenkai/base/raii.hpp"
#include "serenkai/resource/asset_manager.hpp"
#include "serenkai/resource/asset_source.hpp"
#include "serenkai/scenes/game_scene.hpp"
#include "serenkai/scenes/scene.hpp"
#include "serenkai/scenes/scene_manager.hpp"
#include "serenkai/scenes/title_scene.hpp"
#include "serenkai/script/script.hpp"
#include "serenkai/script/script_engine.hpp"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <filesystem>
#include <fstream>
#include <memory>
#include <utility>

namespace fs = std::filesystem;
using namespace serenkai;

namespace {

/// @brief Mock AssetSource for in-memory scene asset mapping.
class MockSceneAssetSource : public AssetSource {
public:
    explicit MockSceneAssetSource(AssetFileMap files)
        : m_files(std::move(files)) {}

    AssetFileMap& get_asset_files() override { return m_files; }
    std::string source_name() const override { return "MockSceneAssetSource"; }

private:
    AssetFileMap m_files;
};

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

    void on_enter(AppContext*) override {
        if (m_stats) {
            ++m_stats->enter_count;
        }
    }

    void on_leave(AppContext*) override {
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

    void render(RenderContext*) override {
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
    TestSceneManager() : SceneManager(nullptr) {}
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
    DefaultFactorySceneManager() : SceneManager(nullptr) {}
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

TEST_CASE("SceneManager render propagation to active scene", "[scene]") {
    TestSceneManager manager;
    RenderContext render_ctx{};

    SECTION("Render on empty manager is safe no-op") {
        manager.render(&render_ctx);
        manager.render(nullptr);
        CHECK(manager.empty());
    }

    SECTION("Render forwards to current scene") {
        manager.request_push(SceneType::Title);
        manager.update(0.016F);

        CHECK(manager.title_stats->render_count == 0);
        manager.render(&render_ctx);
        CHECK(manager.title_stats->render_count == 1);
        manager.render(nullptr);
        CHECK(manager.title_stats->render_count == 2);
    }
}

TEST_CASE("GameScene lifecycle and safety", "[scene]") {
    GameScene scene;

    SECTION("on_enter throws on null AppContext") {
        CHECK_THROWS_AS(scene.on_enter(nullptr), std::runtime_error);
    }

    SECTION("update and render without on_enter do not crash") {
        scene.update(0.016F);
        scene.render(nullptr);
        RenderContext ctx{};
        scene.render(&ctx);
    }
}

TEST_CASE("GameScene script lifecycle hooks and event dispatch", "[scene]") {
    AssetManager asset_manager;

    auto temp_dir =
        fs::temp_directory_path() / "serenkai_test_game_scene_script";
    fs::create_directories(temp_dir);
    RaiiGuard cleanup_guard([]() {},
                            [&temp_dir]() {
                                std::error_code ec;
                                fs::remove_all(temp_dir, ec);
                            });

    // Provide mock event dispatcher module
    fs::path event_path = temp_dir / "event.luau";
    {
        std::ofstream out(event_path);
        out << R"(
            local Event = {}
            local listeners = {}

            function Event.on(name, fn)
                if not listeners[name] then
                    listeners[name] = {}
                end
                table.insert(listeners[name], fn)
            end

            function Event.emit(name, ...)
                local list = listeners[name]
                if not list then return end
                for _, fn in list do
                    fn(...)
                end
            end

            return Event
        )";
    }

    // Provide mock game scene script targeting lifecycle events
    fs::path game_scene_path = temp_dir / "game_scene.luau";
    {
        std::ofstream out(game_scene_path);
        out << R"(
            local Event = require("serenkai:scripts/event.luau")

            function on_enter(mgr, cam)
                Event.emit("serenkai:events/enter_game")
            end

            function on_update(dt)
                Event.emit("serenkai:events/update_game")
            end

            function on_leave()
                Event.emit("serenkai:events/leave_game")
            end
        )";
    }

    // Register a listener script that listens to game_scene events
    fs::path listener_path = temp_dir / "listener.luau";
    {
        std::ofstream out(listener_path);
        out << R"(
            local Event = require("serenkai:scripts/event.luau")
            local enter_count = 0
            local update_count = 0
            local leave_count = 0

            Event.on("serenkai:events/enter_game", function()
                enter_count += 1
            end)

            Event.on("serenkai:events/update_game", function()
                update_count += 1
            end)

            Event.on("serenkai:events/leave_game", function()
                leave_count += 1
            end)

            function get_counts(): (number, number, number)
                return enter_count, update_count, leave_count
            end
        )";
    }

    AssetFileMap files;
    files.emplace(*ResourceLocation::parse("serenkai:scripts/event.luau"),
                  event_path.string());
    files.emplace(*ResourceLocation::parse("serenkai:scripts/game_scene.luau"),
                  game_scene_path.string());
    files.emplace(*ResourceLocation::parse("test:game_scene_listener.luau"),
                  listener_path.string());
    asset_manager.merge_source(
        std::make_shared<MockSceneAssetSource>(std::move(files)));

    ScriptEngine script_engine(
        ScriptEngineConfig{.asset_manager = &asset_manager});
    auto listener_script =
        script_engine.create("test:game_scene_listener.luau");
    REQUIRE(listener_script.has_value());

    AppContext ctx{};
    ctx.assets = &asset_manager;
    ctx.script = &script_engine;

    {
        GameScene scene;
        scene.on_enter(&ctx);
        scene.update(0.016F);
        scene.on_leave(&ctx);
    }

    auto counts = listener_script->call<int, int, int>("get_counts");
    REQUIRE(counts.has_value());
    CHECK(std::get<0>(*counts) == 1);
    CHECK(std::get<1>(*counts) == 1);
    CHECK(std::get<2>(*counts) == 1);
}

TEST_CASE("TitleScene lifecycle and safety", "[scene]") {
    TitleScene scene;

    SECTION("on_enter throws on null AppContext") {
        CHECK_THROWS_AS(scene.on_enter(nullptr), std::runtime_error);
    }

    SECTION("render with null RenderContext does not crash") {
        scene.render(nullptr);
    }
}
