#include "lcd_gpio.h"

#define LCD_CS   (1U << 2)
#define LCD_RS   (1U << 3)
#define LCD_WR   (1U << 4)
#define LCD_RD   (1U << 5)
#define LCD_RST  (1U << 6)
#define LCD_BL   (1U << 7)

void lcd_gpio_init(void)
{
    /* Habilitar reloj de GPIOB y GPIOC */
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOBEN |
                    RCC_AHB2ENR_GPIOCEN;

    (void)RCC->AHB2ENR;

    /*
     * Establecer niveles iniciales antes
     * de habilitar las salidas.
     *
     * CS, WR, RD, RST = 1
     * RS, BL = 0
     */
    GPIOC->BSRR =
        LCD_CS | LCD_WR | LCD_RD | LCD_RST |
        ((LCD_RS | LCD_BL) << 16);

    /* Bus de datos inicialmente en cero */
    GPIOB->BSRR = 0xFFFF0000UL;

    /*
     * PB0-PB15 como salidas:
     * MODER = 01 para cada pin.
     */
    GPIOB->MODER = 0x55555555UL;

    /* Salidas push-pull */
    GPIOB->OTYPER = 0x00000000UL;

    /* Velocidad media inicial */
    GPIOB->OSPEEDR = 0x55555555UL;

    /* Sin pull-up ni pull-down */
    GPIOB->PUPDR = 0x00000000UL;

    /*
     * Configurar PC2-PC7 como salidas
     * sin modificar los demás pines.
     */
    for (uint32_t pin = 2; pin <= 7; pin++)
    {
        GPIOC->MODER &= ~(3UL << (pin * 2));
        GPIOC->MODER |=  (1UL << (pin * 2));

        GPIOC->OTYPER &= ~(1UL << pin);

        GPIOC->PUPDR &= ~(3UL << (pin * 2));
    }
}
