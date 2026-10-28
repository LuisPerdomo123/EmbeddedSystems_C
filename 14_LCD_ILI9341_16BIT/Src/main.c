
#include "stm32l4xx.h"
#include <stdint.h>
#include "lcd_gpio.h"

/* =====================================================
 * PROYECTO: DIAGNOSTICO LCD ILI9341
 * MCU: STM32L476RG
 * LCD: MRB3205, 240 x 320
 * INTERFAZ: 8080 - 16 BITS
 *
 * DB0-DB15 -> PB0-PB15
 * CS       -> PC2
 * RS       -> PC3
 * WR       -> PC4
 * RD       -> PC5
 * RST      -> PC6
 * BL       -> PC7
 * ===================================================== */

#define LCD_CS      (1UL << 2)
#define LCD_RS      (1UL << 3)
#define LCD_WR      (1UL << 4)
#define LCD_RD      (1UL << 5)
#define LCD_RST     (1UL << 6)
#define LCD_BL      (1UL << 7)

#define LCD_WIDTH   240U
#define LCD_HEIGHT  320U

#define RED         0xF800U
#define GREEN       0x07E0U
#define BLUE        0x001FU
#define BLACK       0x0000U
#define WHITE       0xFFFFU

/* Reloj MSI de arranque, sin PLL */
#define CPU_FREQ_HZ 4000000UL

/* =====================================================
 * RETARDO CON SYSTICK
 * Sin dependencia de SystemCoreClock
 * ===================================================== */

static void delay_ms(uint32_t ms)
{
    const uint32_t ticks = CPU_FREQ_HZ / 1000UL;

    SysTick->LOAD = ticks - 1UL;
    SysTick->VAL = 0UL;

    SysTick->CTRL =
        SysTick_CTRL_CLKSOURCE_Msk |
        SysTick_CTRL_ENABLE_Msk;

    while (ms > 0UL)
    {
        if ((SysTick->CTRL &
             SysTick_CTRL_COUNTFLAG_Msk) != 0UL)
        {
            ms--;
        }
    }

    SysTick->CTRL = 0UL;
}

/* =====================================================
 * ESCRITURA PARALELA DE 16 BITS
 * ===================================================== */

static inline void lcd_write16(uint16_t value)
{
    GPIOB->ODR = (uint32_t)value;

    __DSB();

    /* WR = 0 */
    GPIOC->BSRR = (LCD_WR << 16);

    __DSB();

    /* WR = 1 */
    GPIOC->BSRR = LCD_WR;

    __DSB();
}

/* =====================================================
 * COMANDOS LCD
 * ===================================================== */

static void lcd_cmd(uint8_t command)
{
    /* RS = 0 */
    GPIOC->BSRR = (LCD_RS << 16);

    /* CS = 0 */
    GPIOC->BSRR = (LCD_CS << 16);

    lcd_write16((uint16_t)command);

    /* CS = 1 */
    GPIOC->BSRR = LCD_CS;
}

/* =====================================================
 * DATOS LCD DE 8 BITS
 * ===================================================== */

static void lcd_data8(uint8_t data)
{
    /* RS = 1 */
    GPIOC->BSRR = LCD_RS;

    /* CS = 0 */
    GPIOC->BSRR = (LCD_CS << 16);

    lcd_write16((uint16_t)data);

    /* CS = 1 */
    GPIOC->BSRR = LCD_CS;
}

/* =====================================================
 * INICIALIZACION ILI9341
 * ===================================================== */

static void lcd_init(void)
{
    GPIOC->BSRR = LCD_CS | LCD_WR | LCD_RD;

    /* Reset fisico */
    GPIOC->BSRR = (LCD_RST << 16);
    delay_ms(30U);

    GPIOC->BSRR = LCD_RST;
    delay_ms(150U);

    /* Software reset */
    lcd_cmd(0x01);
    delay_ms(150U);

    /* Display OFF */
    lcd_cmd(0x28);

    /* RGB565 */
    lcd_cmd(0x3A);
    lcd_data8(0x55);

    /* Memory Access Control */
    lcd_cmd(0x36);
    lcd_data8(0x48);

    /* Sleep OUT */
    lcd_cmd(0x11);
    delay_ms(150U);

    /* Display ON */
    lcd_cmd(0x29);
    delay_ms(50U);

    /* Backlight */
    GPIOC->BSRR = LCD_BL;
}

/* =====================================================
 * CONFIGURAR VENTANA
 * ===================================================== */

static void lcd_set_window(uint16_t x0,
                           uint16_t y0,
                           uint16_t x1,
                           uint16_t y1)
{
    lcd_cmd(0x2A);

    lcd_data8((uint8_t)(x0 >> 8));
    lcd_data8((uint8_t)x0);
    lcd_data8((uint8_t)(x1 >> 8));
    lcd_data8((uint8_t)x1);

    lcd_cmd(0x2B);

    lcd_data8((uint8_t)(y0 >> 8));
    lcd_data8((uint8_t)y0);
    lcd_data8((uint8_t)(y1 >> 8));
    lcd_data8((uint8_t)y1);

    lcd_cmd(0x2C);
}

/* =====================================================
 * RECTANGULO RELLENO
 * ===================================================== */

static void lcd_fill_rect(uint16_t x,
                          uint16_t y,
                          uint16_t w,
                          uint16_t h,
                          uint16_t color)
{
    if ((w == 0U) || (h == 0U))
        return;

    if ((x >= LCD_WIDTH) || (y >= LCD_HEIGHT))
        return;

    if (w > LCD_WIDTH - x)
        w = LCD_WIDTH - x;

    if (h > LCD_HEIGHT - y)
        h = LCD_HEIGHT - y;

    lcd_set_window(
        x,
        y,
        x + w - 1U,
        y + h - 1U
    );

    GPIOC->BSRR = LCD_RS;
    GPIOC->BSRR = (LCD_CS << 16);

    uint32_t pixels = (uint32_t)w * h;

    while (pixels--)
    {
        lcd_write16(color);
    }

    GPIOC->BSRR = LCD_CS;
}

/* =====================================================
 * FUENTE HEXADECIMAL 5x7
 *
 * Indices:
 * 0-9 -> digitos
 * 10-15 -> A-F
 * ===================================================== */

static const uint8_t font_hex[16][5] =
{
    /* 0 */ {0x3E,0x51,0x49,0x45,0x3E},
    /* 1 */ {0x00,0x42,0x7F,0x40,0x00},
    /* 2 */ {0x42,0x61,0x51,0x49,0x46},
    /* 3 */ {0x21,0x41,0x45,0x4B,0x31},
    /* 4 */ {0x18,0x14,0x12,0x7F,0x10},
    /* 5 */ {0x27,0x45,0x45,0x45,0x39},
    /* 6 */ {0x3C,0x4A,0x49,0x49,0x30},
    /* 7 */ {0x01,0x71,0x09,0x05,0x03},
    /* 8 */ {0x36,0x49,0x49,0x49,0x36},
    /* 9 */ {0x06,0x49,0x49,0x29,0x1E},
    /* A */ {0x7E,0x11,0x11,0x11,0x7E},
    /* B */ {0x7F,0x49,0x49,0x49,0x36},
    /* C */ {0x3E,0x41,0x41,0x41,0x22},
    /* D */ {0x7F,0x41,0x41,0x22,0x1C},
    /* E */ {0x7F,0x49,0x49,0x49,0x41},
    /* F */ {0x7F,0x09,0x09,0x09,0x01}
};

/* =====================================================
 * DIBUJAR CARACTER HEXADECIMAL
 * ===================================================== */

static void lcd_draw_hex_char(uint16_t x,
                              uint16_t y,
                              char ch,
                              uint16_t color,
                              uint8_t scale)
{
    uint8_t index;

    if ((ch >= '0') && (ch <= '9'))
    {
        index = (uint8_t)(ch - '0');
    }
    else if ((ch >= 'A') && (ch <= 'F'))
    {
        index = (uint8_t)(ch - 'A' + 10);
    }
    else
    {
        return;
    }

    const uint8_t *glyph = font_hex[index];

    for (uint8_t col = 0U; col < 5U; col++)
    {
        for (uint8_t row = 0U; row < 7U; row++)
        {
            if (glyph[col] & (1U << row))
            {
                lcd_fill_rect(
                    x + col * scale,
                    y + row * scale,
                    scale,
                    scale,
                    color
                );
            }
        }
    }
}

/* =====================================================
 * DIBUJAR ETIQUETA HEXADECIMAL
 * ===================================================== */

static void lcd_draw_hex_text(uint16_t x,
                              uint16_t y,
                              const char *text,
                              uint16_t color,
                              uint8_t scale)
{
    while (*text != '\0')
    {
        lcd_draw_hex_char(
            x,
            y,
            *text,
            color,
            scale
        );

        x += 6U * scale;
        text++;
    }
}

/* =====================================================
 * CONVERTIR UINT16 A TEXTO HEXADECIMAL
 * ===================================================== */

static void hex16_to_string(uint16_t value,
                            char text[5])
{
    static const char hex[] =
        "0123456789ABCDEF";

    text[0] = hex[(value >> 12) & 0xFU];
    text[1] = hex[(value >> 8)  & 0xFU];
    text[2] = hex[(value >> 4)  & 0xFU];
    text[3] = hex[value & 0xFU];
    text[4] = '\0';
}

/* =====================================================
 * MATRIZ DE DIAGNOSTICO RGB565
 * ===================================================== */

static void lcd_diagnostic_matrix(void)
{
    static const uint16_t patterns[16] =
    {
        0x0000U, 0xFFFFU, 0xF800U, 0x07E0U,
        0x001FU, 0x07FFU, 0xF81FU, 0xFFE0U,
        0x0010U, 0x0400U, 0x8000U, 0x5555U,
        0xAAAAU, 0x0001U, 0x0020U, 0x0800U
    };

    static const uint16_t text_colors[16] =
    {
        WHITE, BLACK, BLACK, BLACK,
        WHITE, BLACK, BLACK, BLACK,
        WHITE, WHITE, WHITE, BLACK,
        WHITE, WHITE, WHITE, WHITE
    };

    for (uint8_t i = 0U; i < 16U; i++)
    {
        uint16_t x = (i % 4U) * 60U;
        uint16_t y = (i / 4U) * 80U;

        uint16_t color = patterns[i];

        /* Fondo de cada celda */
        lcd_fill_rect(
            x,
            y,
            60U,
            80U,
            color
        );

        /* Etiqueta hexadecimal */
        char label[5];

        hex16_to_string(color, label);

        /* Texto centrado */
        lcd_draw_hex_text(
            x + 18U,
            y + 36U,
            label,
            text_colors[i],
            1U
        );
    }
}

/* =====================================================
 * MAIN
 * ===================================================== */

int main(void)
{
    /* Inicializar GPIO */
    lcd_gpio_init();

    /* Inicializar ILI9341 */
    lcd_init();

    /* Dibujar matriz RGB565 */
    lcd_diagnostic_matrix();

    /* Imagen estatica */
    while (1)
    {
        __NOP();
    }
}
