#include <pico/stdio_usb.h>

#include "runtime_settings.hpp"
#include "asha_logging.h"
#include "async_context.h"

namespace asha
{

// Define a structure to store manufacturer information
struct ManufacturerInfo {
    uint16_t id;
    const char* name;
    bool known_issues;
    const char* issues_description;
};

// Initialize with known manufacturer information
// Reference: https://www.bluetooth.com/specifications/assigned-numbers/company-identifiers/
static const ManufacturerInfo manufacturers[] = {
    { 0x001D, "Oticon", false, nullptr },
    { 0x0036, "Starkey", true, "Issues with L2CAP CoC connection" },
    { 0x000D, "Phonak/Sonova", true, "May use non-standard ROP format" },
    { 0x0031, "ReSound", true, "Potential issues with L2CAP PSM value" },
    { 0x0042, "Widex", true, "May require different security level" },
    { 0x004C, "Sivantos/Signia", true, "May not support LE CoC correctly" },
    // Add other manufacturers as they are tested
    { 0x0000, "Unknown", false, nullptr }  // Default entry
};

LogLevel log_level = Info;  // Default log level

async_context_t *logging_ctx = nullptr;
async_when_pending_worker_t logging_pending_worker;
etl::circular_buffer<etl::string<log_line_len>, log_lines> log_buffer;

bool has_known_issues(uint16_t manufacturer_id) {
    for (const auto& mfg : manufacturers) {
        if (mfg.id == manufacturer_id) {
            return mfg.known_issues;
        }
    }
    return false;
}

const char* get_manufacturer_name(uint16_t manufacturer_id) {
    for (const auto& mfg : manufacturers) {
        if (mfg.id == manufacturer_id) {
            return mfg.name;
        }
    }
    return "Unknown Manufacturer";
}

const char* get_known_issues_description(uint16_t manufacturer_id) {
    for (const auto& mfg : manufacturers) {
        if (mfg.id == manufacturer_id && mfg.known_issues) {
            return mfg.issues_description;
        }
    }
    return "No known issues";
}

void log_compatibility_issue(uint16_t manufacturer_id, const char* component, const char* format, ...) {
    if (log_level < Compat) {
        return;
    }

    char buffer[256];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    const char* mfg_name = get_manufacturer_name(manufacturer_id);
    
    print_log_msg(Compat, "COMPAT", 
                  "[%s - Mfg: %s (0x%04X)] %s: %s", 
                  component, mfg_name, manufacturer_id, 
                  has_known_issues(manufacturer_id) ? "KNOWN ISSUE" : "NEW ISSUE",
                  buffer);
}

void print_log_msg(LogLevel lvl, const char* prefix, const char* format, ...) {
    if (log_level < lvl) {
        return;
    }

    char buffer[log_line_len];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    char timestamp[16];
    uint32_t ms = to_ms_since_boot(get_absolute_time());
    snprintf(timestamp, sizeof(timestamp), "%lu.%03lu", ms / 1000, ms % 1000);

    char final_buffer[log_line_len];
    snprintf(final_buffer, sizeof(final_buffer), "[%s] [%-7s] %s", 
             timestamp, prefix, buffer);

    // Print to standard output
    printf("%s\n", final_buffer);

    // Store in log buffer
    log_buffer.push(final_buffer);

    // Signal the async context that we have new log data
    if (logging_ctx) {
        async_context_set_work_pending(logging_ctx, &logging_pending_worker);
    }
}

void handle_logging_pending_worker(async_context_t *context, async_when_pending_worker_t *worker)
{
    while(!log_buffer.empty() && (runtime_settings.serial_uart_enabled || stdio_usb_connected())) {
        printf("%s", log_buffer.front().c_str());
        log_buffer.pop();
    }
}

enum LogLevel str_to_log_level(const char* str) {
    if (strcasecmp(str, "NONE") == 0)     return None;
    if (strcasecmp(str, "ERROR") == 0)    return Error;
    if (strcasecmp(str, "WARNING") == 0)  return Warning;
    if (strcasecmp(str, "INFO") == 0)     return Info;
    if (strcasecmp(str, "DEBUG") == 0)    return Debug;
    if (strcasecmp(str, "VERBOSE") == 0)  return Verbose;
    if (strcasecmp(str, "SCAN") == 0)     return Scan;
    if (strcasecmp(str, "COMPAT") == 0)   return Compat;
    if (strcasecmp(str, "MFG") == 0)      return Mfg;
    return None;
}

} //namespace asha