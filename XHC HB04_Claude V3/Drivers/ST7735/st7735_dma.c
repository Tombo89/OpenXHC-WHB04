/* vim: set ai et ts=4 sw=4: */
#include "st7735_dma.h"

#include "stm32f1xx_hal.h"
#include <string.h>
#include <stdlib.h>
#include "main.h"


/* -------------------- Hardware Macros: nur ST77_* aus st7735.h -------------------- */
#ifndef ST77_CS_GPIO
# error "Define ST77_CS_GPIO / ST77_CS_PIN in st7735.h"
#endif
#ifndef ST77_DC_GPIO
# error "Define ST77_DC_GPIO / ST77_DC_PIN in st7735.h"
#endif
#ifndef ST77_RST_GPIO
# error "Define ST77_RST_GPIO / ST77_RST_PIN in st7735.h"
#endif
#ifndef ST7735_SPI_PORT
# error "Define ST7735_SPI_PORT (e.g. hspi1) in st7735.h"
#endif

/* Pin helpers */
static inline void CS_LOW(void)   { HAL_GPIO_WritePin(ST77_CS_GPIO,  ST77_CS_PIN,  GPIO_PIN_RESET); }
static inline void CS_HIGH(void)  { HAL_GPIO_WritePin(ST77_CS_GPIO,  ST77_CS_PIN,  GPIO_PIN_SET);   }
static inline void DC_CMD(void)   { HAL_GPIO_WritePin(ST77_DC_GPIO,  ST77_DC_PIN,  GPIO_PIN_RESET); }
static inline void DC_DATA(void)  { HAL_GPIO_WritePin(ST77_DC_GPIO,  ST77_DC_PIN,  GPIO_PIN_SET);   }
static inline void RST_LOW(void)  { HAL_GPIO_WritePin(ST77_RST_GPIO, ST77_RST_PIN, GPIO_PIN_RESET); }
static inline void RST_HIGH(void) { HAL_GPIO_WritePin(ST77_RST_GPIO, ST77_RST_PIN, GPIO_PIN_SET);   }

#define DELAY 0x80

#define ST_COLOR_BURST_PIXELS 128
static uint8_t st_color_burst_buf[ST_COLOR_BURST_PIXELS * 2];

/* DMA busy + Zeilenpuffer */
static volatile uint8_t st_dma_busy = 0;
/* 1 Zeile RGB565 -> 2*ST7735_WIDTH Bytes */
static uint8_t linebuf[ST7735_WIDTH * 2];

/* Wird aus IRQ vom HAL gerufen, wenn SPI-DMA fertig ist */
void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi)
{
    if (hspi == &ST7735_SPI_PORT) {
        st_dma_busy = 0;
    }
}

static inline void ST_WaitDMA(void)     { while (st_dma_busy) { __NOP(); } }
static inline void ST_StartDMA(uint8_t *buf, uint16_t len)
{
    st_dma_busy = 1;
    HAL_SPI_Transmit_DMA(&ST7735_SPI_PORT, buf, len);
}

/* ------------------------------ Low-Level I/O ------------------------------ */

static void ST_WriteCommand(uint8_t cmd)
{
    CS_LOW();   DC_CMD();
    HAL_SPI_Transmit(&ST7735_SPI_PORT, &cmd, 1, HAL_MAX_DELAY);
    CS_HIGH();
}

static void ST_WriteData8(uint8_t d)
{
    CS_LOW();   DC_DATA();
    HAL_SPI_Transmit(&ST7735_SPI_PORT, &d, 1, HAL_MAX_DELAY);
    CS_HIGH();
}

static void ST_WriteData(const uint8_t *buf, uint16_t len)
{
    if (!len) return;
    CS_LOW();   DC_DATA();
    HAL_SPI_Transmit(&ST7735_SPI_PORT, (uint8_t*)buf, len, HAL_MAX_DELAY);
    CS_HIGH();
}

/* Für große Blöcke:
   - Address Window vorher setzen
   - Dann: CS_LOW(); DC_DATA();  ...mehrfach ST_StartDMA() / ST_WaitDMA()... ; CS_HIGH();
*/
static inline void ST_BeginData(void) { CS_LOW();  DC_DATA(); }
static inline void ST_EndData(void)   { CS_HIGH(); }

/* --------------------------- Init Command Tables --------------------------- */

static const uint8_t init_cmds1[] = {
    15,
    ST7735_SWRESET, DELAY, 150,
    ST7735_SLPOUT , DELAY, 255,
    ST7735_FRMCTR1, 3,   0x01, 0x2C, 0x2D,
    ST7735_FRMCTR2, 3,   0x01, 0x2C, 0x2D,
    ST7735_FRMCTR3, 6,   0x01, 0x2C, 0x2D, 0x01, 0x2C, 0x2D,
    ST7735_INVCTR , 1,   0x07,
    ST7735_PWCTR1 , 3,   0xA2, 0x02, 0x84,
    ST7735_PWCTR2 , 1,   0xC5,
    ST7735_PWCTR3 , 2,   0x0A, 0x00,
    ST7735_PWCTR4 , 2,   0x8A, 0x2A,
    ST7735_PWCTR5 , 2,   0x8A, 0xEE,
    ST7735_VMCTR1 , 1,   0x0E,
    ST7735_INVOFF , 0,
    ST7735_MADCTL , 1,   ST7735_ROTATION,
    ST7735_COLMOD , 1,   0x05  /* 16-bit */
};

#if (defined(ST7735_IS_128X128) || defined(ST7735_IS_160X128))
static const uint8_t init_cmds2[] = {
    2,
    ST7735_CASET  , 4,  0x00, 0x00, 0x00, 0x7F,   /* X 0..127 */
    ST7735_RASET  , 4,  0x00, 0x00, 0x00, 0x7F    /* Y 0..127 (bei 160x128 später via MADCTL/Offsets) */
};
#endif

#ifdef ST7735_IS_160X80
static const uint8_t init_cmds2[] = {
    3,
    ST7735_CASET  , 4,  0x00, 0x00, 0x00, 0x4F,   /* X 0..79  */
    ST7735_RASET  , 4,  0x00, 0x00, 0x00, 0x9F,   /* Y 0..159 */
    ST7735_INVON  , 0
};
#endif

static const uint8_t init_cmds3[] = {
    4,
    ST7735_GMCTRP1, 16, 0x02, 0x1c, 0x07, 0x12, 0x37, 0x32, 0x29, 0x2d,
                     0x29, 0x25, 0x2B, 0x39, 0x00, 0x01, 0x03, 0x10,
    ST7735_GMCTRN1, 16, 0x03, 0x1d, 0x07, 0x06, 0x2E, 0x2C, 0x29, 0x2D,
                     0x2E, 0x2E, 0x37, 0x3F, 0x00, 0x00, 0x02, 0x10,
    ST7735_NORON  , DELAY, 10,
    ST7735_DISPON , DELAY, 100
};

static void ST_ExecCmdList(const uint8_t *p)
{
    uint8_t n = *p++;
    while (n--) {
        uint8_t cmd = *p++;
        ST_WriteCommand(cmd);

        uint8_t numArgs = *p++;
        uint16_t ms = numArgs & DELAY;
        numArgs &= ~DELAY;

        if (numArgs) {
            ST_WriteData(p, numArgs);
            p += numArgs;
        }
        if (ms) {
            ms = *p++;
            if (ms == 255) ms = 500;
            HAL_Delay(ms);
        }
    }
}

/* ------------------------------ Address Window ---------------------------- */

void ST7735_SetAddressWindow(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1)
{
    ST_WriteCommand(ST7735_CASET);
    uint8_t caset[4] = {
        (uint8_t)((x0 + ST7735_XSTART) >> 8), (uint8_t)((x0 + ST7735_XSTART) & 0xFF),
        (uint8_t)((x1 + ST7735_XSTART) >> 8), (uint8_t)((x1 + ST7735_XSTART) & 0xFF)
    };
    ST_WriteData(caset, 4);

    ST_WriteCommand(ST7735_RASET);
    uint8_t raset[4] = {
        (uint8_t)((y0 + ST7735_YSTART) >> 8), (uint8_t)((y0 + ST7735_YSTART) & 0xFF),
        (uint8_t)((y1 + ST7735_YSTART) >> 8), (uint8_t)((y1 + ST7735_YSTART) & 0xFF)
    };
    ST_WriteData(raset, 4);

    ST_WriteCommand(ST7735_RAMWR);
}

/* --------------------------------- Public --------------------------------- */
void ST7735_Select(void) {
    HAL_GPIO_WritePin(ST7735_CS_GPIO_Port, ST7735_CS_Pin, GPIO_PIN_RESET);
}


void ST7735_Unselect(void) { CS_HIGH(); }

static void ST_Reset(void)
{
    RST_LOW();  HAL_Delay(5);
    RST_HIGH(); HAL_Delay(120);
}

void ST7735_Init(void)
{
    CS_HIGH(); RST_HIGH(); HAL_Delay(5);

    ST_Reset();
    ST_ExecCmdList(init_cmds1);
    ST_ExecCmdList(init_cmds2);
    ST_ExecCmdList(init_cmds3);

    /* optional clear */
    uint16_t bg = ST7735_BLACK;
    ST7735_FillRectangleFast(0, 0, ST7735_WIDTH, ST7735_HEIGHT, bg);
}

void ST7735_DrawPixel(uint16_t x, uint16_t y, uint16_t color)
{
    if (x >= ST7735_WIDTH || y >= ST7735_HEIGHT) return;
    ST7735_SetAddressWindow(x, y, x, y);
    uint8_t d[2] = { (uint8_t)(color >> 8), (uint8_t)(color & 0xFF) };
    ST_WriteData(d, 2);
}

static void ST_WriteChar(uint16_t x, uint16_t y, char ch,
                         FontDef font, uint16_t color, uint16_t bgcolor)
{
    uint32_t i, b, j;

    ST7735_SetAddressWindow(x, y, x + font.width - 1, y + font.height - 1);

    ST_BeginData();
    for (i = 0; i < font.height; i++) {
        b = font.data[(ch - 32) * font.height + i];
        for (j = 0; j < font.width; j++) {
            uint16_t c = ((b << j) & 0x8000) ? color : bgcolor;
            uint8_t d[2] = { (uint8_t)(c >> 8), (uint8_t)(c & 0xFF) };
            HAL_SPI_Transmit(&ST7735_SPI_PORT, d, 2, HAL_MAX_DELAY);
        }
    }
    ST_EndData();
}

void ST7735_WriteString(uint16_t x, uint16_t y, const char* s,
                        FontDef font, uint16_t color, uint16_t bgcolor)
{
    while (*s) {
        if (x + font.width >= ST7735_WIDTH) {
            x = 0;
            y += font.height;
            if (y + font.height >= ST7735_HEIGHT) break;
            //if (*s == ' ') { s++; continue; }
        }
        ST_WriteChar(x, y, *s, font, color, bgcolor);
        x += font.width;
        s++;
    }
}



/* Blocking Fill (klein & simpel) – bleibt als Referenz */
void ST7735_FillRectangle(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color)
{
    if (x >= ST7735_WIDTH || y >= ST7735_HEIGHT) return;
    if ((x + w) > ST7735_WIDTH)  w = ST7735_WIDTH - x;
    if ((y + h) > ST7735_HEIGHT) h = ST7735_HEIGHT - y;

    ST7735_SetAddressWindow(x, y, x + w - 1, y + h - 1);

    uint8_t hi = (uint8_t)(color >> 8), lo = (uint8_t)(color & 0xFF);
    for (uint16_t row = 0; row < h; row++) {
        for (uint16_t i = 0; i < w; ++i) {
            linebuf[2*i] = hi; linebuf[2*i+1] = lo;
        }
        ST_BeginData();
        HAL_SPI_Transmit(&ST7735_SPI_PORT, linebuf, (uint16_t)(w*2), HAL_MAX_DELAY);
        ST_EndData();
    }
}

/* DMA-Fast: Address Window EINMAL setzen, dann streamen (CS dauerhaft LOW) */
void ST7735_FillRectangleFast(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color)
{
    if (x >= ST7735_WIDTH || y >= ST7735_HEIGHT) return;
    if ((x + w) > ST7735_WIDTH)  w = ST7735_WIDTH - x;
    if ((y + h) > ST7735_HEIGHT) h = ST7735_HEIGHT - y;

    ST7735_SetAddressWindow(x, y, x + w - 1, y + h - 1);

    /* Zeilenpuffer vorbereiten */
    uint8_t hi = (uint8_t)(color >> 8), lo = (uint8_t)(color & 0xFF);
    for (uint16_t i = 0; i < w; ++i) {
        linebuf[2*i] = hi; linebuf[2*i+1] = lo;
    }

    ST_BeginData(); /* CS LOW, DC DATA */
    for (uint16_t row = 0; row < h; ++row) {
        ST_StartDMA(linebuf, (uint16_t)(w * 2));
        ST_WaitDMA();
    }
    ST_EndData();   /* CS HIGH */
}

// interne Helfer: ohne Select/Unselect (für gebündelte Transfers)
static void ST7735_FillRectDMA_NoSelect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color)
{
    if((x >= ST7735_WIDTH) || (y >= ST7735_HEIGHT)) return;
    if((x + w - 1) >= ST7735_WIDTH)  w = ST7735_WIDTH  - x;
    if((y + h - 1) >= ST7735_HEIGHT) h = ST7735_HEIGHT - y;

    ST7735_SetAddressWindow(x, y, x + w - 1, y + h - 1);

    static uint16_t line_buf[ST7735_WIDTH];              // 1 Zeile
    for (uint16_t i = 0; i < w; ++i) line_buf[i] = __REV16(color);

    HAL_GPIO_WritePin(ST77_DC_GPIO, ST77_DC_PIN, GPIO_PIN_SET);

    for (uint16_t row = 0; row < h; ++row) {
        HAL_SPI_Transmit_DMA(&ST7735_SPI_PORT, (uint8_t*)line_buf, w * 2);
        HAL_DMA_PollForTransfer(ST7735_SPI_PORT.hdmatx, HAL_DMA_FULL_TRANSFER, HAL_MAX_DELAY);
    }
}

// öffentlich: löscht alte Pos & zeichnet neue Pos in EINEM Frame
void ST7735_MoveRectFrame(uint16_t prev_x, uint16_t new_x,
                          uint16_t y, uint16_t w, uint16_t h,
                          uint16_t bg, uint16_t fg)
{
    ST7735_Select();

    // 1) alte Position löschen (Hintergrund)
    ST7735_FillRectDMA_NoSelect(prev_x, y, w, h, bg);

    // 2) neue Position zeichnen (Vordergrund)
    ST7735_FillRectDMA_NoSelect(new_x,  y, w, h, fg);

    ST7735_Unselect();
}

void ST7735_FillScreen(uint16_t color)
{
    ST7735_FillRectangleFast(0, 0, ST7735_WIDTH, ST7735_HEIGHT, color);
}

/* Image: RGB565 (little endian) -> byteswappen in linebuf, mit DMA streamen */
void ST7735_DrawImage(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint16_t* data)
{
    if (x >= ST7735_WIDTH || y >= ST7735_HEIGHT) return;
    if ((x + w) > ST7735_WIDTH)  w = ST7735_WIDTH - x;
    if ((y + h) > ST7735_HEIGHT) h = ST7735_HEIGHT - y;

    ST7735_SetAddressWindow(x, y, x + w - 1, y + h - 1);

    ST_BeginData();
    for (uint16_t row = 0; row < h; ++row) {
        const uint16_t *src = data + (uint32_t)row * w;
        for (uint16_t i = 0; i < w; ++i) {
            uint16_t c = src[i];
            linebuf[2*i] = (uint8_t)(c >> 8);
            linebuf[2*i+1] = (uint8_t)(c & 0xFF);
        }
        ST_StartDMA(linebuf, (uint16_t)(w * 2));
        ST_WaitDMA();
    }
    ST_EndData();
}

void ST7735_InvertColors(bool invert)
{
    ST_WriteCommand(invert ? ST7735_INVON : ST7735_INVOFF);
}

void ST7735_SetGamma(GammaDef gamma)
{
    ST_WriteCommand(ST7735_GAMSET);
    ST_WriteData8((uint8_t)gamma);
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

    HAL_GPIO_WritePin(ST7735_DC_GPIO_Port, ST77_DC_PIN, GPIO_PIN_SET); // Data mode

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
    st_clip_rect(&X, &Y, &W, &H, ST7735_WIDTH, ST7735_HEIGHT);
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

void ST7735_DrawRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h,
                     uint16_t color, uint16_t thickness)
{
    if (w == 0 || h == 0 || thickness == 0) return;

    // oben + unten
    ST7735_FillRectangle(x, y, w, thickness, color);
    ST7735_FillRectangle(x, y + h - thickness, w, thickness, color);

    // links + rechts
    ST7735_FillRectangle(x, y, thickness, h, color);
    ST7735_FillRectangle(x + w - thickness, y, thickness, h, color);
}
