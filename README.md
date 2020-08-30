# Arduino LED clock
This is my .ino source code for my first Arduino hobby project: infinite LED clock.
You may see my blog entry about this project here [https://blog.sovoboys.net/?p=1086](https://blog.sovoboys.net/?p=1086).

## Requirement

 - **Arduino** (, of course) - I use (Chinese) Arduino Nano.
 - DC5V **WS2812** Addressable RGB LED Strip - I use 96LEDs/m, but cut it to only 60 LEDs.
 - **DS3231** RTC (Real Time Clock) module.
 - **MAX9812** Microphone module (, for clap sound input).
 - 3 push-on-release-off switches, or you can use only 1 rotary encoder with button.
 - AC220V to **DC5V 2A** adapter to supply the whole system.

There are other hardwares/tools that be need for this project - e.g. capacitors, resisters, electrical soldering tools, mirror, glass, one-way reflective film, photo frame. But for the code scope I won't talk about it.
## Noted

 - According to the DS3231 module references - it always requires `SDA_PIN` to Arduino's `A4` pin, and `SCL_PIN` to Arduino's `A5` pin.
 - This code uses FastLED library, which requires exact amount of LEDs of each stripe. You can see it in the `LEDS_NUM` value
