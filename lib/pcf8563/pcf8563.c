/* ===========================================================================
 * pcf8563.c – implementation
 * =========================================================================== */
#include "pcf8563.h"
#include "lib_i2c.h"

/* ───── lib_i2c device descriptor ------------------------------------------- */
static i2c_device_t _rtc_dev = {
    .clkr = PCF8563_I2C_SPEED,
    .type = I2C_ADDR_7BIT,
    .addr = PCF8563_I2C_ADDR,
    .regb = 1,
    .tout = 100000
};

/* ───── Register map / bits -------------------------------------------------- */
#define REG_CTRL1       0x00
#define REG_CTRL2       0x01
#define REG_VL_SECONDS  0x02
#define REG_MINUTES     0x03
#define REG_HOURS       0x04
#define REG_DAYS        0x05
#define REG_WEEKDAYS    0x06
#define REG_C_MONTHS    0x07
#define REG_YEARS       0x08
#define REG_M_ALARM     0x09
#define REG_H_ALARM     0x0A
#define REG_D_ALARM     0x0B
#define REG_W_ALARM     0x0C
#define REG_CLKOUT      0x0D
#define REG_TMR_CTRL    0x0E
#define REG_TMR         0x0F

#define CTRL1_TEST1 (1u<<7)
#define CTRL1_STOP  (1u<<5)
#define CTRL1_TESTC (1u<<3)

#define CTRL2_TI_TP (1u<<4)
#define CTRL2_AF    (1u<<3)
#define CTRL2_TF    (1u<<2)
#define CTRL2_AIE   (1u<<1)
#define CTRL2_TIE   (1u<<0)

#define SEC_VL      (1u<<7)
#define MON_C       (1u<<7)
#define ALM_AE      (1u<<7)
#define CLK_FE      (1u<<7)

/* ───── BCD helpers ---------------------------------------------------------- */
static inline uint8_t _to_bcd(uint8_t v){ return (uint8_t)(((v/10u)<<4)|(v%10u)); }
static inline uint8_t _from_bcd(uint8_t b){ return (uint8_t)(((b>>4)*10u) + (b&0x0Fu)); }
/* ───── I²C helpers (register‑based transfers) ------------------------------- */
static inline void _i2c_read(uint8_t reg, uint8_t *buf, uint16_t n)
{
    /* Use register-based helper to ensure write(reg) + repeated-start + read */
    (void)i2c_read_reg(&_rtc_dev, (uint32_t)reg, buf, (size_t)n);
}

static inline void _i2c_write(uint8_t reg, const uint8_t *buf, uint16_t n)
{
    /* Use register-based helper for atomic reg+payload transfer */
    (void)i2c_write_reg(&_rtc_dev, (uint32_t)reg, buf, (size_t)n);
}

/* ───── API ----------------------------------------------------------------- */
/* Return 1 if the Voltage Low (VL) flag is set in seconds register, else 0. */
uint8_t pcf8563_vl(void)
{
    uint8_t s = 0;
    _i2c_read(REG_VL_SECONDS, &s, 1);
    return (uint8_t)((s & SEC_VL) ? 1 : 0);
}

void pcf8563_init(void)
{
    static uint8_t init_done = 0;
    if(!init_done){
        i2c_init(&_rtc_dev);
        init_done = 1;
    }
    /* CTRL1: TEST bits 0, STOP=0. CTRL2: clear flags, IRQs disabled. */
    uint8_t v1 = 0x00; _i2c_write(REG_CTRL1, &v1, 1);
    uint8_t v2 = 0x00; _i2c_write(REG_CTRL2, &v2, 1);
}

void pcf8563_stop(uint8_t stop)
{
    uint8_t c1; _i2c_read(REG_CTRL1, &c1, 1);
    c1 &= (uint8_t)~(CTRL1_TEST1|CTRL1_TESTC);
    if(stop){ c1 |= CTRL1_STOP; } else { c1 &= (uint8_t)~CTRL1_STOP; }
    _i2c_write(REG_CTRL1, &c1, 1);
}

void pcf8563_get_time(pcf8563_time_t *t)
{
    uint8_t b[7];
    _i2c_read(REG_VL_SECONDS, b, 7);
    t->vl   = (uint8_t)((b[0] & SEC_VL) ? 1 : 0);
    t->sec  = _from_bcd((uint8_t)(b[0] & 0x7Fu));
    t->min  = _from_bcd((uint8_t)(b[1] & 0x7Fu));
    t->hour = _from_bcd((uint8_t)(b[2] & 0x3Fu));
    t->day  = _from_bcd((uint8_t)(b[3] & 0x3Fu));
    t->wday = (uint8_t)(b[4] & 0x07u);
    t->century = (uint8_t)((b[5] & MON_C) ? 1 : 0);
    t->mon  = _from_bcd((uint8_t)(b[5] & 0x1Fu));
    uint8_t y = _from_bcd(b[6]);
#if PCF8563_BASE_YEAR
    t->year = (uint16_t)(PCF8563_BASE_YEAR + (t->century?100:0) + y);
#else
    t->year = y;
#endif
}

void pcf8563_set_time(const pcf8563_time_t *t)
{
    if(t->sec>59||t->min>59||t->hour>23||t->day==0||t->day>31||t->mon==0||t->mon>12||t->wday>6)
        return;

    pcf8563_stop(1);
    uint8_t b[7];
    b[0] = (uint8_t)(_to_bcd(t->sec) & 0x7Fu);
    b[1] = (uint8_t)(_to_bcd(t->min) & 0x7Fu);
    b[2] = (uint8_t)(_to_bcd(t->hour) & 0x3Fu);
    b[3] = (uint8_t)(_to_bcd(t->day) & 0x3Fu);
    b[4] = (uint8_t)(t->wday & 0x07u);
    uint8_t mon = (uint8_t)(_to_bcd(t->mon) & 0x1Fu);
    if(t->century) mon |= MON_C;
    b[5] = mon;
#if PCF8563_BASE_YEAR
    uint8_t y = (uint8_t)(((t->year>=PCF8563_BASE_YEAR)?(t->year-PCF8563_BASE_YEAR):t->year)%100);
#else
    uint8_t y = (uint8_t)(t->year%100);
#endif
    b[6] = _to_bcd(y);
    _i2c_write(REG_VL_SECONDS, b, 7);
    pcf8563_stop(0);
}

static inline uint8_t _enc_alarm(int8_t v, uint8_t mask)
{ return (v<0)?ALM_AE:(uint8_t)(_to_bcd((uint8_t)v)&mask); }

void pcf8563_set_alarm(int8_t min, int8_t hour, int8_t day, int8_t wday)
{
    if((min>=0&&min>59)||(hour>=0&&hour>23)||(day>=0&&(day==0||day>31))||(wday>=0&&wday>6))
        return;
    uint8_t b[4];
    b[0] = _enc_alarm(min,  0x7F);
    b[1] = _enc_alarm(hour, 0x3F);
    b[2] = _enc_alarm(day,  0x3F);
    b[3] = (wday<0)?ALM_AE:(uint8_t)(wday&0x07u);
    _i2c_write(REG_M_ALARM, b, 4);
}

void pcf8563_irq_config(uint8_t alarm_en, uint8_t timer_en, uint8_t pulse_mode)
{
    uint8_t c2; _i2c_read(REG_CTRL2, &c2, 1);
    c2 &= (uint8_t)~(CTRL2_AIE|CTRL2_TIE|CTRL2_TI_TP);
    if(alarm_en){ c2 |= CTRL2_AIE; }
    if(timer_en){ c2 |= CTRL2_TIE; }
    if(pulse_mode){ c2 |= CTRL2_TI_TP; }
    _i2c_write(REG_CTRL2, &c2, 1);
}

void pcf8563_get_clear_flags(uint8_t *af, uint8_t *tf)
{
    uint8_t c2; _i2c_read(REG_CTRL2, &c2, 1);
    if(af) *af = (uint8_t)((c2 & CTRL2_AF)?1:0);
    if(tf) *tf = (uint8_t)((c2 & CTRL2_TF)?1:0);
    c2 &= (uint8_t)~(CTRL2_AF|CTRL2_TF);
    _i2c_write(REG_CTRL2, &c2, 1);
}

void pcf8563_clkout(uint8_t enable, uint8_t sel)
{
    uint8_t v = (uint8_t)((enable?CLK_FE:0) | (sel & 0x03u));
    _i2c_write(REG_CLKOUT, &v, 1);
}

void pcf8563_timer_stop(void)
{
    uint8_t tc = 0x03u; /* TE=0, TD=11 per datasheet rec when unused */
    _i2c_write(REG_TMR_CTRL, &tc, 1);
}

void pcf8563_timer_start(uint8_t src, uint8_t value)
{
    if(value==0){
        pcf8563_timer_stop();
        return;
    }
    uint8_t tc = (uint8_t)(0x80u | (src & 0x03u)); /* TE=1, TD */
    _i2c_write(REG_TMR, &value, 1);
    _i2c_write(REG_TMR_CTRL, &tc, 1);
}

void pcf8563_read(uint8_t reg, uint8_t *buf, uint16_t n){ _i2c_read(reg, buf, n); }
void pcf8563_write(uint8_t reg, const uint8_t *buf, uint16_t n){ _i2c_write(reg, buf, n); }

/* ───── Stub if user forgot delay_us --------------------------------------- */
__attribute__((weak)) void rtc_delay_us(uint32_t us){ for(volatile uint32_t i=0;i<us*8;i++); }