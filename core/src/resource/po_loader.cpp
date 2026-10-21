#include "serenkai/resource/po_loader.hpp"

#include "serenkai/base/language.hpp"
#include "serenkai/resource/asset_manager.hpp"
#include "serenkai/resource/po_data.hpp"
#include "serenkai/resource/resource_location.hpp"

#include <algorithm>
#include <fstream>
#include <magic_enum/magic_enum.hpp>
#include <optional>
#include <spdlog/spdlog.h>
#include <string>
#include <string_view>
#include <utility>

namespace {
std::string_view trim_left(std::string_view line) {
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

std::string_view parse_quoted(std::string_view s) {
    auto first = s.find('"');
    auto last = s.rfind('"');
    if (first == std::string_view::npos || last == std::string_view::npos ||
        first >= last) {
        return {};
    }
    return s.substr(first + 1, last - first - 1);
}

std::string unescape(std::string_view s) {
    std::string result;
    result.reserve(s.size());

    for (std::size_t i = 0; i < s.size(); ++i) {
        if (s[i] == '\\' && i + 1 < s.size()) {
            ++i;
            switch (s[i]) {
            case 'n':
                result.push_back('\n');
                break;
            case 't':
                result.push_back('\t');
                break;
            case 'r':
                result.push_back('\r');
                break;
            case '"':
                result.push_back('"');
                break;
            case '\\':
                result.push_back('\\');
                break;
            default:

                result.push_back('\\');
                result.push_back(s[i]);
                break;
            }
        } else {
            result.push_back(s[i]);
        }
    }
    return result;
}

enum class MsgField { Msgid, Msgctxt, Msgstr, Header };

} // namespace

namespace serenkai {
PoLoader::PoLoader(AssetManager* asset_manager)
    : m_asset_manager(asset_manager) {}

std::optional<PoData> PoLoader::load(std::string_view loc) {
    auto res = ResourceLocation::parse(loc);
    if (!res) {
        spdlog::error("Invalid resource location {}", loc);
        return std::nullopt;
    }

    return load(*res);
}

bool parse_header(std::string_view header, PoData& data) {
    auto p_lang = header.find("Language:");
    if (p_lang == std::string_view::npos) {
        spdlog::error("Missing language field");
        return false;
    }
    auto val_start = p_lang + 9;
    auto val_end = header.find_first_of("\r\n\\;\"", val_start);
    auto raw_lang = (val_end == std::string_view::npos)
                        ? header.substr(val_start)
                        : header.substr(val_start, val_end - val_start);
    auto lang_str = trim(raw_lang);
    std::string normalized{lang_str};
    std::ranges::replace(normalized, '-', '_');
    auto lang = magic_enum::enum_cast<Language>(normalized,
                                                magic_enum::case_insensitive);

    if (!lang) {
        spdlog::error("Language {} not supported", lang_str);
        return false;
    }
    data.lang = *lang;
    return true;
}

std::optional<PoData> PoLoader::load(const ResourceLocation& loc) {
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

    auto commit_entry = [&]() {
        if (parsed_header && !cur_msgctxt.empty()) {
            auto res = ResourceLocation::parse(cur_msgctxt);
            if (res) {
                cur_entry.msgctxt = *res;
                cur_entry.msgstr = unescape(cur_entry.msgstr);
                data.entries.emplace_back(std::move(cur_entry));
            } else {
                spdlog::error("Invalid key {}", cur_msgctxt);
            }
        }
        cur_entry = PoEntry{};
        cur_msgctxt.clear();
    };

    while (std::getline(file, line)) {
        auto l = trim_left(line);

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
                commit_entry();
            }
            cur_msgctxt += parse_quoted(l);
        } else if (l.starts_with("msgid")) {
            field = parsed_header ? MsgField::Msgid : MsgField::Header;
        } else if (l.starts_with("msgstr")) {
            if (!parsed_header) {
                field = MsgField::Header;
                header += parse_quoted(l);
            } else {
                field = MsgField::Msgstr;
                cur_entry.msgstr += parse_quoted(l);
            }
        } else {
            switch (field) {
            case MsgField::Msgctxt:
                cur_msgctxt += parse_quoted(l);
                break;
            case MsgField::Msgid:
                break;
            case MsgField::Msgstr:
                cur_entry.msgstr += parse_quoted(l);
                break;
            case MsgField::Header:
                if (!parsed_header) {
                    header += parse_quoted(l);
                }
                break;
            }
        }
    }

    // The last one.
    commit_entry();

    if (!parse_header(header, data)) {
        return std::nullopt;
    }

    return data;
}

} // namespace serenkai