#include "task_dispatcher.hpp"

namespace dispatcher {
TaskDispatcher::TaskDispatcher(size_t thread_count) {}
TaskDispatcher::~TaskDispatcher() {}

void TaskDispatcher::schedule(TaskPriority priority, std::function<void()> task) {}
} // namespace dispatcher