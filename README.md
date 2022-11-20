# KMITL_Microcontroller_Project
Project for my Microcontroller class.

A STM32F767ZI microcontroller music box project.

## Things That It Does:
- Analog PCM output to standard 3.5mm audio jacks. No negative voltage though, so I can't guarantee that your speakers will survive if you actually plug it in.
  - Also, it's very bad.
- You can play music on it, in all its 1-octave 7-note glory. Uses a custom-made synthesizer library.
- You can also record music. Uses a very shitty audio format.
  - Recorded music is stored on the on-chip FLASH memory because I couldn't figure out how to make the fucking SD card module work.
- LCD display.
- Makes me a very annoyed and tired wolf.

## Modules Used:
- ILI9341 240x320 TFT LCD
- Standard TRRS 3.5mm audio jack breakout

## I Want To Die