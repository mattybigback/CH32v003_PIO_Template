/*  Template PlatformIO project for ch32fun SDK
    Includes various helper functions and libraries
*/

#include "main.h"
#include "lcd_pcf8574.h" // For LCD functions
#include "pcf8563.h"     // For RTC functions
#include "ticks.h"       // For millis/micros
#include <stdio.h>

pcf8563_time_t now;

int main(void) {
    // Setup
    SystemInit();  // bring HCLK up to 48 MHz
    millis_init(); // initialize SysTick for millisecond ticks
    // Initialize RTC
    funGpioInitAll(); // enable GPIO clocks
    lcd_init(16, 2);
    pcf8563_init();
    if (pcf8563_vl()) {
        // Time contents are invalid; set them once.
        pcf8563_time_t t = {.year = 2025, .mon = 8, .day = 20, .wday = 2, .hour = 5, .min = 7, .sec = 20, .century = 0};
        pcf8563_set_time(&t);
    }
    uint8_t ctrl1 = 0, ctrl2 = 0, sec = 0;
    pcf8563_read(0x00, &ctrl1, 1);
    pcf8563_read(0x01, &ctrl2, 1);
    pcf8563_read(0x02, &sec, 1);
    pcf8563_get_time(&now);

    // Main loop

    while (1) {

        char time_str[17] = {0};
        char date_str[17] = {0};
        static uint32_t last_tick = 0;
        if (millis() - last_tick >= 200) { // Update every 200 ms
            last_tick += 200;
            pcf8563_get_time(&now);
            snprintf(date_str, sizeof(date_str), "Date: %04u-%02u-%02u", now.year, now.mon, now.day);
            snprintf(time_str, sizeof(time_str), "Time: %02u:%02u:%02u", now.hour, now.min, now.sec);
            lcd_set_cursor(0, 0); // Set cursor to the first line
            lcd_print(date_str);
            lcd_set_cursor(0, 1); // Set cursor to the second line
            lcd_print(time_str);
        }
    }
}
