#include "serenkai/resource/directory_source.hpp"

#include "serenkai/base/raii.hpp"
#include "serenkai/resource/resource_location.hpp"

#include <exception>
#include <filesystem>
#include <fmt/format.h>
#include <glaze/glaze.hpp>
#include <spdlog/spdlog.h>
#include <stdexcept>

namespace fs = std::filesystem;
namespace serenkai::detail {

constexpr const char* CONFIG_NAME = "assets.json";

struct DirectoryConfig {
    std::string ns;
};
} // namespace serenkai::detail

template <> struct glz::meta<serenkai::detail::DirectoryConfig> {
    static constexpr bool requires_key(std::string_view key, bool) {

        if (key == "ns") {
            return true;
        }

        return false;
    }
};

namespace serenkai {

DirectorySource::DirectorySource(fs::path dir)
    : m_dir_str(dir.lexically_normal().string()) {
    try {
        search(dir);
    } catch (const std::exception& e) {
        spdlog::error("Load directory source failed, {}", e.what());
    }
}

void DirectorySource::search(std::filesystem::path dir) {
    auto guard = RaiiGuard{
        [&dir]() { spdlog::info("Start assets search {}", dir.string()); },
        [this]() {
            spdlog::info("Finished assets search, find assets size {}",
                         m_files.size());
        }};

    if (!fs::exists(dir) || !fs::is_directory(dir)) {
        return;
    }

    fs::path config = dir / detail::CONFIG_NAME;

    if (!fs::exists(config)) {
        return;
    }

    detail::DirectoryConfig data;
    std::string buffer{};

    auto ec = glz::read_file_json<glz::opts{.error_on_missing_keys = true}>(
        data, config.string(), buffer);

    if (ec) {
        std::string error_msg = glz::format_error(ec, buffer);
        throw std::runtime_error(error_msg);
    }

    for (auto entry : fs::recursive_directory_iterator(
             dir, fs::directory_options::skip_permission_denied)) {
        if (!fs::is_regular_file(entry)) {
            continue;
        }

        if (entry.path().filename().string() == detail::CONFIG_NAME) {
            continue;
        }

        std::string entry_str = entry.path().lexically_normal().string();

        auto p = fs::relative(entry.path(), dir).generic_string();

        auto resource = ResourceLocation::parse(data.ns + ":" + p);
        if (!resource) {
            spdlog::warn("Invalid path string {}:{}", data.ns, p);
            continue;
        }
        m_files.emplace(std::move(*resource), std::move(entry_str));
    }
}

AssetFileMap& DirectorySource::get_asset_files() { return m_files; }

std::string DirectorySource::source_name() const {
    return fmt::format("Directory Source: {}", m_dir_str);
}

} // namespace serenkai