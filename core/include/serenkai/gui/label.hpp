#pragma once

#include "serenkai/gui/color.hpp"
#include "serenkai/gui/widget.hpp"

#include <string>
namespace serenkai {
class Font;

/// @brief Text component that only renders text
///
/// The label's size is determined by the text size.
class Label : public Widget {
public:
    Label(Widget* parent);

    void set_text(std::string text);
    void set_color(Color color);
    void set_font(Font* font);

    std::string text() const;
    Color color() const;
    Font* font() const;

private:
    std::string m_text;
    Color m_color = Color::White;
    Font* m_font = nullptr;

    void measure_size();
    void on_render(GuiContext* context) override;
};
} // namespace serenkai