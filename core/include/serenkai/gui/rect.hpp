#pragma once

#include "serenkai/gui/color.hpp"
#include "serenkai/gui/widget.hpp"

#include <string_view>
namespace serenkai {
/// @brief Basic rectangle widget.
///
/// Provides basic rectangle drawing, supports different alpha values, and
/// supports fill parent.
class Rect : public Widget {
public:
    Rect(std::string_view name, Widget* parent);

    /// @brief Set the rectangle fill color.
    void set_color(Color color);

    /// @brief Set the rectangle opacity/alpha in the range [0.0, 1.0].
    void set_alpha(float alpha);

    /// @brief Set whether the rectangle automatically fills its parent or
    /// logical window.
    void set_fill_parent(bool fill);

    /// @brief Set the rectangle size explicitly.
    /// @note Ignored if fill_parent is true.
    void set_size(glm::ivec2 size) override;

    Color color() const;

    float alpha() const;

    bool fill_parent() const;

private:
    void on_render(GuiContext* context) override;
    void on_update(float dt) override;

    Color m_color = Color::White;
    float m_alpha = 1.0f;
    bool m_fill_parent = false;
};
} // namespace serenkai