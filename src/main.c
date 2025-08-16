/*  Template PlatformIO project for ch32fun SDK
    Includes various helper functions and libraries
*/

#include "main.h"
#include "lcd_pcf8574.h" // For LCD functions
#include "pcf8563.h"     // For RTC functions
#include "ticks.h"       // For millis/micros
#include <stdio.h>
#include <stdbool.h>     // For bool type

pcf8563_time_t now;

int main(void) {
    // Setup
    SystemInit();  // bring HCLK up to configured frequency
    millis_init(); // initialize SysTick for millisecond ticks
    
    // Initialize peripherals
    funGpioInitAll(); // enable GPIO clocks
    lcd_init(16, 2);
    pcf8563_init();
    if (pcf8563_vl()) {
        // Time contents are invalid; set them once.
        pcf8563_time_t t = {.year = 2000, .mon = 1, .day = 1, .wday = 1, .hour = 0, .min = 0, .sec = 0, .century = 0};
        pcf8563_set_time(&t);
    }

    funPinMode(PC4, GPIO_Speed_10MHz | GPIO_CNF_OUT_PP); // Set PC4 as output for LED

    // Main loop

    while (1) {

        char time_str[17] = {0};
        char date_str[17] = {0};
        static uint32_t last_tick = 0;
        
        if (millis() - last_tick >= 500) { // Update every 500 ms
            last_tick += 500;  // Use += instead of = to avoid drift
            
            funDigitalWrite(PC4, !funDigitalRead(PC4)); // Toggle LED state
            
            // Simple approach: try operations, they'll handle timeouts internally
            // The device libraries should be modified to include proper error handling
            
            // Try RTC operation
            pcf8563_get_time(&now);
            
            // Format the time
            snprintf(date_str, sizeof(date_str), "Date: %04u-%02u-%02u", now.year, now.mon, now.day);
            snprintf(time_str, sizeof(time_str), "Time: %02u:%02u:%02u", now.hour, now.min, now.sec);
            
            // Try LCD operations
            lcd_set_cursor(0, 0);
            lcd_print(date_str);
            lcd_set_cursor(0, 1);
            lcd_print(time_str);
        }
    }
}
