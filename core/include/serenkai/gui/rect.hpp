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

    void set_color(Color color);
    void set_alpha(float alpha);
    void set_fill_parent(bool fill);
    void set_size(glm::ivec2 size) override;

    Color color() const;
    float alpha() const;

private:
    void on_render(GuiContext* context) override;
    void on_update(float dt) override;

    Color m_color = Color::White;
    float m_alpha = 1.0f;
    bool m_fill_parent = false;
};
} // namespace serenkai