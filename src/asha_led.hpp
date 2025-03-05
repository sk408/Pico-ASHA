#pragma once

#include <cstdint>

#include <pico/async_context.h>

namespace asha
{

/**
 * @brief Manages the onboard LED for status indication
 * 
 * This class provides functionality to control the onboard LED,
 * including static on/off states and blinking patterns.
 */
class LEDManager
{
public:
    /**
     * @brief LED pattern configuration
     * 
     * Defines a repeating bit pattern for LED blinking.
     * Each bit in the pattern represents the LED state (1=on, 0=off).
     */
    struct Pattern {
        // Number of bits in the sequence. Can be up to 32
        size_t len = 0;

        // Time between each bit in the sequence
        uint32_t interval_ms = 0;

        // Delay after last bit before restarting sequence
        uint32_t delay_ms = 0;

        // Sequence bit pattern. If bit is set, LED will be ON, otherwise
        // LED will be OFF
        uint32_t pattern = 0;

        // Current position in sequence
        size_t pos = 0;
    };

    /**
     * @brief Static LED states
     */
    enum class State { On, Off };

    /**
     * @brief Construct a new LEDManager
     */
    LEDManager();

    /**
     * @brief Set the async context for LED pattern scheduling
     * 
     * @param ctx Pointer to the async context
     */
    void set_ctx(async_context_t* ctx) {led_ctx = ctx;}
    
    /**
     * @brief Set the LED to a static state
     * 
     * @param led_state The desired LED state (On or Off)
     */
    void set_led(State led_state);
    
    /**
     * @brief Set the LED to display a blinking pattern
     * 
     * @param pattern The pattern configuration to display
     */
    void set_led_pattern(Pattern const& pattern);
    
private:
    /**
     * @brief Disable the current LED pattern
     */
    void disable_curr_led_pattern();
    
    /**
     * @brief Worker function to handle LED pattern updates
     * 
     * @param ctx The async context
     * @param worker The worker object
     */
    static void handle_led_work(async_context_t* ctx, async_at_time_worker_t *worker);
    
    // Async context for scheduling LED updates
    async_context_t *led_ctx = nullptr;
    
    // Worker for async LED pattern updates
    async_at_time_worker_t led_worker = {};
    
    // Current active pattern
    Pattern curr_pattern = {};
};

} // namespace asha