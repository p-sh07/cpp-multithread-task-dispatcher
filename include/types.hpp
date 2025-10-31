#pragma once

#include <memory>
#include <ranges>

namespace dispatcher {
namespace rg = std::ranges;
namespace vw = std::views;

enum class TaskPriority { High, Normal };

}  // namespace dispatcher