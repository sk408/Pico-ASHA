#pragma once

#include <stdio.h>

#include <etl/circular_buffer.h>
#include <etl/string.h>

#include <pico/time.h>
#include <pico/async_context.h>

#include <hci_dump_embedded_stdout.h>
#include <btstack_debug.h>
#include "runtime_settings.hpp"

namespace asha
{

constexpr size_t log_line_len = 160;
constexpr size_t log_lines = 128;

// Use the LogLevel enum from runtime_settings.hpp
// Do not redefine it here
extern enum LogLevel log_level;

extern async_context_t *logging_ctx;
extern async_when_pending_worker_t logging_pending_worker;

void handle_logging_pending_worker(async_context_t *context, async_when_pending_worker_t *worker);

extern etl::circular_buffer<etl::string<log_line_len>, log_lines> log_buffer;

// New function to check if a given manufacturer ID has known issues
bool has_known_issues(uint16_t manufacturer_id);

// New function to get manufacturer name from ID
const char* get_manufacturer_name(uint16_t manufacturer_id);

// Log a compatibility issue with a specific manufacturer
void log_compatibility_issue(uint16_t manufacturer_id, const char* component, const char* format, ...);

template <typename ... Arg>
static void asha_log(enum LogLevel level, const char* fmt, Arg...args)
{
    if (runtime_settings.hci_dump_enabled) {
        int log_level = (level == Error) ? HCI_DUMP_LOG_LEVEL_ERROR : HCI_DUMP_LOG_LEVEL_INFO;
        hci_dump_log(log_level, fmt, log_level_to_str(level), to_ms_since_boot(get_absolute_time()), args...);
    } else {
        etl::string<log_line_len> line = {};
        int len = snprintf(line.data(), line.capacity(), fmt, log_level_to_str(level), to_ms_since_boot(get_absolute_time()), args...);
        if (len >= 0) {
            line.uninitialized_resize((size_t)len <= log_line_len ? (size_t)len : log_line_len);
            log_buffer.push(line);
            if (logging_ctx) {
                async_context_set_work_pending(logging_ctx, &logging_pending_worker);
            }
            //printf(fmt, log_level_to_str(level), to_ms_since_boot(get_absolute_time()), args...);
        }
    }
}

// Updated logging macros
#define LOG_ERROR(format, ...)   asha_log(Error,   "ERROR",   format, ##__VA_ARGS__)
#define LOG_WARN(format, ...)    asha_log(Warning, "WARNING", format, ##__VA_ARGS__)
#define LOG_INFO(format, ...)    asha_log(Info,    "INFO",    format, ##__VA_ARGS__)
#define LOG_DEBUG(format, ...)   asha_log(Debug,   "DEBUG",   format, ##__VA_ARGS__)
#define LOG_VERBOSE(format, ...) asha_log(Verbose, "VERBOSE", format, ##__VA_ARGS__)
#define LOG_SCAN(format, ...)    asha_log(Scan,    "SCAN",    format, ##__VA_ARGS__)
#define LOG_COMPAT(format, ...)  asha_log(Compat,  "COMPAT",  format, ##__VA_ARGS__)
#define LOG_MFG(format, ...)     asha_log(Mfg,     "MFG",     format, ##__VA_ARGS__)

void print_log_msg(LogLevel lvl, const char* prefix, const char* format, ...);

// Make this declaration match the one in runtime_settings.hpp
constexpr const char* log_level_to_str(enum LogLevel lvl);

enum LogLevel str_to_log_level(const char* str);

} // namespace asha
