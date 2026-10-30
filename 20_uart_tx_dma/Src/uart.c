#include "uart.h"

#define GPIOAEN             (1U << 0)
#define UART2EN             (1U << 17)

#define SYS_FREQ            4000000U
#define APB1_CLK            SYS_FREQ
#define UART_BAUDRATE       115200U

/* DMA1_CSELR: selección de solicitud para el canal 7. */
#define DMA_C7S_MASK         (0xFU << 24)
#define DMA_C7S_USART2_TX    (2U << 24)

static void uart_set_baudrate(
    USART_TypeDef *USARTx,
    uint32_t PeriphClk,
    uint32_t BaudRate
);

static uint16_t compute_uart_bd(
    uint32_t PeriphClk,
    uint32_t BaudRate
);

int __io_putchar(int ch)
{
    uart2_write(ch);
    return ch;
}

void dma1_CxS2_init(uint32_t src, uint32_t dst, uint32_t len)
{
    /* El contador DMA tiene 16 bits. */
    if ((len == 0U) || (len > 65535U))
    {
        return;
    }

    /* Habilitar reloj de DMA1 en STM32L476RG. */
    RCC->AHB1ENR |= RCC_AHB1ENR_DMA1EN;
    (void)RCC->AHB1ENR;

    /* Detener solicitudes mientras se configura el canal. */
    USART2->CR3 &= ~USART_CR3_DMAT;

    DMA1_Channel7->CCR &= ~DMA_CCR_EN;

    while ((DMA1_Channel7->CCR & DMA_CCR_EN) != 0U)
    {
    }

    /* Limpiar banderas de una transferencia anterior. */
    DMA1->IFCR = DMA_IFCR_CGIF7;
    NVIC_ClearPendingIRQ(DMA1_Channel7_IRQn);

    /*
     * USART2_TX:
     * DMA1, canal 7, solicitud 2.
     */
    DMA1_CSELR->CSELR =
        (DMA1_CSELR->CSELR & ~DMA_C7S_MASK)
        | DMA_C7S_USART2_TX;

    /* Dirección del periférico: USART2->TDR. */
    DMA1_Channel7->CPAR = dst;

    /* Dirección del buffer en memoria. */
    DMA1_Channel7->CMAR = src;

    /* Cantidad de bytes por transferir. */
    DMA1_Channel7->CNDTR = len;

    /*
     * Configuración:
     * DIR   = 1: memoria hacia periférico.
     * MINC  = 1: incrementar dirección de memoria.
     * PINC  = 0: dirección del periférico fija.
     * MSIZE = 00: memoria de 8 bits.
     * PSIZE = 00: periférico de 8 bits.
     * CIRC  = 0: transferencia única.
     * TCIE  = 1: interrupción por transferencia completa.
     * TEIE  = 1: interrupción por error.
     */
    DMA1_Channel7->CCR =
          DMA_CCR_DIR
        | DMA_CCR_MINC
        | DMA_CCR_TCIE
        | DMA_CCR_TEIE;

    /* Habilitar la interrupción correspondiente en el NVIC. */
    NVIC_EnableIRQ(DMA1_Channel7_IRQn);

    /* Completar las escrituras de memoria antes de arrancar DMA. */
    __DMB();

    /* Primero habilitar el canal; después las solicitudes USART. */
    DMA1_Channel7->CCR |= DMA_CCR_EN;
    USART2->CR3 |= USART_CR3_DMAT;
}

void uart2_tx_init(void)
{
    /* Habilitar reloj GPIOA. */
    RCC->AHB2ENR |= GPIOAEN;
    (void)RCC->AHB2ENR;

    /* PA2: modo función alternativa. */
    GPIOA->MODER &= ~(3U << 4);
    GPIOA->MODER |=  (2U << 4);

    /* PA2: AF7 = USART2_TX. */
    GPIOA->AFR[0] &= ~(0xFU << 8);
    GPIOA->AFR[0] |=  (7U << 8);

    /* Habilitar reloj USART2. */
    RCC->APB1ENR1 |= UART2EN;
    (void)RCC->APB1ENR1;

    /* Deshabilitar USART antes de configurar. */
    USART2->CR1 = 0U;

    uart_set_baudrate(
        USART2,
        APB1_CLK,
        UART_BAUDRATE
    );

    /* Transmisión, 8 bits, sin paridad, oversampling por 16. */
    USART2->CR1 = USART_CR1_TE;
    USART2->CR1 |= USART_CR1_UE;
}

void uart2_rxtx_init(void)
{
    /* Habilitar reloj GPIOA. */
    RCC->AHB2ENR |= GPIOAEN;
    (void)RCC->AHB2ENR;

    /* PA2 y PA3: modo función alternativa. */
    GPIOA->MODER &= ~((3U << 4) | (3U << 6));
    GPIOA->MODER |=  ((2U << 4) | (2U << 6));

    /* PA2 y PA3: AF7 = USART2. */
    GPIOA->AFR[0] &= ~((0xFU << 8) | (0xFU << 12));
    GPIOA->AFR[0] |=  ((7U << 8) | (7U << 12));

    /* Habilitar reloj USART2. */
    RCC->APB1ENR1 |= UART2EN;
    (void)RCC->APB1ENR1;

    USART2->CR1 = 0U;

    uart_set_baudrate(
        USART2,
        APB1_CLK,
        UART_BAUDRATE
    );

    /* Habilitar transmisión y recepción. */
    USART2->CR1 = USART_CR1_TE | USART_CR1_RE;
    USART2->CR1 |= USART_CR1_UE;
}

void uart2_rx_interrupt_init(void)
{
    uart2_rxtx_init();

    /* Esta función requiere USART2_IRQHandler en la aplicación
     * que utilice recepción por interrupciones.
     * El main de este ejemplo no llama esta función.
     */
    USART2->CR1 |= USART_CR1_RXNEIE;
    NVIC_EnableIRQ(USART2_IRQn);
}

char uart2_read(void)
{
    while ((USART2->ISR & USART_ISR_RXNE) == 0U)
    {
    }

    return (char)(USART2->RDR & 0xFFU);
}

void uart2_write(int ch)
{
    while ((USART2->ISR & USART_ISR_TXE) == 0U)
    {
    }

    USART2->TDR = (uint8_t)ch;
}

static void uart_set_baudrate(
    USART_TypeDef *USARTx,
    uint32_t PeriphClk,
    uint32_t BaudRate
)
{
    USARTx->BRR = compute_uart_bd(PeriphClk, BaudRate);
}

static uint16_t compute_uart_bd(
    uint32_t PeriphClk,
    uint32_t BaudRate
)
{
    return (uint16_t)(
        (PeriphClk + (BaudRate / 2U)) / BaudRate
    );
}
