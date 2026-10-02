#pragma once
#include "serenkai/scenes/scene.hpp"

#include <memory>
#include <optional>
#include <stack>
#include <vector>
namespace serenkai {
class Renderer;

/// @brief Class for managing Scene
///
/// Used to manage and switch the current Scene, ensuring that the Scene enters
/// and exits properly.
class SceneManager {
public:
    SceneManager(const SceneManager&) = delete;
    SceneManager(SceneManager&&) = delete;
    SceneManager& operator=(const SceneManager&) = delete;
    SceneManager& operator=(SceneManager&&) = delete;
    SceneManager();
    virtual ~SceneManager();

    void update(float dt);
    void render(Renderer& renderer);

    bool handle_event(const Event& e);

    void request_change(SceneType type);
    void request_push(SceneType type);
    void request_pop();

    [[nodiscard]] std::size_t size() const noexcept;
    [[nodiscard]] bool empty() const noexcept;
    [[nodiscard]] Scene* current_scene() const noexcept;

protected:
    virtual std::unique_ptr<Scene> create_scene(SceneType type);

private:
    enum class OperationType { Push, Pop, Change };
    struct Operation {
        OperationType type;
        std::optional<SceneType> scene;
        Operation(OperationType op, std::optional<SceneType> s)
            : type(op), scene(s) {}
    };

    std::vector<std::unique_ptr<Scene>> m_pending_delete_scene;
    std::optional<Operation> m_operation;
    std::stack<std::unique_ptr<Scene>> m_scenes;

    void process_operation();

    void push(SceneType type);
    void change(SceneType type);
    void pop();
};
} // namespace serenkai