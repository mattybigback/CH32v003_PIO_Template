/*
 * lcd_pcf8574.c – minimal HD44780 driver over PCF8574 for CH32V003
 * ---------------------------------------------------------------------------
 * Pure‑C, ≤1 kB flash when linked against ADBeta's "CH32V003_lib_i2c" v5.4.
 * No direct register twiddling – all I²C traffic goes through the lib_i2c
 * wrappers so you keep one tested code path for every device on the bus.
 *
 * 2025‑07‑30 – adapted to lib_i2c API per user request.
 *
 * ---------------------------------------------------------------------------
 * External deps
 *   • ch32_lib_i2c.h  ( https://github.com/ADBeta/CH32V003_lib_i2c )
 *   • a µs delay helper – either ch32fun's Delay_Us() or your own.
 *
 * Public API (LiquidCrystal subset)
 *   void lcd_init(uint8_t cols, uint8_t rows);
 *   void lcd_clear(void);
 *   void lcd_home(void);
 *   void lcd_set_cursor(uint8_t col, uint8_t row);
 *   void lcd_write_char(char c);
 *   void lcd_print(const char *s);
 *   void lcd_set_backlight(uint8_t on);
 */

#include <stdint.h>
#include "lib_i2c.h"

/* ───── User‑configurable constants ───────────────────────────────────────*/
#ifndef LCD_I2C_ADDR
#define LCD_I2C_ADDR 0x27
#endif
#ifndef LCD_I2C_SPEED
#define LCD_I2C_SPEED I2C_CLK_100KHZ
#endif
#ifndef LCD_BUSY_POLL
#define LCD_BUSY_POLL 1          /* 1 = busy‑flag, 0 = 50 µs delay */
#endif
#ifndef LCD_DELAY_US
#define LCD_DELAY_US(x)   Delay_Us(x)   /* ch32fun micro‑second delay */
#endif

/* ───── lib_i2c device descriptor for the PCF8574 backpack ─────────────── */
static i2c_device_t pcf_dev = {
    .clkr = LCD_I2C_SPEED,
    .type = I2C_ADDR_7BIT,
    .addr = LCD_I2C_ADDR,
    .regb = 0,
    .tout = 100000
};

/* ───── Bit‑positions in PCF8574 byte (verify your board!) ─────────────── */
#define PIN_RS 0
#define PIN_RW 1
#define PIN_E  2
#define PIN_BL 3
#define PIN_D4 4
#define BM(x) (1U << (x))

static uint8_t shadow = BM(PIN_BL);   /* cached output – backlight on */

/* ───── Thin i2c wrappers (1‑byte TX / RX) ─────────────────────────────── */
static inline void pcf_write(uint8_t v)
{
    shadow = v;
    i2c_write_raw(&pcf_dev, &v, 1);
}

static inline uint8_t pcf_read(void)
{
    uint8_t b;
    i2c_read_raw(&pcf_dev, &b, 1);
    return b;
}

/* ───── LCD low‑level helpers ──────────────────────────────────────────── */
static inline void lcd_toggle_e(void)
{
    pcf_write(shadow | BM(PIN_E));
    pcf_write(shadow & ~BM(PIN_E));
}

static void lcd_send_nibble(uint8_t nibble)
{
    pcf_write((shadow & 0x0F) | (nibble & 0xF0));
    lcd_toggle_e();
}

/* ───── Busy‑flag read ----------------------------------------------------- */
static uint8_t lcd_read_status(void)
{
    uint8_t base = BM(PIN_RW) | (shadow & BM(PIN_BL));
    pcf_write(base | 0xF0);
    /* high nibble */
    pcf_write(base | 0xF0 | BM(PIN_E));
    uint8_t hi = pcf_read();
    pcf_write(base | 0xF0);
    /* low nibble */
    pcf_write(base | 0xF0 | BM(PIN_E));
    uint8_t lo = pcf_read();
    pcf_write(base | 0xF0);
    return (hi & 0xF0) | ((lo & 0xF0) >> 4);
}

static void lcd_wait_ready(void)
{
#if LCD_BUSY_POLL
    while (lcd_read_status() & 0x80) { }
#else
    LCD_DELAY_US(50);
#endif
}

/* ───── Send byte (cmd=0 / data=1) --------------------------------------- */
static void lcd_send(uint8_t val, uint8_t rs)
{
    lcd_wait_ready();
    shadow = (shadow & ~(BM(PIN_RS)|BM(PIN_RW))) | (rs?BM(PIN_RS):0);
    pcf_write(shadow & ~BM(PIN_RW));      /* write mode */

    lcd_send_nibble(val & 0xF0);
    lcd_send_nibble((val << 4) & 0xF0);
}

/* ───── Public primitives ------------------------------------------------- */
static uint8_t _cols, _rows;

void lcd_command(uint8_t c)       { lcd_send(c, 0); }
void lcd_write_char(char c)       { lcd_send((uint8_t)c, 1); }

void lcd_clear(void)              { lcd_command(0x01); }
void lcd_home(void)               { lcd_command(0x02); }

void lcd_set_cursor(uint8_t col, uint8_t row)
{
    static const uint8_t offs[] = {0x00,0x40,0x14,0x54};
    if(row>=_rows) row=_rows-1;
    lcd_command(0x80 | (col + offs[row]));
}

void lcd_print(const char *s)     { while(*s) lcd_write_char(*s++); }

void lcd_set_backlight(uint8_t on){ if(on) shadow|=BM(PIN_BL); else shadow&=~BM(PIN_BL); pcf_write(shadow); }

/* ───── Power‑up 4‑bit init sequence ------------------------------------- */
static void lcd_4bit_wakeup(void)
{
    /* everything low except BL */
    pcf_write(shadow);
    LCD_DELAY_US(15000);
    for(int i=0;i<3;i++){ lcd_send_nibble(0x30); LCD_DELAY_US(4100);} /* 8‑bit cmd */
    lcd_send_nibble(0x20); LCD_DELAY_US(100);                         /* to 4‑bit */

    lcd_command(0x28); /* 4‑bit, 2‑line */
    lcd_command(0x08); /* display off */
    lcd_clear();
    lcd_command(0x06); /* entry inc */
    lcd_command(0x0C); /* display on, cursor off */
}

void lcd_init(uint8_t cols, uint8_t rows)
{
    _cols = cols; _rows = rows;

    /* one‑time bus bring‑up (idempotent) */
    static uint8_t init_done = 0;
    if(!init_done){
        i2c_init(&pcf_dev);            /* sets pins per lib settings */
        init_done = 1;
    }
    lcd_4bit_wakeup();
}

/* ───── Stub if user forgot delay_us ------------------------------------- */
__attribute__((weak)) void delay_us(uint32_t us){ for(volatile uint32_t i=0;i<us*8;i++); }
