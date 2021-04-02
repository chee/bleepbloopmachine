#pragma once

#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>

#define NEOPIXEL_PIN 8
#define NEOPIXEL_LENGTH 5

#define DISPLAY_CS 44
#define DISPLAY_RST 46
#define DISPLAY_DC 45
#define DISPLAY_BACKLIGHT 47
#define DISPLAY_HEIGHT 128
#define DISPLAY_WIDTH 160

#define BLOCK_SIZE 20
#define GRID_OFFSET_X 36
#define GRID_OFFSET_Y 20
#define GRID_GAP 2
#define STANDARD_BLOCK_COLOR 0xfebb
#define TICK_BLOCK_COLOR 0xfc53

struct Point {
  int x;
  int y;
};

Point getCoord(int idx);
