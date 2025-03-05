#include <stdio.h>
#include <pico/stdio.h>
#include <pico/multicore.h>
#include <pico/flash.h>
// TinyUSB
#include <bsp/board.h>
#include <tusb.h>

#include "asha_unique_id.hpp"
#include "asha_audio.hpp"
#include "asha_usb_serial.hpp"
#include "runtime_settings.hpp"
#include "asha_logging.h"
#include "hearing_aid.hpp"

namespace asha
{
    
AudioBuffer audio_buff;
async_context_t *bt_async_ctx = nullptr;
async_when_pending_worker_t bt_audio_pending_worker = {};

etl::string<stdin_str_size> curr_stdin_buff = {};
etl::string<stdin_str_size> complete_std_line = {};

async_context_t *usb_ser_ctx = nullptr;
async_when_pending_worker_t stdin_pending_worker = {};

char pico_uid[pico_uid_size];

RuntimeSettings runtime_settings = {};

etl::circular_buffer<etl::string<log_line_len>, log_lines> log_buffer = {};
async_context_t *logging_ctx = nullptr;
async_when_pending_worker_t logging_pending_worker = {};

HearingAid ha_1 = {};
HearingAid ha_2 = {};

// Forward declarations
extern "C" void bt_main();
void usb_main();

/**
 * @brief Initialize flash for safe multicore operation
 * 
 * Initializes flash operations to be safe with multicore processing.
 * This is necessary because BTStack saves pairing info to flash.
 */
static void init_flash_for_multicore() {
    flash_safe_execute_core_init();
}

/**
 * @brief Initialize global variables and buffers
 */
static void init_globals() {
    patom::PseudoAtomicInit();
    audio_buff.init();
    
    // Get and format unique board ID
    pico_get_unique_board_id_string(pico_uid, sizeof pico_uid);
    const char uid_suffix[4] = {'-', 'C', 'D', 'C'};
    memcpy(pico_uid + (sizeof(pico_uid) - sizeof(uid_suffix) - 1), uid_suffix, sizeof(uid_suffix));
}

/**
 * @brief Initialize USB and TinyUSB stack
 */
static void init_usb() {
    // Init TinyUSB before stdio init
    board_init();
    // init device stack on configured roothub port
    tud_init(BOARD_TUD_RHPORT);
}

extern "C" int main()
{
    // Initialize components in the correct order
    init_flash_for_multicore();
    init_globals();
    init_usb();

    stdio_init_all();
    sleep_ms(250);
    
    // Launch Bluetooth processing on core 1
    multicore_launch_core1(bt_main);

    sleep_ms(250);
    
    // Run USB processing on core 0
    usb_main();
    
    // This should never be reached in normal operation
    while(1) {
        sleep_ms(1000);
    }
}

} // namespace asha