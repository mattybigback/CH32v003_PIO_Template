/*  Template PlatformIO project for ch32fun SDK
  	Includes various helper functions and libraries
*/

#include "main.h"
#include "ticks.h"              // For millis/micros

int main(void) {
    // Setup
    SystemInit();               // bring HCLK up to 48 MHz
    millis_init();              // initialize SysTick for millisecond ticks
    funGpioInitAll();           // enable GPIO clocks

    // Main loop
    while (1) {

    }
}
