#pragma once

#include "shady/model.hpp"

#include <optional>
#include <ostream>
#include <string_view>

namespace shady {

std::optional<std::pair<int, int>> parse_size(std::string_view text);
std::optional<AppOptions> parse_arguments(int argc, char** argv, bool headless);
void print_usage(std::ostream& output, bool headless);

} // namespace shady
