#include "nodes/core/logging.hpp"

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <algorithm>
#include <chrono>
#include <csignal>
#include <deque>
#include <filesystem>
#include <new>
#include <mutex>
#include <sstream>
#include <string>
#include <typeinfo>
#include <vector>

#include <spdlog/details/log_msg.h>
#include <spdlog/logger.h>
#include <spdlog/sinks/base_sink.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#if defined(_WIN32)
#include <Windows.h>
#include <eh.h>
#include <io.h>
#include <process.h>
#else
#include <sys/types.h>
#include <unistd.h>
#endif

RUZINO_NAMESPACE_OPEN_SCOPE

namespace {

constexpr const char* kDetailedLogPattern =
    "[%Y-%m-%d %H:%M:%S.%e] [%P:%t] [%l] [%n] %v";

int current_process_id()
{
#if defined(_WIN32)
    return static_cast<int>(::_getpid());
#else
    return static_cast<int>(::getpid());
#endif
}

std::string current_timestamp()
{
    const auto now = std::chrono::system_clock::now();
    const auto time_t_now = std::chrono::system_clock::to_time_t(now);
    const auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(
                                  now.time_since_epoch()) %
                              1000;

    std::tm local_tm {};
#if defined(_WIN32)
    localtime_s(&local_tm, &time_t_now);
#else
    localtime_r(&time_t_now, &local_tm);
#endif

    char buffer[64] = {};
    std::snprintf(
        buffer,
        sizeof(buffer),
        "%04d-%02d-%02d %02d:%02d:%02d.%03d",
        local_tm.tm_year + 1900,
        local_tm.tm_mon + 1,
        local_tm.tm_mday,
        local_tm.tm_hour,
        local_tm.tm_min,
        local_tm.tm_sec,
        static_cast<int>(milliseconds.count()));
    return std::string(buffer);
}

std::filesystem::path process_directory()
{
#if defined(_WIN32)
    std::wstring buffer(MAX_PATH, L'\0');
    const DWORD size = ::GetModuleFileNameW(nullptr, buffer.data(), MAX_PATH);
    if (size > 0) {
        buffer.resize(size);
        return std::filesystem::path(buffer).parent_path();
    }
#endif
    return std::filesystem::current_path();
}

std::string sanitize_filename(std::string name)
{
    if (name.empty()) {
        return "Framework3D";
    }

    for (char& ch : name) {
        const bool valid = (ch >= '0' && ch <= '9') || (ch >= 'A' && ch <= 'Z') ||
                           (ch >= 'a' && ch <= 'z') || ch == '_' || ch == '-';
        if (!valid) {
            ch = '_';
        }
    }
    return name;
}

std::string source_location_string(const spdlog::source_loc& source)
{
    if (!source.filename || source.line <= 0) {
        return "<unknown>";
    }

    std::ostringstream oss;
    oss << source.filename << ':' << source.line;
    if (source.funcname && std::strlen(source.funcname) > 0) {
        oss << " (" << source.funcname << ')';
    }
    return oss.str();
}

std::string level_string(spdlog::level::level_enum level)
{
    const auto view = spdlog::level::to_string_view(level);
    return std::string(view.data(), view.size());
}

std::string format_log_entry(const spdlog::details::log_msg& msg)
{
    std::ostringstream oss;
    oss << current_timestamp() << " [pid:" << current_process_id() << "] [tid:"
        << msg.thread_id << "] [" << level_string(msg.level) << "] ";

    if (msg.logger_name.size() > 0) {
        oss << '[' << std::string(msg.logger_name.data(), msg.logger_name.size())
            << "] ";
    }

    oss << std::string(msg.payload.data(), msg.payload.size());

    if (msg.source.filename) {
        oss << " | source=" << source_location_string(msg.source);
    }

    return oss.str();
}

std::string nested_exception_details(const std::exception& exception, int depth = 0)
{
    std::ostringstream oss;
    oss << std::string(static_cast<size_t>(depth) * 2, ' ') << "- "
        << typeid(exception).name() << ": " << exception.what() << '\n';

    try {
        std::rethrow_if_nested(exception);
    }
    catch (const std::exception& nested) {
        oss << nested_exception_details(nested, depth + 1);
    }
    catch (...) {
        oss << std::string(static_cast<size_t>(depth + 1) * 2, ' ')
            << "- <non-std nested exception>\n";
    }

    return oss.str();
}

std::string current_exception_details()
{
    auto current = std::current_exception();
    if (!current) {
        return "No active exception.";
    }

    try {
        std::rethrow_exception(current);
    }
    catch (const std::exception& exception) {
        return nested_exception_details(exception);
    }
    catch (...) {
        return "<non-std exception>";
    }
}

const char* signal_name(int signal_number)
{
    switch (signal_number) {
        case SIGABRT: return "SIGABRT";
        case SIGFPE: return "SIGFPE";
        case SIGILL: return "SIGILL";
        case SIGINT: return "SIGINT";
        case SIGSEGV: return "SIGSEGV";
        case SIGTERM: return "SIGTERM";
        default: return "UNKNOWN_SIGNAL";
    }
}

#if defined(_WIN32)

std::string wide_to_utf8(const wchar_t* text)
{
    if (!text) {
        return "<null>";
    }

    const int utf8_size =
        ::WideCharToMultiByte(CP_UTF8, 0, text, -1, nullptr, 0, nullptr, nullptr);
    if (utf8_size <= 0) {
        return "<wide-char-conversion-failed>";
    }

    std::string result(static_cast<size_t>(utf8_size), '\0');
    ::WideCharToMultiByte(
        CP_UTF8,
        0,
        text,
        -1,
        result.data(),
        utf8_size,
        nullptr,
        nullptr);
    if (!result.empty() && result.back() == '\0') {
        result.pop_back();
    }
    return result;
}

std::string seh_code_name(DWORD code)
{
    switch (code) {
        case EXCEPTION_ACCESS_VIOLATION: return "EXCEPTION_ACCESS_VIOLATION";
        case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
            return "EXCEPTION_ARRAY_BOUNDS_EXCEEDED";
        case EXCEPTION_BREAKPOINT: return "EXCEPTION_BREAKPOINT";
        case EXCEPTION_DATATYPE_MISALIGNMENT:
            return "EXCEPTION_DATATYPE_MISALIGNMENT";
        case EXCEPTION_FLT_DENORMAL_OPERAND:
            return "EXCEPTION_FLT_DENORMAL_OPERAND";
        case EXCEPTION_FLT_DIVIDE_BY_ZERO:
            return "EXCEPTION_FLT_DIVIDE_BY_ZERO";
        case EXCEPTION_FLT_INVALID_OPERATION:
            return "EXCEPTION_FLT_INVALID_OPERATION";
        case EXCEPTION_FLT_OVERFLOW: return "EXCEPTION_FLT_OVERFLOW";
        case EXCEPTION_FLT_STACK_CHECK: return "EXCEPTION_FLT_STACK_CHECK";
        case EXCEPTION_FLT_UNDERFLOW: return "EXCEPTION_FLT_UNDERFLOW";
        case EXCEPTION_ILLEGAL_INSTRUCTION:
            return "EXCEPTION_ILLEGAL_INSTRUCTION";
        case EXCEPTION_IN_PAGE_ERROR: return "EXCEPTION_IN_PAGE_ERROR";
        case EXCEPTION_INT_DIVIDE_BY_ZERO:
            return "EXCEPTION_INT_DIVIDE_BY_ZERO";
        case EXCEPTION_INT_OVERFLOW: return "EXCEPTION_INT_OVERFLOW";
        case EXCEPTION_INVALID_DISPOSITION:
            return "EXCEPTION_INVALID_DISPOSITION";
        case EXCEPTION_NONCONTINUABLE_EXCEPTION:
            return "EXCEPTION_NONCONTINUABLE_EXCEPTION";
        case EXCEPTION_PRIV_INSTRUCTION:
            return "EXCEPTION_PRIV_INSTRUCTION";
        case EXCEPTION_SINGLE_STEP: return "EXCEPTION_SINGLE_STEP";
        case EXCEPTION_STACK_OVERFLOW: return "EXCEPTION_STACK_OVERFLOW";
        default: return "UNKNOWN_SEH_EXCEPTION";
    }
}

std::string describe_seh_exception(EXCEPTION_POINTERS* exception_pointers)
{
    if (!exception_pointers || !exception_pointers->ExceptionRecord) {
        return "No EXCEPTION_RECORD information is available.";
    }

    const auto* record = exception_pointers->ExceptionRecord;
    const auto* context = exception_pointers->ContextRecord;

    std::ostringstream oss;
    oss << "seh_code     : 0x" << std::hex << std::uppercase
        << record->ExceptionCode << std::dec << " ("
        << seh_code_name(record->ExceptionCode) << ")\n";
    oss << "seh_flags    : 0x" << std::hex << std::uppercase
        << record->ExceptionFlags << std::dec << '\n';
    oss << "address      : " << record->ExceptionAddress << '\n';
    oss << "parameters   : " << record->NumberParameters << '\n';

    if (record->ExceptionCode == EXCEPTION_ACCESS_VIOLATION &&
        record->NumberParameters >= 2) {
        const auto access_type = record->ExceptionInformation[0];
        const auto fault_address = record->ExceptionInformation[1];
        const char* access_name = "unknown";
        if (access_type == 0) {
            access_name = "read";
        }
        else if (access_type == 1) {
            access_name = "write";
        }
        else if (access_type == 8) {
            access_name = "execute";
        }

        oss << "access_type  : " << access_name << '\n';
        oss << "fault_addr   : 0x" << std::hex << std::uppercase << fault_address
            << std::dec << '\n';
    }

    if (record->ExceptionCode == EXCEPTION_IN_PAGE_ERROR &&
        record->NumberParameters >= 3) {
        oss << "ntstatus     : 0x" << std::hex << std::uppercase
            << record->ExceptionInformation[2] << std::dec << '\n';
    }

    if (context) {
#if defined(_M_X64)
        oss << "registers    : RIP=0x" << std::hex << std::uppercase
            << context->Rip << " RSP=0x" << context->Rsp << " RBP=0x"
            << context->Rbp << std::dec << '\n';
#elif defined(_M_IX86)
        oss << "registers    : EIP=0x" << std::hex << std::uppercase
            << context->Eip << " ESP=0x" << context->Esp << " EBP=0x"
            << context->Ebp << std::dec << '\n';
#endif
    }

    return oss.str();
}

#endif

class forced_error_sink : public spdlog::sinks::base_sink<std::mutex> {
public:
    forced_error_sink(
        std::filesystem::path file_path,
        std::string app_name,
        size_t context_limit)
        : file_path_(std::move(file_path))
        , app_name_(std::move(app_name))
        , context_limit_(std::max<size_t>(context_limit, 20))
    {
        open_file();
    }

    ~forced_error_sink() override
    {
        close_file();
    }

    void write_manual_report(
        const std::string& title,
        const std::string& detail,
        spdlog::level::level_enum level = spdlog::level::critical)
    {
        std::lock_guard<std::mutex> lock(this->mutex_);
        write_report_locked(title, detail, level, nullptr);
    }

protected:
    void sink_it_(const spdlog::details::log_msg& msg) override
    {
        recent_entries_.push_back(format_log_entry(msg));
        while (recent_entries_.size() > context_limit_) {
            recent_entries_.pop_front();
        }

        if (msg.level >= spdlog::level::err) {
            write_report_locked(
                "Logged error captured by forced_error_sink",
                std::string(msg.payload.data(), msg.payload.size()),
                msg.level,
                &msg);
        }
    }

    void flush_() override
    {
        flush_file_locked();
    }

private:
    void open_file()
    {
#if defined(_WIN32)
        file_ = ::_wfopen(file_path_.wstring().c_str(), L"ab");
#else
        file_ = std::fopen(file_path_.string().c_str(), "ab");
#endif
    }

    void close_file()
    {
        if (file_) {
            flush_file_locked();
            std::fclose(file_);
            file_ = nullptr;
        }
    }

    void flush_file_locked()
    {
        if (!file_) {
            return;
        }

        std::fflush(file_);
#if defined(_WIN32)
        ::_commit(::_fileno(file_));
#endif
    }

    void write_report_locked(
        const std::string& title,
        const std::string& detail,
        spdlog::level::level_enum level,
        const spdlog::details::log_msg* msg)
    {
        if (!file_) {
            open_file();
        }

        if (!file_) {
            return;
        }

        std::ostringstream oss;
        oss << "\n==================== FRAMEWORK ERROR REPORT BEGIN ====================\n";
        oss << "timestamp    : " << current_timestamp() << '\n';
        oss << "application  : " << app_name_ << '\n';
        oss << "process_id   : " << current_process_id() << '\n';
        oss << "level        : " << level_string(level) << '\n';
        oss << "title        : " << title << '\n';

        if (msg) {
            oss << "thread_id    : " << msg->thread_id << '\n';
            oss << "logger       : "
                << std::string(msg->logger_name.data(), msg->logger_name.size())
                << '\n';
            oss << "source       : " << source_location_string(msg->source) << '\n';
        }

        oss << "detail       : " << detail << '\n';
        oss << "recent_log_context (" << recent_entries_.size() << " entries):\n";
        for (const auto& entry : recent_entries_) {
            oss << "  " << entry << '\n';
        }
        oss << "===================== FRAMEWORK ERROR REPORT END =====================\n";

        const std::string report = oss.str();
        std::fwrite(report.data(), 1, report.size(), file_);
        flush_file_locked();
    }

private:
    std::filesystem::path file_path_;
    std::string app_name_;
    size_t context_limit_;
    std::deque<std::string> recent_entries_;
    FILE* file_ = nullptr;
};

struct logging_state {
    std::mutex mutex;
    std::shared_ptr<forced_error_sink> error_sink;
    std::string log_file;
    std::string error_log_file;
    std::string app_name = "Framework3D";
    bool initialized = false;
    std::terminate_handler previous_terminate = nullptr;
#if defined(_WIN32)
    LPTOP_LEVEL_EXCEPTION_FILTER previous_unhandled_filter = nullptr;
#endif
};

logging_state& get_logging_state()
{
    static logging_state state;
    return state;
}

void emergency_append_report(
    const std::string& title,
    const std::string& detail,
    spdlog::level::level_enum level = spdlog::level::critical)
{
    const auto& state = get_logging_state();
    std::string path = state.error_log_file;
    if (path.empty()) {
        std::fprintf(
            stderr,
            "\n[Framework3D emergency log] %s\n%s\n",
            title.c_str(),
            detail.c_str());
        std::fflush(stderr);
        return;
    }

    std::ostringstream oss;
    oss << "\n==================== FRAMEWORK ERROR REPORT BEGIN ====================\n";
    oss << "timestamp    : " << current_timestamp() << '\n';
    oss << "application  : " << state.app_name << '\n';
    oss << "process_id   : " << current_process_id() << '\n';
    oss << "level        : " << level_string(level) << '\n';
    oss << "title        : " << title << '\n';
    oss << "detail       : " << detail << '\n';
    oss << "===================== FRAMEWORK ERROR REPORT END =====================\n";

    FILE* file = std::fopen(path.c_str(), "ab");
    if (!file) {
        std::fprintf(
            stderr,
            "\n[Framework3D emergency log] %s\n%s\n",
            title.c_str(),
            detail.c_str());
        std::fflush(stderr);
        return;
    }

    const std::string report = oss.str();
    std::fwrite(report.data(), 1, report.size(), file);
    std::fflush(file);
#if defined(_WIN32)
    ::_commit(::_fileno(file));
#endif
    std::fclose(file);
}

void write_manual_error_report(
    const std::string& title,
    const std::string& detail,
    spdlog::level::level_enum level = spdlog::level::critical)
{
    auto& state = get_logging_state();
    std::shared_ptr<forced_error_sink> error_sink;
    {
        std::lock_guard<std::mutex> lock(state.mutex);
        error_sink = state.error_sink;
    }
    if (error_sink) {
        error_sink->write_manual_report(title, detail, level);
        return;
    }
    emergency_append_report(title, detail, level);
}

void framework_terminate_handler()
{
    std::ostringstream oss;
    oss << "std::terminate was invoked.\n";
    oss << "active_exception:\n" << current_exception_details();
    emergency_append_report("Unhandled fatal termination", oss.str());

    std::_Exit(EXIT_FAILURE);
}

void framework_signal_handler(int signal_number)
{
    std::ostringstream oss;
    oss << "Fatal signal received.\n";
    oss << "signal       : " << signal_name(signal_number) << " (" << signal_number
        << ")\n";
    emergency_append_report("Fatal signal", oss.str());
    std::_Exit(128 + signal_number);
}

#if defined(_WIN32)
LONG WINAPI framework_unhandled_exception_filter(
    EXCEPTION_POINTERS* exception_pointers)
{
    emergency_append_report(
        "Unhandled SEH exception", describe_seh_exception(exception_pointers));
    return EXCEPTION_EXECUTE_HANDLER;
}

void __cdecl framework_invalid_parameter_handler(
    const wchar_t* expression,
    const wchar_t* function,
    const wchar_t* file,
    unsigned int line,
    uintptr_t)
{
    std::ostringstream oss;
    oss << "CRT invalid parameter handler invoked.\n";
    oss << "expression   : " << wide_to_utf8(expression) << '\n';
    oss << "function     : " << wide_to_utf8(function) << '\n';
    oss << "file         : " << wide_to_utf8(file) << '\n';
    oss << "line         : " << line << '\n';
    emergency_append_report("CRT invalid parameter", oss.str());
    std::_Exit(EXIT_FAILURE);
}

void __cdecl framework_purecall_handler()
{
    emergency_append_report(
        "Pure virtual function call",
        "A pure virtual function call reached the CRT purecall handler.");
    std::_Exit(EXIT_FAILURE);
}
#endif

}  // namespace

void initialize_framework_logging(
    const std::string& app_name,
    spdlog::level::level_enum level,
    const std::string& log_directory,
    size_t context_message_count,
    bool enable_stdout)
{
    auto& state = get_logging_state();
    std::lock_guard<std::mutex> lock(state.mutex);

    if (state.initialized) {
        spdlog::set_level(level);
        if (auto logger = spdlog::default_logger()) {
            logger->set_level(level);
            logger->flush_on(spdlog::level::err);
        }
        return;
    }

    std::filesystem::path base_directory =
        log_directory.empty() ? (process_directory() / "logs")
                              : std::filesystem::path(log_directory);
    std::filesystem::create_directories(base_directory);

    const std::string safe_name = sanitize_filename(app_name);
    const auto log_path = base_directory / (safe_name + ".log");
    const auto error_log_path = base_directory / (safe_name + ".error.log");

    std::vector<spdlog::sink_ptr> sinks;
    if (enable_stdout) {
        sinks.push_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
    }

    auto file_sink =
        std::make_shared<spdlog::sinks::basic_file_sink_mt>(log_path.string(), true);
    auto error_sink = std::make_shared<forced_error_sink>(
        error_log_path, app_name, context_message_count);
    sinks.push_back(file_sink);
    sinks.push_back(error_sink);

    auto logger = std::make_shared<spdlog::logger>(
        safe_name.empty() ? "Framework3D" : safe_name,
        sinks.begin(),
        sinks.end());
    logger->set_level(level);
    logger->set_pattern(kDetailedLogPattern);
    logger->flush_on(spdlog::level::err);
    logger->enable_backtrace(256);

    spdlog::set_default_logger(logger);
    spdlog::set_level(level);
    spdlog::set_error_handler([](const std::string& message) {
        write_manual_error_report(
            "spdlog internal failure",
            message,
            spdlog::level::critical);
    });
    spdlog::flush_every(std::chrono::seconds(1));

    state.error_sink = error_sink;
    state.log_file = log_path.string();
    state.error_log_file = error_log_path.string();
    state.app_name = app_name;
    state.previous_terminate = std::set_terminate(framework_terminate_handler);
#if defined(_WIN32)
    state.previous_unhandled_filter =
        ::SetUnhandledExceptionFilter(framework_unhandled_exception_filter);
    ::_set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
    ::_set_invalid_parameter_handler(framework_invalid_parameter_handler);
    ::_set_purecall_handler(framework_purecall_handler);
#endif
    std::signal(SIGABRT, framework_signal_handler);
    std::signal(SIGSEGV, framework_signal_handler);
    std::signal(SIGILL, framework_signal_handler);
    std::signal(SIGFPE, framework_signal_handler);
    std::signal(SIGTERM, framework_signal_handler);
    state.initialized = true;

    spdlog::info(
        "Framework logging initialized. app='{}', log='{}', error_log='{}'",
        app_name,
        state.log_file,
        state.error_log_file);
}

std::string framework_log_file()
{
    auto& state = get_logging_state();
    std::lock_guard<std::mutex> lock(state.mutex);
    return state.log_file;
}

std::string framework_error_log_file()
{
    auto& state = get_logging_state();
    std::lock_guard<std::mutex> lock(state.mutex);
    return state.error_log_file;
}

void log_exception_with_context(
    std::string_view context,
    const std::exception& exception)
{
    spdlog::error("{}\n{}", context, nested_exception_details(exception));
}

void log_current_exception_with_context(std::string_view context)
{
    spdlog::error("{}\n{}", context, current_exception_details());
}

RUZINO_NAMESPACE_CLOSE_SCOPE
