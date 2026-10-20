#include "serenkai/resource/po_loader.hpp"

#include "serenkai/base/language.hpp"
#include "serenkai/resource/asset_manager.hpp"
#include "serenkai/resource/po_data.hpp"
#include "serenkai/resource/resource_location.hpp"

#include <cstddef>
#include <fstream>
#include <magic_enum/magic_enum.hpp>
#include <optional>
#include <spdlog/spdlog.h>
#include <string>
#include <string_view>
#include <utility>

namespace {
std::string_view trim_right(std::string_view line) {
    auto pos = line.find_first_not_of(" \t");
    if (pos == std::string_view::npos) {
        return {};
    }
    return std::string_view(line.data() + pos, line.size() - pos);
}

std::string_view trim(std::string_view line) {
    auto start = line.find_first_not_of(" \t");
    if (start == std::string_view::npos) {
        return {};
    }
    auto end = line.find_last_not_of(" \t");
    return line.substr(start, end - start + 1);
}

std::string_view parse_quoted(std::string_view s, size_t pos) {
    while (pos < s.size() && s[pos] != '"') {
        ++pos;
    }

    if (pos >= s.size()) {
        return {};
    }

    auto begin = pos + 1;

    // Find the position of the first " scanning from right to left.
    auto end = s.rfind('"');

    if (end == std::string_view::npos) {
        return {};
    }

    return std::string_view(s.data() + begin, end - begin);
}

enum class MsgField { Msgid, Msgctxt, Msgstr, Header };

} // namespace

namespace serenkai {
PoLoader::PoLoader(AssetManager* asset_manager)
    : m_asset_manager(asset_manager) {}

std::optional<PoData> PoLoader::parse(std::string_view loc) {
    auto res = ResourceLocation::parse(loc);
    if (!res) {
        spdlog::error("Invalid resource location {}", loc);
        return std::nullopt;
    }

    return parse(*res);
}

bool parse_header(std::string_view header, PoData& data) {
    auto p_lang = header.find("Language:");
    if (p_lang == std::string_view::npos) {
        spdlog::error("Missing language field");
        return false;
    }
    auto end = header.find_first_of('"', p_lang);
    if (end == std::string_view::npos) {
        spdlog::error("Invalid language filed {}", p_lang);
        return false;
    }

    auto raw_lang = header.substr(p_lang + 9, end - p_lang - 9 + 1);
    auto lang_str = trim(raw_lang);
    auto lang =
        magic_enum::enum_cast<Language>(lang_str, magic_enum::case_insensitive);
    if (!lang) {
        spdlog::error("Language {} not supported", lang_str);
        return false;
    }
    data.lang = *lang;
    return true;
}

std::optional<PoData> PoLoader::parse(const ResourceLocation& loc) {
    if (!m_asset_manager) {
        spdlog::debug("AssetManager is nullptr");
        return std::nullopt;
    }

    auto path = m_asset_manager->get(loc);
    if (!path) {
        spdlog::error("Failed to get asset {} path", loc.str());
        return std::nullopt;
    }

    std::ifstream file(*path);

    if (!file.is_open()) {
        spdlog::error("Failed to open file {}", *path);
        return std::nullopt;
    }

    std::string line;

    PoData data{};

    bool parsed_header = false;

    std::string header;

    std::string cur_msgid;
    std::string cur_msgctxt;
    PoEntry cur_entry{};

    MsgField field = MsgField::Header;

    while (std::getline(file, line)) {
        auto l = trim_right(line);

        if (l.empty() || l.starts_with("#")) {
            continue;
        }

        // The project strictly requires msgctext to be present for semantic
        // keys.
        else if (l.starts_with("msgctxt")) {
            field = MsgField::Msgctxt;
            if (!parsed_header) {
                parsed_header = true;
            } else {
                auto res = ResourceLocation::parse(cur_msgctxt);
                if (!res) {
                    spdlog::error("Invalid key {}", cur_msgctxt);

                } else {
                    cur_entry.msgctxt = *res;
                    data.entires.emplace_back(std::move(cur_entry));
                }
            }
            cur_entry = PoEntry{};
            cur_msgctxt += parse_quoted(l, 7);
        } else if (l.starts_with("msgid")) {
            if (!parsed_header) {
                field = MsgField::Header;
            } else {
                field = MsgField::Msgid;
            }
        } else if (l.starts_with("msgstr")) {
            cur_entry.msgstr += parse_quoted(l, 6);
        } else {
            switch (field) {
            case MsgField::Msgctxt:
                cur_msgctxt += parse_quoted(l, 7);
                break;
            case MsgField::Msgid:
                break;
            case MsgField::Msgstr:
                cur_entry.msgstr += parse_quoted(l, 6);
                break;
            case MsgField::Header:
                if (!parsed_header) {
                    header += parse_quoted(l, 5);
                }
                break;
            }
        }
    }

    if (!parse_header(header, data)) {
        return std::nullopt;
    }

    return data;
}

} // namespace serenkai