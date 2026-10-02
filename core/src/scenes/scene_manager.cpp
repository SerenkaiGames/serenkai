#include "serenkai/scenes/scene_manager.hpp"

#include "serenkai/base/unreachable.hpp"
#include "serenkai/scenes/game_scene.hpp"
#include "serenkai/scenes/title_scene.hpp"

#include <memory>
#include <spdlog/spdlog.h>

namespace serenkai {

SceneManager::SceneManager() {}
SceneManager::~SceneManager() {}

void SceneManager::update(float dt) {

    m_pending_delete_scene.clear();
    process_operation();
    if (!m_scenes.empty()) {
        m_scenes.top()->update(dt);
    }
}
void SceneManager::render(Renderer& renderer) {

    if (m_scenes.empty()) {
        return;
    }
    m_scenes.top()->render(renderer);
}

bool SceneManager::handle_event(const Event& e) {

    if (m_scenes.empty()) {
        return false;
    }
    return m_scenes.top()->handle_event(e);
}

void SceneManager::request_change(SceneType type) {
    if (m_operation.has_value()) {
        spdlog::error("Scene operation already pending");
        return;
    }
    m_operation = {OperationType::Change, type};
}
void SceneManager::request_push(SceneType type) {
    if (m_operation.has_value()) {
        spdlog::error("Scene operation already pending");
        return;
    }
    m_operation = {OperationType::Push, type};
}
void SceneManager::request_pop() {
    if (m_operation.has_value()) {
        spdlog::error("Scene operation already pending");
        return;
    }
    m_operation = {OperationType::Pop, std::nullopt};
}

void SceneManager::process_operation() {
    while (m_operation) {
        auto op = std::move(*m_operation);
        m_operation.reset();

        switch (op.type) {
        case OperationType::Push:
            push(*op.scene);
            break;
        case OperationType::Pop:
            pop();
            break;
        case OperationType::Change:
            change(*op.scene);
            break;
        }
    }
}

void SceneManager::change(SceneType type) {
    if (!m_scenes.empty()) {
        pop();
    }

    push(type);
}
void SceneManager::push(SceneType type) {
    auto scene = create_scene(type);
    scene->on_enter();
    m_scenes.push(std::move(scene));
}
void SceneManager::pop() {
    if (m_scenes.empty()) {
        return;
    }
    auto scene = std::move(m_scenes.top());
    m_scenes.pop();
    scene->on_leave();
    m_pending_delete_scene.push_back(std::move(scene));
}

std::unique_ptr<Scene> SceneManager::create_scene(SceneType type) {
    switch (type) {
    case SceneType::Title: {
        return std::make_unique<TitleScene>();
    } break;
    case SceneType::Game: {
        return std::make_unique<GameScene>();
    } break;
    }

    unreachable();
}

[[nodiscard]] std::size_t SceneManager::size() const noexcept {
    return m_scenes.size();
}
[[nodiscard]] bool SceneManager::empty() const noexcept {
    return m_scenes.empty();
}
[[nodiscard]] Scene* SceneManager::current_scene() const noexcept {
    return m_scenes.empty() ? nullptr : m_scenes.top().get();
}

} // namespace serenkai