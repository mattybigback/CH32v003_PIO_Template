/* ===========================================================================
 * pcf8563.h – PCF8563 RTC driver (CH32V003 + ADBeta lib_i2c, ch32fun style)
 * =========================================================================== */
#ifndef PCF8563_H
#define PCF8563_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ───── Build‑time configuration (override via -D…) -------------------------- */
#ifndef PCF8563_I2C_ADDR
#define PCF8563_I2C_ADDR   0x51      /* 7‑bit */
#endif
#ifndef PCF8563_I2C_SPEED
#define PCF8563_I2C_SPEED  I2C_CLK_100KHZ
#endif
#ifndef PCF8563_DELAY_US
#define PCF8563_DELAY_US(x) Delay_Us(x) /* ch32fun micro‑second delay */
#endif
#ifndef PCF8563_BASE_YEAR
#define PCF8563_BASE_YEAR  2000      /* map 0..99 to BASE_YEAR..+99 */
#endif

/* ───── Public types --------------------------------------------------------- */
typedef struct {
    uint8_t sec, min, hour, day, wday, mon; /* BCD‑decoded */
    uint16_t year;                          /* full year */
    uint8_t vl;                             /* seconds.VL flag */
    uint8_t century;                        /* month.C bit */
} pcf8563_time_t;

/* CLKOUT selections */
#define PCF8563_CLKOUT_32768HZ 0
#define PCF8563_CLKOUT_1024HZ  1
#define PCF8563_CLKOUT_32HZ    2
#define PCF8563_CLKOUT_1HZ     3

/* Timer sources (TD) */
#define PCF8563_TCLK_4096HZ    0
#define PCF8563_TCLK_64HZ      1
#define PCF8563_TCLK_1HZ       2
#define PCF8563_TCLK_1_PER_60  3

/* ───── API ------------------------------------------------------------------ */
void pcf8563_init(void);
void pcf8563_stop(uint8_t stop);
void pcf8563_get_time(pcf8563_time_t *t);
void pcf8563_set_time(const pcf8563_time_t *t);
void pcf8563_set_alarm(int8_t min, int8_t hour, int8_t day, int8_t wday);
void pcf8563_irq_config(uint8_t alarm_en, uint8_t timer_en, uint8_t pulse_mode);
void pcf8563_get_clear_flags(uint8_t *af, uint8_t *tf);
void pcf8563_clkout(uint8_t enable, uint8_t sel);
void pcf8563_timer_stop(void);
void pcf8563_timer_start(uint8_t src, uint8_t value);
void pcf8563_read(uint8_t reg, uint8_t *buf, uint16_t n);
void pcf8563_write(uint8_t reg, const uint8_t *buf, uint16_t n);
uint8_t pcf8563_vl(void);

#ifdef __cplusplus
}
#endif

#endif /* PCF8563_H */