#pragma once
#include "serenkai/gui/anchor.hpp"

#include <glm/vec2.hpp>
#include <memory>
#include <span>
#include <vector>

namespace serenkai {
class GuiContext;

/// @brief Base class for Widget components
///
/// Defines the basic properties of a widget.
/// @note All UI widgets should inherit from this class.
/// All coordinates and sizes are integers.
/// All coordinates are in the logical coordinate system.
class Widget {
public:
    Widget(const Widget&) = delete;
    Widget(Widget&&) = delete;
    Widget& operator=(const Widget&) = delete;
    Widget& operator=(Widget&&) = delete;

    explicit Widget(Widget* parent);

    static void set_logical_window_size(glm::ivec2 size);
    static glm::ivec2 logical_window_size();

    virtual ~Widget() = default;

    virtual void update(float dt);

    virtual void render(GuiContext* context);

    virtual void set_anchor(Anchor anchor);
    virtual void set_offset(glm::ivec2 offset);
    virtual void set_size(glm::ivec2 size);
    virtual void set_visible(bool visible);

    /// @brief Adds an existing widget as a child and updates its parent to this
    /// node.
    void add_child(std::unique_ptr<Widget> child);

    template <typename T, typename... Args> T& create_child(Args&&... args) {
        auto widget = std::make_unique<T>(std::forward<Args>(args)..., this);
        T& ref = *widget;
        m_children.emplace_back(std::move(widget));
        return ref;
    }

    glm::ivec2 size() const;
    glm::ivec2 offset() const;

    /// @brief Computes the absolute logical coordinates in real time and
    /// provides them to the renderer for rendering.
    glm::ivec2 pos() const;

    Anchor anchor() const;

    bool is_visible() const;

    Widget* parent() const;
    std::span<const std::unique_ptr<Widget>> children() const;

protected:
    // When parent is nullptr, it can compute the root node's coordinates
    // relative to the logical window.
    static inline glm::ivec2 m_logical_window_size{0, 0};

    virtual void on_update(float dt);

    virtual void on_render(GuiContext* context);

    glm::ivec2 compute_position() const;

private:
    Widget* m_parent = nullptr;
    std::vector<std::unique_ptr<Widget>> m_children;
    glm::ivec2 m_size{0};   // Logical size
    glm::ivec2 m_offset{0}; // Logical offset, +x to the right, +y downward
    Anchor m_anchor{Anchor::TopLeft};
    bool m_visible = true;
};
} // namespace serenkai