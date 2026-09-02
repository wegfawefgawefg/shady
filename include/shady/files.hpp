#pragma once

#include "shady/model.hpp"

#include <filesystem>
#include <optional>
#include <string>

namespace shady {

std::optional<std::string> read_text_file(const std::string& path);
std::string compose_shader(const std::string& prelude, const std::string& source,
                           const std::string& entry);
RuntimePaths default_runtime_paths();
std::optional<ShaderSources> load_shader_sources(const ShaderProject& project,
                                                 const RuntimePaths& runtime);
std::filesystem::file_time_type file_stamp(const std::string& path);

class SourceWatch {
  public:
    explicit SourceWatch(ShaderProject project);
    [[nodiscard]] bool changed();
    void acknowledge();

  private:
    ShaderProject project_;
    std::array<std::filesystem::file_time_type, channel_count + 1> stamps_{};
};

} // namespace shady
