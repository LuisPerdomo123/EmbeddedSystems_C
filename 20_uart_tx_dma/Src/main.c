#include <stdint.h>
#include "stm32l4xx.h"
#include "uart.h"

#define GPIOAEN    (1U << 0)
#define LED_PIN    (1U << 5)

/* Buffer persistente en RAM.
 * No modificar mientras la DMA esté transfiriendo.
 */
static char message[] = "Hello from STM32 DMA transfer\n\r";

static void dma_callback(void);

int main(void)
{
    /* Habilitar reloj de GPIOA. */
    RCC->AHB2ENR |= GPIOAEN;
    (void)RCC->AHB2ENR;

    /* PA5: salida digital. */
    GPIOA->MODER &= ~(3U << 10);
    GPIOA->MODER |=  (1U << 10);

    /* PA5: push-pull, sin resistencias internas. */
    GPIOA->OTYPER &= ~LED_PIN;
    GPIOA->PUPDR  &= ~(3U << 10);

    /* LD2 inicialmente apagado. */
    GPIOA->BSRR = (LED_PIN << 16);

    /* Inicializar USART2 para transmisión. */
    uart2_tx_init();

    /* Habilitar las interrupciones globales mediante PRIMASK. */
    __enable_irq();

    /*
     * Transferir desde RAM hacia USART2->TDR.
     * Se excluye el terminador '\0'.
     */
    dma1_CxS2_init(
        (uint32_t)(uintptr_t)message,
        (uint32_t)(uintptr_t)&USART2->TDR,
        (uint32_t)(sizeof(message) - 1U)
    );

    while (1)
    {
        /* La finalización se atiende exclusivamente por interrupción. */
    }
}

static void dma_callback(void)
{
    /* Encender LD2 y mantenerlo encendido. */
    GPIOA->BSRR = LED_PIN;
}

/*
 * Este nombre debe coincidir exactamente con el utilizado
 * en la tabla de vectores del archivo startup.
 *
 * Debe existir una sola definición de este manejador
 * en los archivos C del proyecto.
 */
void DMA1_CH7_IRQHandler(void)
{
    uint32_t status = DMA1->ISR;

    /* Error de transferencia: detener DMA sin indicar éxito. */
    if ((status & DMA_ISR_TEIF7) != 0U)
    {
        USART2->CR3 &= ~USART_CR3_DMAT;
        DMA1_Channel7->CCR &= ~DMA_CCR_EN;

        /* Limpiar todas las banderas del canal 7. */
        DMA1->IFCR = DMA_IFCR_CGIF7;
        return;
    }

    /* Transferencia completa del bloque. */
    if ((status & DMA_ISR_TCIF7) != 0U)
    {
        USART2->CR3 &= ~USART_CR3_DMAT;
        DMA1_Channel7->CCR &= ~DMA_CCR_EN;

        /* Limpiar las banderas antes de salir de la interrupción. */
        DMA1->IFCR = DMA_IFCR_CGIF7;

        dma_callback();
    }
}
