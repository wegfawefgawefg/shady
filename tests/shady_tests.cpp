#include "shady/arguments.hpp"
#include "shady/files.hpp"

#include <iostream>
#include <string>
#include <vector>

namespace {
int failures = 0;

void check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "failed: " << message << '\n';
        ++failures;
    }
}

std::optional<shady::AppOptions> arguments(std::vector<std::string> values, bool headless) {
    std::vector<char*> raw;
    raw.reserve(values.size());
    for (std::string& value : values) raw.push_back(value.data());
    return shady::parse_arguments(static_cast<int>(raw.size()), raw.data(), headless);
}
} // namespace

int main() {
    check(shady::parse_size("gba") == std::pair{240, 160}, "size preset");
    check(shady::parse_size("800x450") == std::pair{800, 450}, "custom size");
    check(!shady::parse_size("zero"), "bad size");
    const auto parsed = arguments({"shady", "main.wgsl", "--noise0", "grain",
                                   "--buffer1", "blur.wgsl", "--size", "gba"}, false);
    check(parsed.has_value(), "desktop arguments");
    check(parsed && parsed->project.image_path == "main.wgsl", "positional shader");
    check(parsed && parsed->noise_seeds[0] == "grain", "channel option");
    check(parsed && parsed->project.buffer_paths[1] == "blur.wgsl", "buffer option");
    check(!arguments({"shady-frame", "main.wgsl"}, true), "headless needs output");
    const std::string composed = shady::compose_shader("prelude", "fn shade() {}", "entry");
    check(composed == "prelude\n\nfn shade() {}\n\nentry\n", "shader composition");
    check(shady::compose_shader("prelude", "no entry", "entry").empty(), "shade contract");
    return failures == 0 ? 0 : 1;
}
