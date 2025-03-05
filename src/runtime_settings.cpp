#include "runtime_settings.hpp"

#include "util.hpp"

namespace asha
{

/**
 * @brief Convert string representation of log level to enum
 * 
 * @param log_level String representation of log level
 * @return Corresponding LogLevel enum value
 */
enum LogLevel str_to_log_level(const char* log_level)
{
    if (str_eq(log_level, "ERROR")) {
        return LogLevel::Error;
    } else if (str_eq(log_level, "INFO")) {
        return LogLevel::Info;
    } else if (str_eq(log_level, "SCAN")) {
        return LogLevel::Scan;
    } else if (str_eq(log_level, "AUDIO")) {
        return LogLevel::Audio;
    } else {
        return LogLevel::None;
    }
}

/**
 * @brief Initialize the runtime settings
 * 
 * Sets up the TLV (Tag-Length-Value) implementation for persistent storage
 */
void RuntimeSettings::init()
{
    btstack_tlv_get_instance(&tlv_impl, &tlv_ctx);
}

/**
 * @brief Load settings from flash
 * 
 * Gets settings from flash storage. Settings not found in flash
 * will be set to their default values. Default settings
 * are not written to flash to both conserve flash and
 * to make updating default settings easier.
 */
void RuntimeSettings::get_settings()
{
    // Load each setting, falling back to defaults if not found
    if (!get_tlv_tag(Tag::SerialUARTEnabled, serial_uart_enabled)) {
        serial_uart_enabled = false;
    }
    
    if (!get_tlv_tag(Tag::HCIDump, hci_dump_enabled)) {
        hci_dump_enabled = false;
    }
    
    if (!get_tlv_tag(Tag::FullSetPaired, full_set_paired)) {
        full_set_paired = false;
    }
    
    if (!get_tlv_tag(Tag::LogLevel, log_level)) {
        log_level = LogLevel::Info;
    }
}

/**
 * @brief Enable or disable UART serial
 * 
 * @param is_enabled Whether UART serial should be enabled
 * @return true if the setting was changed and stored, false otherwise
 */
bool RuntimeSettings::set_uart_enabled(bool is_enabled)
{
    // Only update if the value has changed
    if (serial_uart_enabled != is_enabled) {
        serial_uart_enabled = is_enabled;
        return store_tlv_tag(Tag::SerialUARTEnabled, serial_uart_enabled);
    }
    return false;
}

/**
 * @brief Enable or disable HCI dump
 * 
 * @param is_enabled Whether HCI dump should be enabled
 * @return true if the setting was changed and stored, false otherwise
 */
bool RuntimeSettings::set_hci_dump_enabled(bool is_enabled)
{
    // Only update if the value has changed
    if (hci_dump_enabled != is_enabled) {
        hci_dump_enabled = is_enabled;
        return store_tlv_tag(Tag::HCIDump, hci_dump_enabled);
    }
    return false;
}

/**
 * @brief Set whether a full set of hearing aids is paired
 * 
 * @param have_full_set Whether a full set is paired
 * @return true if the setting was changed and stored, false otherwise
 */
bool RuntimeSettings::set_full_set_paired(bool have_full_set)
{
    // Only update if the value has changed
    if (full_set_paired != have_full_set) {
        full_set_paired = have_full_set;
        return store_tlv_tag(Tag::FullSetPaired, have_full_set);
    }
    return false;
}

/**
 * @brief Set the log level
 * 
 * @param level The new log level
 * @return true if the setting was changed and stored, false otherwise
 */
bool RuntimeSettings::set_log_level(enum LogLevel level)
{
    // Only update if the value has changed
    if (log_level != level) {
        log_level = level;
        return store_tlv_tag(Tag::LogLevel, log_level);
    }
    return false;
}

} // namespace asha