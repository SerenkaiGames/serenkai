#pragma once

#include "serenkai/gui/widget.hpp"

#include <string>
#include <string_view>
namespace serenkai {
/// @brief image widget
///
/// The image component does not automatically set its size.
class ImageWidget : public Widget {
public:
    ImageWidget(std::string_view name, Widget* parent);

    void set_image(std::string_view loc);

    const std::string& get_image() const;

private:
    void on_render(GuiContext* context) override;

    std::string m_image;
};
} // namespace serenkai