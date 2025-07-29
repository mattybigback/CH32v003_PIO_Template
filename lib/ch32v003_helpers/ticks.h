/*
Reusable library that provides millisecond and microsecond tick counting
for the CH32V003 series of microcontrollers.

Based on example code from the ch32fun github repo
https://github.com/cnlohr/ch32fun/tree/master/examples/systick_irq
*/
#ifndef TICKS_H_
#define TICKS_H_

#include <stdint.h>
#include "ch32v003fun.h"   /* Needed for FUNCONF_SYSTEM_CORE_CLOCK and SysTick regs */

#ifdef __cplusplus
extern "C" {
#endif

//Clock‑derived constants
#ifdef FUNCONF_SYSTEM_CORE_CLOCK
#define SYSTICK_ONE_MILLISECOND ((uint32_t)(FUNCONF_SYSTEM_CORE_CLOCK) / 1000U)
#define SYSTICK_ONE_MICROSECOND ((uint32_t)(FUNCONF_SYSTEM_CORE_CLOCK) / 1000000U)
#else
#error "FUNCONF_SYSTEM_CORE_CLOCK must be defined to use millis() and micros()."
#endif

// User‑facing convenience macros
// millis() wraps every ~49 days (32‑bit overflow);
#define millis() (systick_millis)

// micros() wraps every ~90 s at 48 MHz because SysTick->CNT is only 24 bits.
#define micros() (SysTick->CNT / SYSTICK_ONE_MICROSECOND)

#ifndef FUNCONF_SYSTICK_USE_HCLK
#warning "FUNCONF_SYSTICK_USE_HCLK is not defined. This may cause micros() to return incorrect values. Define FUNCONF_SYSTICK_USE_HCLK as 1 in funconfig.h if you are using micros()."
#elif FUNCONF_SYSTICK_USE_HCLK != 1
#warning "FUNCONF_SYSTICK_USE_HCLK is not set to 1. This may cause micros() to return incorrect values. Set FUNCONF_SYSTICK_USE_HCLK to 1 in funconfig.h if you are using micros()."
#endif

// Public data

// Incremented in SysTick_Handler every millisecond.
extern volatile uint32_t systick_millis;

// Public functions

// Call once after clocks are configured.
void millis_init(void);

#ifdef __cplusplus
}
#endif

#endif /* TICKS_H_ */
