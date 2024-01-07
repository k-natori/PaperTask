# PaperRTM
PaperRTM is a M5Paper sketch to display tasks on your Remember the Milk.

## How to run
1. This sketch requires M5Paper and microSD card (<= 16GB)
2. Put "settings.txt" into microSD. In this settings file you have to specify following
3. Put TTF font file you use into microSD. GenShinGothic-Medium is recommended
4. Put PEM file (root CA for Remember the Milk) into microSD
5. Get API key and shared secret from Remember the Milk
6. Specify your WiFi, font file name, PEM file name, API key and shared secret in "settings.txt"
7. Build and transfer this project as PlatformIO project

## How it works
Once transfered and run, it displays all incomplete tasks in your Remember the Milk tasks. It shutdown power automatically and reboot on next O'clock.

## Dependencies
This PlatformIO project depends on following libraries:
- M5EPD https://github.com/m5stack/M5EPD
- ArduinoMD5 https://github.com/tzikis/ArduinoMD5
- urlencode https://github.com/plageoj/urlencode

## Licenses Notation
### "default_16MB.csv" in this project is used under following license:
MIT License

Copyright (c) 2020 m5stack

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
