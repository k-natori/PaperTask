# PaperTask
PaperTask is a M5Paper sketch to display tasks on your Remember the Milk.

## How to run
1. This sketch requires M5Paper and microSD card (<= 16GB)
2. Put "settings.txt" into microSD. In this settings file you have to specify following
3. Put TTF font file you use into microSD. GenShinGothic-Medium (http://jikasei.me/font/genshin/) is recommended
4. Put PEM file (root CA for Remember the Milk, should be rememberthemilk-com.pem) into microSD
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
