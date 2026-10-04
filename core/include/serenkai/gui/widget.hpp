#pragma once
#include "serenkai/application/event.hpp"
#include "serenkai/gui/anchor.hpp"

#include <algorithm>
#include <concepts>
#include <cstddef>
#include <glm/vec2.hpp>
#include <memory>
#include <span>
#include <string>
#include <string_view>
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

    explicit Widget(std::string_view name, Widget* parent);

    static void set_logical_window_size(glm::ivec2 size);
    static glm::ivec2 logical_window_size();

    static void set_ui_scale(size_t ui_scale);
    static size_t ui_scale();

    virtual ~Widget() = default;

    virtual void update(float dt);

    virtual void render(GuiContext* context);

    virtual void set_anchor(Anchor anchor);
    virtual void set_offset(glm::ivec2 offset);
    virtual void set_size(glm::ivec2 size);
    virtual void set_visible(bool visible);

    virtual bool handle_mouse_move_event(const MouseMoveEvent& e);
    virtual bool handle_key_event(const KeyEvent& e);

    /// @brief Adds an existing widget as a child and updates its parent to this
    /// node.
    void add_child(std::unique_ptr<Widget> child);

    template <typename T, typename... Args> T& create_child(Args&&... args) {
        auto widget = std::make_unique<T>(std::forward<Args>(args)..., this);
        T& ref = *widget;
        m_children.emplace_back(std::move(widget));
        return ref;
    }

    /// @brief Gets the component of a child node by name
    ///
    /// If the type does not match, it will return nullptr
    /// Linear time complexity
    template <std::derived_from<Widget> T>
    T* fetch_child(std::string_view name) {
        auto it = std::find_if(m_children.begin(), m_children.end(),
                               [name](const std::unique_ptr<Widget>& widget) {
                                   return name == widget->m_name;
                               });
        if (it == m_children.end()) {
            return nullptr;
        }
        return dynamic_cast<T*>(it->get());
    }

    template <std::derived_from<Widget> T>
    const T* fetch_child(std::string_view name) const {
        return const_cast<Widget*>(this)->fetch_child<T>(name);
    }

    glm::ivec2 size() const;
    glm::ivec2 offset() const;

    /// @brief Computes the absolute logical coordinates in real time and
    /// provides them to the renderer for rendering.
    glm::ivec2 pos() const;

    Anchor anchor() const;

    bool is_visible() const;

    bool has_parent() const;

    Widget* parent() const;

    const std::string& name() const;

    std::span<const std::unique_ptr<Widget>> children() const;

protected:
    virtual void on_update(float dt);

    /// @note Need to check whether context is nullptr.
    virtual void on_render(GuiContext* context);

    glm::ivec2 compute_position() const;

private:
    // When parent is nullptr, it can compute the root node's coordinates
    // relative to the logical window.
    static inline glm::ivec2 m_logical_window_size{0, 0};

    // For widgets that need to handle mouse events.
    static inline size_t m_ui_scale{3};

    const std::string m_name;
    Widget* m_parent = nullptr;
    std::vector<std::unique_ptr<Widget>> m_children;
    glm::ivec2 m_size{0};   // Logical size
    glm::ivec2 m_offset{0}; // Logical offset, +x to the right, +y downward
    Anchor m_anchor{Anchor::TopLeft};
    bool m_visible = true;
};
} // namespace serenkai