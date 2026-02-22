# Kids Audio Player - Functional Specification Document (FSD)

## 1. Purpose
A standalone, kid-friendly audio player inspired by Yoto: browses MP3 files from SD, plays audio via I2S, shows track info and artwork on a 1.44" TFT, and uses a rotary encoder with pushbutton for navigation and playback control.

## 2. Scope
This spec covers firmware behavior for Rev00 on ESP32-S3 (Feather-class), SD card storage, TFT UI, rotary input, and MP3 playback. Enclosure/CAD and audio content creation are out of scope.

## 3. System Overview
- MCU: ESP32-S3 (Adafruit Feather class)
- Storage: microSD (SPI)
- UI: 1.44" 128x128 ST7735 TFT + JPEG artwork per track
- Input: I2C QT rotary encoder (seesaw)
- Audio: MP3 playback via I2S
- Power: Battery gauge (MAX17048 supported)

## 4. Hardware Requirements
- ESP32-S3 board with exposed SPI and I2S pins
- ST7735 1.44" SPI TFT (128x128)
- microSD breakout (SPI)
- Adafruit I2C QT rotary encoder (seesaw, addr 0x36)
- Optional: MAX17048 battery gauge

## 5. Functional Requirements

### 5.1 Boot / Init
- Initialize TFT and show startup image `/startup.jpg` if present.
- Initialize SD card over SPI; halt if SD init fails.
- Scan SD root for `.mp3` files (max 50 entries).
- Initialize rotary encoder and set position to 0.
- Initialize I2S and audio player pipeline.

### 5.2 Media Library
- Read `.mp3` files from SD root directory.
- Display a scrolling list of tracks on the TFT.
- Selection is controlled by rotary rotation.

### 5.3 Playback
- Press encoder to start playback of selected track.
- Press encoder again to stop playback.
- While playing, rotary adjusts volume.
- Audio playback runs in a dedicated RTOS task.

### 5.4 Artwork
- When playback starts, attempt to display a `.jpg` with the same basename as the selected `.mp3`.
- JPEG is scaled to fit the 128x128 display.

### 5.5 Volume
- Volume range: 0.0–1.0
- Default volume: 0.3
- Rotary changes volume in 3% increments.

### 5.6 State Management
- Playback states: STOPPED, PLAYING, PAUSED (pause not yet used in UI).
- UI reflects state: list view in STOPPED/PAUSED, artwork in PLAYING.

## 6. UI Requirements
- List view shows all MP3 files with current selection highlighted.
- Playing view shows album art (JPG).
- Volume indicator appears at bottom while changing volume.

## 7. Error Handling
- SD init failure should halt with on-screen message.
- Encoder init failure should halt with on-screen message.
- Missing artwork should log to Serial but not block playback.

## 8. Performance & Limits
- Max MP3 files: 50
- JPEG decoding uses TJpg_Decoder, scaled to fit display.
- Audio runs in RTOS task with non-blocking pipeline.

## 9. Configuration
- Pin definitions are hardcoded in `rev00.ino` for SD, TFT, I2S, and encoder.
- Audio volume default hardcoded to 0.3.

## 10. Future Enhancements
- Pause/resume support with UI feedback.
- Long-press for power-off or sleep mode.
- Battery percentage display via MAX17048.
- Folder browsing instead of root-only.
- UI improvements (icons, progress bar, elapsed time).

