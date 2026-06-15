#pragma once

#include <cstddef>
#include <exception>
#include <string>
#include <string_view>

#include <spdlog/common.h>

#include "api.h"

RUZINO_NAMESPACE_OPEN_SCOPE

NODES_CORE_API void initialize_framework_logging(
    const std::string& app_name,
    spdlog::level::level_enum level = spdlog::level::info,
    const std::string& log_directory = {},
    size_t context_message_count = 200,
    bool enable_stdout = true);

NODES_CORE_API std::string framework_log_file();
NODES_CORE_API std::string framework_error_log_file();

NODES_CORE_API void log_exception_with_context(
    std::string_view context,
    const std::exception& exception);
NODES_CORE_API void log_current_exception_with_context(std::string_view context);

RUZINO_NAMESPACE_CLOSE_SCOPE
