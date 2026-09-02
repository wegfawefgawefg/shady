#include "shady/files.hpp"

#include <fstream>
#include <sstream>

namespace shady {

std::optional<std::string> read_text_file(const std::string& path) {
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        return std::nullopt;
    }
    std::ostringstream content;
    content << input.rdbuf();
    return content.str();
}

std::string compose_shader(const std::string& prelude, const std::string& source,
                           const std::string& entry) {
    if (source.find("fn shade") == std::string::npos) {
        return {};
    }
    std::string composed;
    composed.reserve(prelude.size() + source.size() + entry.size() + 6);
    composed += prelude;
    composed += "\n\n";
    composed += source;
    composed += "\n\n";
    composed += entry;
    composed += '\n';
    return composed;
}

RuntimePaths default_runtime_paths() {
#ifdef SHADY_RUNTIME_DIR
    const std::string root = SHADY_RUNTIME_DIR;
#else
    const std::string root = "runtime";
#endif
    return {root + "/prelude.wgsl", root + "/entry.wgsl"};
}

std::optional<ShaderSources> load_shader_sources(const ShaderProject& project,
                                                 const RuntimePaths& runtime) {
    const auto prelude = read_text_file(runtime.prelude);
    const auto entry = read_text_file(runtime.entry);
    const auto image = read_text_file(project.image_path);
    if (!prelude || !entry || !image) {
        return std::nullopt;
    }

    ShaderSources sources;
    sources.image = compose_shader(*prelude, *image, *entry);
    if (sources.image.empty()) {
        return std::nullopt;
    }
    for (std::size_t index = 0; index < project.buffer_paths.size(); ++index) {
        if (project.buffer_paths[index].empty()) {
            continue;
        }
        const auto buffer = read_text_file(project.buffer_paths[index]);
        if (!buffer) {
            return std::nullopt;
        }
        sources.buffers[index] = compose_shader(*prelude, *buffer, *entry);
        if (sources.buffers[index].empty()) {
            return std::nullopt;
        }
        sources.has_buffer[index] = true;
    }
    return sources;
}

std::filesystem::file_time_type file_stamp(const std::string& path) {
    std::error_code error;
    const auto stamp = std::filesystem::last_write_time(path, error);
    return error ? std::filesystem::file_time_type::min() : stamp;
}

SourceWatch::SourceWatch(ShaderProject project) : project_(std::move(project)) {
    acknowledge();
}

bool SourceWatch::changed() {
    if (file_stamp(project_.image_path) != stamps_[0]) {
        return true;
    }
    for (std::size_t index = 0; index < project_.buffer_paths.size(); ++index) {
        if (!project_.buffer_paths[index].empty() &&
            file_stamp(project_.buffer_paths[index]) != stamps_[index + 1]) {
            return true;
        }
    }
    return false;
}

void SourceWatch::acknowledge() {
    stamps_[0] = file_stamp(project_.image_path);
    for (std::size_t index = 0; index < project_.buffer_paths.size(); ++index) {
        stamps_[index + 1] = file_stamp(project_.buffer_paths[index]);
    }
}

} // namespace shady
