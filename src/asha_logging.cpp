#include <pico/stdio_usb.h>

#include "runtime_settings.hpp"
#include "asha_logging.h"

namespace asha
{

/**
 * @brief Worker function to process pending log messages
 * 
 * This function is called by the async context when there are pending log
 * messages to be processed. It outputs log messages from the buffer to
 * the appropriate output streams (USB or UART) when they are available.
 * 
 * @param context The async context
 * @param worker The worker object
 */
void handle_logging_pending_worker([[maybe_unused]] async_context_t *context, [[maybe_unused]] async_when_pending_worker_t *worker)
{
    // Process all buffered log messages if any output is available
    while(!log_buffer.empty() && (runtime_settings.serial_uart_enabled || stdio_usb_connected())) {
        printf("%s", log_buffer.front().c_str());
        log_buffer.pop();
    }
}

} //namespace asha