#include <bitset>
#include <pico/cyw43_arch.h>

#include "asha_led.hpp"

namespace asha
{

/**
 * @brief Construct a new LEDManager
 * 
 * Initializes the LED worker with this instance as user data
 * and sets the work handler function.
 */
LEDManager::LEDManager()
{
    led_worker.user_data = this;
    led_worker.do_work = &LEDManager::handle_led_work;
}

/**
 * @brief Set the LED to a static state (on or off)
 * 
 * Disables any running pattern and sets the LED to the specified state.
 * 
 * @param led_state The desired LED state (On or Off)
 */
void LEDManager::set_led(LEDManager::State led_state)
{
    // Stop any running pattern first
    disable_curr_led_pattern();
    
    // Set the LED state directly
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, (led_state == State::On));
}

/**
 * @brief Set the LED to display a blinking pattern
 * 
 * Disables any running pattern and starts a new pattern.
 * The pattern is a bit sequence where 1 means LED on and 0 means LED off.
 * 
 * @param pattern The pattern configuration to display
 */
void LEDManager::set_led_pattern(LEDManager::Pattern const& pattern)
{
    // Validate pattern and context
    if (pattern.len > 32 || !led_ctx) return;
    
    // Stop any running pattern
    disable_curr_led_pattern();
    
    // Set the new pattern and schedule the first update
    curr_pattern = pattern;
    async_context_add_at_time_worker_in_ms(led_ctx, &led_worker, 0);
}

/**
 * @brief Worker function to handle LED pattern updates
 * 
 * This static function is called by the async context to update
 * the LED state according to the current pattern.
 * 
 * @param ctx The async context
 * @param worker The worker object containing the LEDManager instance
 */
void LEDManager::handle_led_work(async_context_t* ctx, async_at_time_worker_t *worker)
{
    // Get the LEDManager instance from worker user data
    LEDManager *this_ = static_cast<LEDManager*>(worker->user_data);
    Pattern& p = this_->curr_pattern;
    
    // Convert pattern to bitset for easier bit access
    std::bitset<32> pattern_bitset(p.pattern);
    
    // Set LED according to current position in pattern
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, pattern_bitset[p.pos]);
    
    // Move to next position in pattern
    ++p.pos;
    
    // If we reached the end of the pattern, reset position and add delay
    if (p.pos >= p.len) {
        p.pos = 0;
        async_context_add_at_time_worker_in_ms(ctx, worker, p.interval_ms + p.delay_ms);
    } else {
        // Otherwise just wait for the next interval
        async_context_add_at_time_worker_in_ms(ctx, worker, p.interval_ms);
    }
}

/**
 * @brief Disable the current LED pattern
 * 
 * Removes the LED worker from the async context to stop pattern updates.
 */
void LEDManager::disable_curr_led_pattern()
{
    if (led_ctx) {
        async_context_remove_at_time_worker(led_ctx, &led_worker);
    }
}

} // namespace asha