#include "serenkai/gui/image_widget.hpp"

#include "serenkai/gui/widget.hpp"

namespace serenkai {

ImageWidget::ImageWidget(std::string_view name, Widget* parent)
    : Widget(name, parent) {}

void ImageWidget::set_image(std::string_view loc) { m_image = loc; }

const std::string& ImageWidget::get_image() const { return m_image; }

} // namespace serenkai