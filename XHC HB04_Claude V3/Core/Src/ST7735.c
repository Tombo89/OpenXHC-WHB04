#include <ST7735.h>
#include <string.h>



#define ST_COLOR_BURST_PIXELS 128
static uint8_t st_color_burst_buf[ST_COLOR_BURST_PIXELS * 2];


int16_t _width;       ///< Display width as modified by current rotation
int16_t _height;      ///< Display height as modified by current rotation
int16_t cursor_x;     ///< x location to start print()ing text
int16_t cursor_y;     ///< y location to start print()ing text
uint8_t rotation;     ///< Display rotation (0 thru 3)
uint8_t _colstart;   ///< Some displays need this changed to offset
uint8_t _rowstart;       ///< Some displays need this changed to offset
uint8_t _xstart;
uint8_t _ystart;

  const uint8_t
  init_cmds1[] = {            // Init for 7735R, part 1 (red or green tab)
    15,                       // 15 commands in list:
    ST7735_SWRESET,   DELAY,  //  1: Software reset, 0 args, w/delay
      150,                    //     150 ms delay
    ST7735_SLPOUT ,   DELAY,  //  2: Out of sleep mode, 0 args, w/delay
      255,                    //     500 ms delay
    ST7735_FRMCTR1, 3      ,  //  3: Frame rate ctrl - normal mode, 3 args:
      0x01, 0x2C, 0x2D,       //     Rate = fosc/(1x2+40) * (LINE+2C+2D)
    ST7735_FRMCTR2, 3      ,  //  4: Frame rate control - idle mode, 3 args:
      0x01, 0x2C, 0x2D,       //     Rate = fosc/(1x2+40) * (LINE+2C+2D)
    ST7735_FRMCTR3, 6      ,  //  5: Frame rate ctrl - partial mode, 6 args:
      0x01, 0x2C, 0x2D,       //     Dot inversion mode
      0x01, 0x2C, 0x2D,       //     Line inversion mode
    ST7735_INVCTR , 1      ,  //  6: Display inversion ctrl, 1 arg, no delay:
      0x07,                   //     No inversion
    ST7735_PWCTR1 , 3      ,  //  7: Power control, 3 args, no delay:
      0xA2,
      0x02,                   //     -4.6V
      0x84,                   //     AUTO mode
    ST7735_PWCTR2 , 1      ,  //  8: Power control, 1 arg, no delay:
      0xC5,                   //     VGH25 = 2.4C VGSEL = -10 VGH = 3 * AVDD
    ST7735_PWCTR3 , 2      ,  //  9: Power control, 2 args, no delay:
      0x0A,                   //     Opamp current small
      0x00,                   //     Boost frequency
    ST7735_PWCTR4 , 2      ,  // 10: Power control, 2 args, no delay:
      0x8A,                   //     BCLK/2, Opamp current small & Medium low
      0x2A,  
    ST7735_PWCTR5 , 2      ,  // 11: Power control, 2 args, no delay:
      0x8A, 0xEE,
    ST7735_VMCTR1 , 1      ,  // 12: Power control, 1 arg, no delay:
      0x0E,
    ST7735_INVOFF , 0      ,  // 13: Don't invert display, no args, no delay
    ST7735_COLMOD , 1      ,  // 15: set color mode, 1 arg, no delay:
      0x05 },                 //     16-bit color

#if (defined(ST7735_IS_128X128) || defined(ST7735_IS_160X128))
  init_cmds2[] = {            // Init for 7735R, part 2 (1.44" display)
    2,                        //  2 commands in list:
    ST7735_CASET  , 4      ,  //  1: Column addr set, 4 args, no delay:
      0x00, 0x00,             //     XSTART = 0
      0x00, 0x7F,             //     XEND = 127
    ST7735_RASET  , 4      ,  //  2: Row addr set, 4 args, no delay:
      0x00, 0x00,             //     XSTART = 0
      0x00, 0x7F },           //     XEND = 127
#endif // ST7735_IS_128X128

#ifdef ST7735_IS_160X80
  init_cmds2[] = {            // Init for 7735S, part 2 (160x80 display)
    3,                        //  3 commands in list:
    ST7735_CASET  , 4      ,  //  1: Column addr set, 4 args, no delay:
      0x00, 0x00,             //     XSTART = 0
      0x00, 0x4F,             //     XEND = 79
    ST7735_RASET  , 4      ,  //  2: Row addr set, 4 args, no delay:
      0x00, 0x00,             //     XSTART = 0
      0x00, 0x9F ,            //     XEND = 159
    ST7735_INVON, 0 },        //  3: Invert colors
#endif

  init_cmds3[] = {            // Init for 7735R, part 3 (red or green tab)
    4,                        //  4 commands in list:
    ST7735_GMCTRP1, 16      , //  1: Magical unicorn dust, 16 args, no delay:
      0x02, 0x1c, 0x07, 0x12,
      0x37, 0x32, 0x29, 0x2d,
      0x29, 0x25, 0x2B, 0x39,
      0x00, 0x01, 0x03, 0x10,
    ST7735_GMCTRN1, 16      , //  2: Sparkles and rainbows, 16 args, no delay:
      0x03, 0x1d, 0x07, 0x06,
      0x2E, 0x2C, 0x29, 0x2D,
      0x2E, 0x2E, 0x37, 0x3F,
      0x00, 0x00, 0x02, 0x10,
    ST7735_NORON  ,    DELAY, //  3: Normal display on, no args, w/delay
      10,                     //     10 ms delay
    ST7735_DISPON ,    DELAY, //  4: Main screen turn on, no args w/delay
      100 };                  //     100 ms delay

void ST7735_Select()
{
    HAL_GPIO_WritePin(CS_PORT, CS_PIN, GPIO_PIN_RESET);
}

void ST7735_Unselect()
{
    HAL_GPIO_WritePin(CS_PORT, CS_PIN, GPIO_PIN_SET);
}

void ST7735_Reset()
{
    HAL_GPIO_WritePin(RST_PORT, RST_PIN, GPIO_PIN_RESET);
    HAL_Delay(5);
    HAL_GPIO_WritePin(RST_PORT, RST_PIN, GPIO_PIN_SET);
}

  void ST7735_WriteCommand(uint8_t cmd)
  {
    HAL_GPIO_WritePin(DC_PORT, DC_PIN, GPIO_PIN_RESET);
    HAL_SPI_Transmit(&ST7735_SPI_PORT, &cmd, sizeof(cmd), HAL_MAX_DELAY);
}

void ST7735_WriteData(uint8_t* buff, size_t buff_size)
{
    HAL_GPIO_WritePin(DC_PORT, DC_PIN, GPIO_PIN_SET);
    HAL_SPI_Transmit(&ST7735_SPI_PORT, buff, buff_size, HAL_MAX_DELAY);
}

void DisplayInit(const uint8_t *addr)
{
    uint8_t numCommands, numArgs;
    uint16_t ms;

    numCommands = *addr++;
    while(numCommands--) {
        uint8_t cmd = *addr++;
        ST7735_WriteCommand(cmd);

        numArgs = *addr++;
        // If high bit set, delay follows args
        ms = numArgs & DELAY;
        numArgs &= ~DELAY;
        if(numArgs) {
            ST7735_WriteData((uint8_t*)addr, numArgs);
            addr += numArgs;
        }

        if(ms) {
            ms = *addr++;
            if(ms == 255) ms = 500;
            HAL_Delay(ms);
        }
    }
}

void ST7735_SetAddressWindow(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1)
{
    // column address set
    ST7735_WriteCommand(ST7735_CASET);
    uint8_t data[] = { 0x00, x0 + _xstart, 0x00, x1 + _xstart };
    ST7735_WriteData(data, sizeof(data));

    // row address set
    ST7735_WriteCommand(ST7735_RASET);
    data[1] = y0 + _ystart;
    data[3] = y1 + _ystart;
    ST7735_WriteData(data, sizeof(data));

    // write to RAM
    ST7735_WriteCommand(ST7735_RAMWR);
}

void ST7735_Init(uint8_t rotation)
{
    ST7735_Select();
    ST7735_Reset();
    DisplayInit(init_cmds1);
    DisplayInit(init_cmds2);
    DisplayInit(init_cmds3);
#if ST7735_IS_160X80
    _colstart = 24;
    _rowstart = 0;
 /*****  IF Doesn't work, remove the code below (before #elif) *****/
    uint8_t data = 0xC0;
    ST7735_Select();
    ST7735_WriteCommand(ST7735_MADCTL);
    ST7735_WriteData(&data,1);
    ST7735_Unselect();

#elif ST7735_IS_128X128
    _colstart = 2;
    _rowstart = 3;
#else
    _colstart = 0;
    _rowstart = 0;
#endif
    ST7735_SetRotation (rotation);
    ST7735_Unselect();

}

void ST7735_SetRotation(uint8_t m)
{

  uint8_t madctl = 0;

  rotation = m % 4; // can't be higher than 3

  switch (rotation)
  {
  case 0:
#if ST7735_IS_160X80
	  madctl = ST7735_MADCTL_MX | ST7735_MADCTL_MY | ST7735_MADCTL_BGR;
#else
      madctl = ST7735_MADCTL_MX | ST7735_MADCTL_MY | ST7735_MADCTL_RGB;
      _height = ST7735_HEIGHT;
      _width = ST7735_WIDTH;
      _xstart = _colstart;
      _ystart = _rowstart;
#endif
    break;
  case 1:
#if ST7735_IS_160X80
	  madctl = ST7735_MADCTL_MY | ST7735_MADCTL_MV | ST7735_MADCTL_BGR;
#else
      madctl = ST7735_MADCTL_MY | ST7735_MADCTL_MV | ST7735_MADCTL_RGB;
      _width = ST7735_HEIGHT;
      _height = ST7735_WIDTH;
    _ystart = _colstart;
    _xstart = _rowstart;
#endif
    break;
  case 2:
#if ST7735_IS_160X80
	  madctl = ST7735_MADCTL_BGR;
#else
      madctl = ST7735_MADCTL_RGB;
      _height = ST7735_HEIGHT;
      _width = ST7735_WIDTH;
    _xstart = _colstart;
    _ystart = _rowstart;
#endif
    break;
  case 3:
#if ST7735_IS_160X80
	  madctl = ST7735_MADCTL_MX | ST7735_MADCTL_MV | ST7735_MADCTL_BGR;
#else
      madctl = ST7735_MADCTL_MX | ST7735_MADCTL_MV | ST7735_MADCTL_RGB;
      _width = ST7735_HEIGHT;
      _height = ST7735_WIDTH;
    _ystart = _colstart;
    _xstart = _rowstart;
#endif
    break;
  }
  ST7735_Select();
  ST7735_WriteCommand(ST7735_MADCTL);
  ST7735_WriteData(&madctl,1);
  ST7735_Unselect();
}

void ST7735_DrawPixel(uint16_t x, uint16_t y, uint16_t color) {
    if((x >= _width) || (y >= _height))
        return;

    ST7735_Select();

    ST7735_SetAddressWindow(x, y, x+1, y+1);
    uint8_t data[] = { color >> 8, color & 0xFF };
    ST7735_WriteData(data, sizeof(data));

    ST7735_Unselect();
}

void ST7735_WriteChar(uint16_t x, uint16_t y, char ch, FontDef font, uint16_t color, uint16_t bgcolor) {
    uint32_t i, b, j;

    ST7735_SetAddressWindow(x, y, x+font.width-1, y+font.height-1);

    for(i = 0; i < font.height; i++) {
        b = font.data[(ch - 32) * font.height + i];
        for(j = 0; j < font.width; j++) {
            if((b << j) & 0x8000)  {
                uint8_t data[] = { color >> 8, color & 0xFF };
                ST7735_WriteData(data, sizeof(data));
            } else {
                uint8_t data[] = { bgcolor >> 8, bgcolor & 0xFF };
                ST7735_WriteData(data, sizeof(data));
            }
        }
    }
}

void ST7735_WriteString(uint16_t x, uint16_t y, const char* str, FontDef font, uint16_t color, uint16_t bgcolor) {
    ST7735_Select();

    while(*str) {
        if(x + font.width >= _width) {
            x = 0;
            y += font.height;
            if(y + font.height >= _height) {
                break;
            }

            if(*str == ' ') {
                // skip spaces in the beginning of the new line
                str++;
                continue;
            }
        }

        ST7735_WriteChar(x, y, *str, font, color, bgcolor);
        x += font.width;
        str++;
    }

    ST7735_Unselect();
}

void ST7735_FillRectangle(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color)
{
    if((x >= _width) || (y >= _height)) return;
    if((x + w - 1) >= _width) w = _width - x;
    if((y + h - 1) >= _height) h = _height - y;

    ST7735_Select();
    ST7735_SetAddressWindow(x, y, x+w-1, y+h-1);

    uint8_t data[] = { color >> 8, color & 0xFF };
    HAL_GPIO_WritePin(DC_PORT, DC_PIN, GPIO_PIN_SET);
    for(y = h; y > 0; y--) {
        for(x = w; x > 0; x--) {
            HAL_SPI_Transmit(&ST7735_SPI_PORT, data, sizeof(data), HAL_MAX_DELAY);
        }
    }

    ST7735_Unselect();
}

void ST7735_DrawImage(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint16_t* data) {
    if((x >= _width) || (y >= _height)) return;
    if((x + w - 1) >= _width) return;
    if((y + h - 1) >= _height) return;

    ST7735_Select();
    ST7735_SetAddressWindow(x, y, x+w-1, y+h-1);
    ST7735_WriteData((uint8_t*)data, sizeof(uint16_t)*w*h);
    ST7735_Unselect();
}

void ST7735_InvertColors(bool invert) {
    ST7735_Select();
    ST7735_WriteCommand(invert ? ST7735_INVON : ST7735_INVOFF);
    ST7735_Unselect();
}

/**
 * @brief Schreibt einzelnes Zeichen mit GFX Font
 * @param x X-Position
 * @param y Y-Position
 * @param c Zeichen
 * @param gfxFont GFX Font Struktur
 * @param color Textfarbe
 * @param bgcolor Hintergrundfarbe
 */
void ST7735_WriteChar_GFX(uint16_t x, uint16_t y, char c, const GFXfont *gfxFont,
                          uint16_t color, uint16_t bgcolor)
{
    if (!gfxFont) return;

    // Zeichen-Bereich prüfen
    if ((c < gfxFont->first) || (c > gfxFont->last)) {
        return;
    }

    // Glyph-Array als richtige Struktur casten
    const GFXglyph *glyph_array = (const GFXglyph *)gfxFont->glyph;

    // Entsprechendes Glyph finden
    uint8_t glyph_index = (uint8_t)(c - gfxFont->first);
    const GFXglyph *glyph = &glyph_array[glyph_index];

    // Bitmap-Daten extrahieren
    const uint8_t *bitmap = gfxFont->bitmap + glyph->bitmapOffset;

    // *** AUTOMATISCHE Y-KORREKTUR ***
    // Für die meisten GFX Fonts ist der Y-Offset ca. 55% der yAdvance
    int16_t y_correction = (gfxFont->yAdvance * 55) / 100;

    // Zeichen rendern mit automatischer Y-Korrektur
    uint16_t bo = 0;
    for (uint8_t yy = 0; yy < glyph->height; yy++) {
        for (uint8_t xx = 0; xx < glyph->width; xx++) {

            uint8_t byte_pos = bo / 8;
            uint8_t bit_pos = 7 - (bo % 8);
            uint8_t bit = (bitmap[byte_pos] >> bit_pos) & 1;

            if (bit) {
                ST7735_DrawPixel(x + glyph->xOffset + xx,
                               y + y_correction + glyph->yOffset + yy, color);
            } else if (bgcolor != color) {
                ST7735_DrawPixel(x + glyph->xOffset + xx,
                               y + y_correction + glyph->yOffset + yy, bgcolor);
            }

            bo++;
        }
    }
}

/**
 * @brief Schreibt String mit GFX Font
 * @param x Start X-Position
 * @param y Start Y-Position
 * @param str String
 * @param gfxFont GFX Font Struktur
 * @param color Textfarbe
 * @param bgcolor Hintergrundfarbe
 */
void ST7735_WriteString_GFX(uint16_t x, uint16_t y, const char* str,
                           const GFXfont *gfxFont, uint16_t color, uint16_t bgcolor)
{
    if (!gfxFont || !str) return;

    // *** ERWEITERTE CACHE-LOGIK für 6 Positionen ***
    static char last_strings[6][20] = {"", "", "", "", "", ""};
    static uint16_t last_positions[6][2] = {{0,0}, {0,0}, {0,0}, {0,0}, {0,0}, {0,0}};

    // Präzise Position-Index basierend auf Y-Koordinate
    uint8_t pos_index = 0;
    if (y >= 15 && y < 25) pos_index = 1;       // WC Y (Y=17)
    else if (y >= 30 && y < 40) pos_index = 2;  // WC Z (Y=32)
    else if (y >= 45 && y < 55) pos_index = 3;  // MC X (Y=49)
    else if (y >= 60 && y < 70) pos_index = 4;  // MC Y (Y=64)
    else if (y >= 75 && y < 85) pos_index = 5;  // MC Z (Y=79)
    // pos_index = 0 für alle anderen (WC X bei Y=2)

    // Prüfe ob String sich geändert hat
    if (strcmp(str, last_strings[pos_index]) != 0 ||
        x != last_positions[pos_index][0] || y != last_positions[pos_index][1]) {

        // Präzise löschen
        uint16_t text_width = ST7735_GetStringWidth_GFX(str, gfxFont);
        uint16_t old_width = ST7735_GetStringWidth_GFX(last_strings[pos_index], gfxFont);
        uint16_t max_width = (text_width > old_width) ? text_width : old_width;

        // Kleine Höhe für dichten Text
        ST7735_FillRectangle(x, y, max_width + 2, 12, bgcolor);

        // Cache aktualisieren
        strncpy(last_strings[pos_index], str, 19);
        last_strings[pos_index][19] = '\0';
        last_positions[pos_index][0] = x;
        last_positions[pos_index][1] = y;
    }

    ST7735_Select();

    uint16_t cursor_x = x;
    uint16_t cursor_y = y;

    const GFXglyph *glyph_array = (const GFXglyph *)gfxFont->glyph;

    while (*str) {
        char c = *str++;

        if (c == '\n') {
            cursor_x = x;
            cursor_y += gfxFont->yAdvance;
            continue;
        }

        if ((c < gfxFont->first) || (c > gfxFont->last)) {
            continue;
        }

        if (cursor_x >= _width) {
            cursor_x = x;
            cursor_y += gfxFont->yAdvance;
            if (cursor_y >= _height) {
                break;
            }
        }

        uint8_t glyph_index = c - gfxFont->first;
        const GFXglyph *glyph = &glyph_array[glyph_index];

        ST7735_WriteChar_GFX(cursor_x, cursor_y, c, gfxFont, color, bgcolor);

        cursor_x += glyph->xAdvance;
    }

    ST7735_Unselect();
}

/**
 * @brief Berechnet String-Breite mit GFX Font
 * @param str String
 * @param gfxFont GFX Font
 * @return Breite in Pixeln
 */
uint16_t ST7735_GetStringWidth_GFX(const char* str, const GFXfont *gfxFont)
{
    if (!gfxFont || !str) return 0;

    uint16_t width = 0;
    const GFXglyph *glyph_array = (const GFXglyph *)gfxFont->glyph;

    while (*str) {
        char c = *str++;

        if ((c >= gfxFont->first) && (c <= gfxFont->last)) {
            uint8_t glyph_index = c - gfxFont->first;
            const GFXglyph *glyph = &glyph_array[glyph_index];
            width += glyph->xAdvance;
        }
    }

    return width;
}

/**
 * @brief Test-Funktion für GFX Fonts
 */
void test_gfx_fonts(void)
{
    // Hintergrund löschen
    ST7735_FillScreen(BLACK);

    // FreeSansBold9pt Font testen
    ST7735_WriteString_GFX(10, 30, "XHC-HB04", &FreeSansBold9pt7b, WHITE, BLACK);
    ST7735_WriteString_GFX(10, 55, "GFX Font Test", &FreeSansBold9pt7b, YELLOW, BLACK);

    // Zahlen testen
    ST7735_WriteString_GFX(10, 80, "X: 123.456", &FreeSansBold9pt7b, GREEN, BLACK);
    ST7735_WriteString_GFX(10, 105, "Y: -78.901", &FreeSansBold9pt7b, RED, BLACK);

    // Vergleich mit normaler Font
    ST7735_WriteString(10, 130, "Normal Font", Font_7x10, CYAN, BLACK);
}



// Eigene Funktionen

static void ST7735_WriteColorBurst(uint16_t color, uint32_t pixel_count)
{
    // Puffer einmal mit Farbwert füllen
    uint8_t hi = color >> 8, lo = color & 0xFF;
    for (int i = 0; i < ST_COLOR_BURST_PIXELS; ++i) {
        st_color_burst_buf[2*i]   = hi;
        st_color_burst_buf[2*i+1] = lo;
    }

    HAL_GPIO_WritePin(DC_PORT, DC_PIN, GPIO_PIN_SET); // Data mode

    while (pixel_count) {
        uint32_t chunk = (pixel_count > ST_COLOR_BURST_PIXELS) ? ST_COLOR_BURST_PIXELS : pixel_count;
        HAL_SPI_Transmit(&ST7735_SPI_PORT, st_color_burst_buf, (uint16_t)(chunk * 2), HAL_MAX_DELAY);
        pixel_count -= chunk;
    }
}

// Clipping-Helfer, wie bei dir in FillRectangle
static inline void st_clip_rect(uint16_t *x, uint16_t *y, uint16_t *w, uint16_t *h, uint16_t W, uint16_t H)
{
    if (*x >= W || *y >= H) { *w = *h = 0; return; }
    if (*x + *w > W) *w = W - *x;
    if (*y + *h > H) *h = H - *y;
}

void ST7735_DrawRectFast(uint16_t x, uint16_t y, uint16_t w, uint16_t h,
                         uint16_t color, uint16_t thickness)
{
    if (w == 0 || h == 0 || thickness == 0) return;

    // Clipping vorbereiten (lokale Kopien, um Original nicht zu verändern)
    uint16_t X = x, Y = y, W = w, H = h;
    st_clip_rect(&X, &Y, &W, &H, _width, _height);
    if (W == 0 || H == 0) return;

    // Kanten-Geometrie
    uint16_t top_h    = (thickness < H) ? thickness : H;
    uint16_t bottom_h = top_h;
    uint16_t left_w   = (thickness < W) ? thickness : W;
    uint16_t right_w  = left_w;

    ST7735_Select();

    // Oben (X..X+W-1, Y..Y+top_h-1)
    ST7735_SetAddressWindow(X, Y, X + W - 1, Y + top_h - 1);
    ST7735_WriteColorBurst(color, (uint32_t)W * top_h);

    // Unten (X..X+W-1, Y+H-bottom_h..Y+H-1)
    if (H > top_h) {
        ST7735_SetAddressWindow(X, Y + H - bottom_h, X + W - 1, Y + H - 1);
        ST7735_WriteColorBurst(color, (uint32_t)W * bottom_h);
    }

    // Links (X..X+left_w-1, Y+top_h..Y+H-bottom_h-1)
    if (H > (top_h + bottom_h) && left_w) {
        uint16_t v_h = H - top_h - bottom_h;
        ST7735_SetAddressWindow(X, Y + top_h, X + left_w - 1, Y + top_h + v_h - 1);
        ST7735_WriteColorBurst(color, (uint32_t)left_w * v_h);
    }

    // Rechts (X+W-right_w..X+W-1, Y+top_h..Y+H-bottom_h-1)
    if (H > (top_h + bottom_h) && right_w && W > left_w) {
        uint16_t v_h = H - top_h - bottom_h;
        ST7735_SetAddressWindow(X + W - right_w, Y + top_h, X + W - 1, Y + top_h + v_h - 1);
        ST7735_WriteColorBurst(color, (uint32_t)right_w * v_h);
    }

    ST7735_Unselect();
}

/**
 * @brief Erweiterte UI-Update-Funktion mit GFX Font
 */

