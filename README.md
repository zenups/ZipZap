# Spot welder control with rotary encoder

This is a simple control program for a spot welder made from an old microwave transformer.
It uses a ssd1306 128x64 pixel oled display to show the zapping time in milliseconds.
Time is selected by a KY-040 rotary encoder.
~~The interpretation/debouncing of the KY-040 signals could use some work but it gets the job done.~~
PIO interpretation/debouncing of the KY-040 is pretty good.
A simple momentary switch is used to trigger the zapping.
A ZGT-40 DA solid state relay is used to control the zapping.

See [this instructable](https://www.youtube.com/redirect?event=video_description&redir_token=QUFFLUhqbk9nN1VsX1h3b2ljWXBFbXAtdXVhZUhXaERSd3xBQ3Jtc0trV3FueXdhbEdBbUZ3ZC15OHhLWGVTRVl4bVhTeVdkd0FLVzNLei1ab0Myc2lLdDNpcGwzXzhSZmtQR182b0pkSklSamswaFFWaktwMFZ6MExkUHJqMl9wVDR6Vy1IYjc0R1hyanQ2bDZuX3dEQVEzNA&q=https%3A%2F%2Fwww.instructables.com%2FDIY-Spot-Welder-From-Microwave%2F&v=6w9dFNRtqlg)
or [this video](https://www.youtube.com/watch?v=6w9dFNRtqlg)
for more information. *BEWARE:* Opening a microwave is dangerous.
There are high voltage capacitors that can hold a change for a long time after the power is disconnected.
Salvage electronics responsibly at your own risk.

## Some bits of interest as examples even if you aren't building a spot welder:
- PIO interrupts to the cpu from two different state machines using 3 different interrupts.
- A rotary encoder implementation using PIO
- It's not in micropython (I've found c/c++ pico examples to be hard to find)

## Dependencies
You'll need a symlink to [pico_ssd1306](https://github.com/Harbys/pico-ssd1306) in the base directory.

Also... the whole pico sdk. Make sure you copy `pico_sdk_import.cmake` from `<your pico sdk location>/external/` to the
base directory of this project and have `PICO_SDK_PATH` set in your environment.

## Building
Once the dependencies are set:

`$ mkdir build`

`$ cd build`

`$ cmake ../`

`$ make`

Copy ZipZap.[elf|uf2] to the pico of your choice.
