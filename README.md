# CH32V003 Non-blocking blink

A PlatformIO project that demonstrates a simple "blink" program for the CH32V003 microcontroller, using the [ch32fun](https://github.com/cnlohr/ch32fun/) framework.

Instead of using `Delay_Ms` to set the intervals, it uses the system counter register (`CNT`) of `SysTick` - effectively re-implementing the `millis()` function found in the Arduino framework.