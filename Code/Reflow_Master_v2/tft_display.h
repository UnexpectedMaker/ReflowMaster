#ifndef TFT_DISPLAY_H
#define TFT_DISPLAY_H

#include <Adafruit_ILI9341.h>  // Add from Library Manager
#include <Adafruit_GFX.h>  // Add from Library Manager

// Just a bunch of pre-defined colours
#define DEFAULT_COLOR 0xABCD0000  // magic value, not a real color
#define BLUE      0x001F
#define TEAL      0x0438
#define GREEN     0x07E0
#define CYAN      0x07FF
#define RED       0xF800
#define MAGENTA   0xF81F
#define YELLOW    0xFFE0
#define ORANGE    0xFC00
#define PINK      0xF81F
#define PURPLE    0x8010
#define GREY      0xC618
#define WHITE     0xFFFF
#define BLACK     0x0000
#define DKBLUE    0x000D
#define DKTEAL    0x020C
#define DKGREEN   0x03E0
#define DKCYAN    0x03EF
#define DKRED     0x6000
#define DKMAGENTA 0x8008
#define DKYELLOW  0x8400
#define DKORANGE  0x8200
#define DKPINK    0x9009
#define DKPURPLE  0x4010
#define DKGREY    0x4A49

extern Adafruit_ILI9341 tft;

#endif
