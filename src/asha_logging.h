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

/**
 * @brief Maximum length of a log line
 */
constexpr size_t log_line_len = 160;

/**
 * @brief Maximum number of log lines in the buffer
 */
constexpr size_t log_lines = 128;

/**
 * @brief Async context for logging operations
 */
extern async_context_t *logging_ctx;

/**
 * @brief Worker for handling pending log operations
 */
extern async_when_pending_worker_t logging_pending_worker;

/**
 * @brief Worker function to process pending log messages
 * 
 * @param context The async context
 * @param worker The worker object
 */
void handle_logging_pending_worker(async_context_t *context, async_when_pending_worker_t *worker);

/**
 * @brief Circular buffer for storing log messages
 */
extern etl::circular_buffer<etl::string<log_line_len>, log_lines> log_buffer;

/**
 * @brief Log a message with the specified level
 * 
 * This template function handles formatting and buffering of log messages.
 * It supports both HCI dump mode and standard output mode.
 * 
 * @tparam Arg Variadic template for format arguments
 * @param level Log level for the message
 * @param fmt Format string
 * @param args Format arguments
 */
template <typename ... Arg>
static void asha_log(enum LogLevel level, const char* fmt, Arg...args)
{
    if (runtime_settings.hci_dump_enabled) {
        // Use HCI dump for logging when enabled
        int log_level = (level == LogLevel::Error) ? HCI_DUMP_LOG_LEVEL_ERROR : HCI_DUMP_LOG_LEVEL_INFO;
        hci_dump_log(log_level, fmt, log_level_to_str(level), to_ms_since_boot(get_absolute_time()), args...);
    } else {
        // Otherwise use the circular buffer for logging
        etl::string<log_line_len> line = {};
        int len = snprintf(line.data(), line.capacity(), fmt, log_level_to_str(level), to_ms_since_boot(get_absolute_time()), args...);
        if (len >= 0) {
            // Resize the string to the actual length (up to capacity)
            line.uninitialized_resize((size_t)len <= log_line_len ? (size_t)len : log_line_len);
            log_buffer.push(line);
            
            // Signal that there's work to be done
            if (logging_ctx) {
                async_context_set_work_pending(logging_ctx, &logging_pending_worker);
            }
        }
    }
}

/**
 * @brief Base macro for logging with level and format
 */
#define ASHA_LOG(level, fmt, ...) asha_log((level), "[%-5s : %u] " fmt "\n", ##__VA_ARGS__)

/**
 * @brief Log an error message
 */
#define LOG_ERROR(fmt, ...) if (runtime_settings.log_level >= LogLevel::Error) { ASHA_LOG(LogLevel::Error, fmt, ##__VA_ARGS__); }

/**
 * @brief Log an info message
 */
#define LOG_INFO(fmt, ...) if (runtime_settings.log_level >= LogLevel::Info) { ASHA_LOG(LogLevel::Info, fmt, ##__VA_ARGS__); }

/**
 * @brief Log a scan-related message
 */
#define LOG_SCAN(fmt, ...) if (runtime_settings.log_level >= LogLevel::Scan) { ASHA_LOG(LogLevel::Scan, fmt, ##__VA_ARGS__); }

/**
 * @brief Log an audio-related message
 */
#define LOG_AUDIO(fmt, ...) if (runtime_settings.log_level >= LogLevel::Audio) { ASHA_LOG(LogLevel::Audio, fmt, ##__VA_ARGS__); }

} // namespace asha
