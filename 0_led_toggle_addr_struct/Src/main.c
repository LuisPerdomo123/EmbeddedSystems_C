//Where is the led connected?
//Port: A
//Pin: 5
#include <stdint.h>

#define PERIPH_BASE          (0x40000000UL)

#define AHB2PERIPH_OFFSET    (0x08000000UL)
#define AHB2PERIPH_BASE      (PERIPH_BASE + AHB2PERIPH_OFFSET)

#define GPIOA_OFFSET         (0x00000000UL)
#define GPIOA_BASE           (AHB2PERIPH_BASE + GPIOA_OFFSET)

#define RCC_OFFSET           (0x00021000UL)
#define RCC_BASE             (PERIPH_BASE + RCC_OFFSET)

#define GPIOAEN              (1U << 0)

#define PIN5                 (1U << 5)
#define LED_PIN              PIN5


typedef struct
{
    volatile uint32_t UNUSED[19];
    volatile uint32_t AHB2ENR;

} RCC_TypeDef;


typedef struct
{
    volatile uint32_t MODER;       /* 0x00 */
    volatile uint32_t UNUSED[4];   /* 0x04 - 0x10 */
    volatile uint32_t ODR;         /* 0x14 */

} GPIO_TypeDef;


#define RCC     ((RCC_TypeDef *)RCC_BASE)
#define GPIOA   ((GPIO_TypeDef *)GPIOA_BASE)


int main(void)
{
    /* 1. Enable clock access to GPIOA */
    RCC->AHB2ENR |= GPIOAEN;

    /* 2. PA5 = general-purpose output: MODER5[1:0] = 01 */
    GPIOA->MODER &= ~(1U << 11);
    GPIOA->MODER |=  (1U << 10);

    while (1)
    {
        /* 3. Toggle PA5 */
        GPIOA->ODR ^= LED_PIN;

        /* Crude software delay - not accurate */
        for (volatile uint32_t i = 0; i < 100000; i++)
        {
        }
    }
}
