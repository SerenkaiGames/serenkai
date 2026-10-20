#pragma once

#include "serenkai/base/language.hpp"
#include "serenkai/resource/resource_location.hpp"

#include <string>
#include <vector>

namespace serenkai {

struct PoEntry {

    // Semantic key, must be present, eg: "ui.error"
    // If duplicates occur, use "ns:ui.error" to distinguish them.
    ResourceLocation msgctxt;
    std::string msgstr; // The translated text.
};

struct PoData {
    Language lang = Language::en_US;
    int nplurals = 2;
    std::vector<PoEntry> entires;
};

} // namespace serenkai