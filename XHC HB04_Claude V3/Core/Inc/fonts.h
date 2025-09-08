/* vim: set ai et ts=4 sw=4: */
#ifndef __FONTS_H__
#define __FONTS_H__

#include <stdint.h>

typedef struct {
    const uint8_t width;
    uint8_t height;
    const uint16_t *data;
} FontDef;


extern FontDef Font_7x10;
extern FontDef Font_11x18;
extern FontDef Font_16x26;

typedef struct {
    const uint8_t *bitmap;
    const void *glyph;
    uint8_t first;
    uint8_t last;
    uint8_t yAdvance;
} GFXfont;

typedef struct {
    uint16_t bitmapOffset;
    uint8_t width;
    uint8_t height;
    uint8_t xAdvance;
    int8_t xOffset;
    int8_t yOffset;
} GFXglyph;

// Font-Deklaration
extern const GFXfont FreeSansBold9pt7b;
extern const GFXfont dosis_bold8pt7b;

#endif // __FONTS_H__
