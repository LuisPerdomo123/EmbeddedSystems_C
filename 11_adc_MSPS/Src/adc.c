
#include <stdint.h>
#include "stm32l4xx.h"
#include "adc.h"

#define GPIOAEN          (1U<<0)
#define ADC1EN           (1U<<13)
#define ADC2EN           (1U<<14)  /* [M1] Clock ADC2 */

#define ADC_CH6          (6U<<6)
#define ADC_SEQ_LEN_1    0x00U
#define CR_ADON          (1U<<0)
#define CR2_ADSTART      (1U<<2)
#define SR_EOC           (1U<<2)
#define CFGR_CONT       (1U<<13)


/* [M2] 8.00 MSPS nominal at 12 bits: two 4 MSPS ADCs interleaved.
 * HSI16 / PLLM(2) * PLLSAI1N(19) / PLLSAI1R(2) = 76 MHz.
 * Each ADC: 6.5 sampling + 12.5 conversion = 19 ADC clocks.
 * Aggregate: 2 * 76 MHz / 19 = 8 MSPS (average).
 * Every DMA word: [31:16] ADC2 (newer), [15:0] ADC1.
 */
#define ADC_DMA_WORDS 200U /* 400 samples at 8 MSPS = 50 us */
static volatile uint32_t adc_dma_buffer[ADC_DMA_WORDS];

/* [F2] Latches for post-capture diagnosis. Do not print during the capture. */
static uint32_t capture_dma_flags = 0U;
static uint32_t capture_adc1_flags = 0U;
static uint32_t capture_adc2_flags = 0U;
static uint32_t capture_remaining_words = ADC_DMA_WORDS;
static uint32_t capture_error = 0U;  /* 1=DMA transfer, 2=ADC overrun, 3=timeout */

/* [F1] Main AHB/SYSCLK=80 MHz to service 4 million 32-bit DMA words/s.
 * ADC PLLSAI1R remains 76 MHz; USART2 uses independent HSI16 (16 MHz)
 * so the existing uart.c baud-rate calculation remains unchanged.
 * This routine expects reset clock configuration, with all PLLs OFF.
 */
static void adc_clock_76mhz_init(void){
    RCC->APB1ENR1 |= RCC_APB1ENR1_PWREN;
    PWR->CR1 = (PWR->CR1 & ~PWR_CR1_VOS) | PWR_CR1_VOS_0;
    while(PWR->SR2 & PWR_SR2_VOSF){}

    RCC->CR |= RCC_CR_HSION;
    while(!(RCC->CR & RCC_CR_HSIRDY)){}

    if(RCC->CR & (RCC_CR_PLLON | RCC_CR_PLLSAI1ON | RCC_CR_PLLSAI2ON)){
        while(1){} /* Other clock code is active; do not silently reconfigure */
    }

    /* Common input PLLM=2: HSI16/2 = 8 MHz. Main PLL N=20, R=2 -> 80 MHz. */
    RCC->PLLCFGR = (RCC->PLLCFGR &
                    ~(RCC_PLLCFGR_PLLSRC | RCC_PLLCFGR_PLLM |
                      RCC_PLLCFGR_PLLN | RCC_PLLCFGR_PLLR |
                      RCC_PLLCFGR_PLLREN))
                 | RCC_PLLCFGR_PLLSRC_HSI | RCC_PLLCFGR_PLLM_0
                 | (20U << RCC_PLLCFGR_PLLN_Pos) | RCC_PLLCFGR_PLLREN;

    /* PLLSAI1 N=19, R=2 -> 76 MHz ADC asynchronous clock. */
    RCC->PLLSAI1CFGR = (RCC->PLLSAI1CFGR &
                        ~(RCC_PLLSAI1CFGR_PLLSAI1N | RCC_PLLSAI1CFGR_PLLSAI1R))
                      | (19U << RCC_PLLSAI1CFGR_PLLSAI1N_Pos)
                      | RCC_PLLSAI1CFGR_PLLSAI1REN;

    /* Flash latency MUST be increased BEFORE raising SYSCLK to 80 MHz. */
    FLASH->ACR = (FLASH->ACR & ~FLASH_ACR_LATENCY) |
                  FLASH_ACR_LATENCY_4WS | FLASH_ACR_PRFTEN;
    while((FLASH->ACR & FLASH_ACR_LATENCY) != FLASH_ACR_LATENCY_4WS){}

    /* USART2 kernel = HSI16 at 16 MHz, regardless of APB1/SYSCLK. */
    RCC->CCIPR = (RCC->CCIPR & ~(RCC_CCIPR_USART2SEL | RCC_CCIPR_ADCSEL))
               | RCC_CCIPR_USART2SEL_1 | RCC_CCIPR_ADCSEL_0;

    RCC->CR |= RCC_CR_PLLON | RCC_CR_PLLSAI1ON;
    while(!(RCC->CR & RCC_CR_PLLRDY)){}
    while(!(RCC->CR & RCC_CR_PLLSAI1RDY)){}

    /* AHB=80 MHz, APB1=80 MHz (permitted in Range 1). */
    RCC->CFGR = (RCC->CFGR & ~(RCC_CFGR_HPRE | RCC_CFGR_PPRE1 |
                                RCC_CFGR_PPRE2 | RCC_CFGR_SW)) | RCC_CFGR_SW_PLL;
    while((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL){}
    /* Relojes RCC configurados directamente: no requiere SystemCoreClockUpdate(). */
}

static void adc_dma_init(void){
    /* [C1] DMA1 channel 1: 200 * 32-bit words, SINGLE-SHOT (no CIRC). */
    RCC->AHB1ENR |= RCC_AHB1ENR_DMA1EN;
    DMA1_Channel1->CCR = 0U;
    DMA1_CSELR->CSELR &= ~DMA_CSELR_C1S; /* request 0000 = ADC1 */
    DMA1_Channel1->CNDTR = ADC_DMA_WORDS;
    DMA1_Channel1->CPAR = (uint32_t)&ADC123_COMMON->CDR;
    DMA1_Channel1->CMAR = (uint32_t)adc_dma_buffer;
    DMA1->IFCR = DMA_IFCR_CGIF1;
    DMA1_Channel1->CCR = DMA_CCR_MINC |
                         DMA_CCR_PSIZE_1 | DMA_CCR_MSIZE_1 |
                         DMA_CCR_PL_0 | DMA_CCR_PL_1;
}

void pa1_adc_init(void){

    /*** Configure the ADC GPIO pin ***/

    /* Enable clock access to GPIOA */
    RCC->AHB2ENR |= GPIOAEN;

    /* Set the mode of PA1 to analog */
    GPIOA->MODER |= (1U<<2);
    GPIOA->MODER |= (1U<<3);

    /* Enable analog switch PA1 */
    GPIOA->ASCR |= (1U<<1);

    /*** Configure the ADC module ***/

    /* Enable clock access to ADC */
    RCC->AHB2ENR |= ADC1EN | ADC2EN; /* [M1] Add ADC2 */

    adc_clock_76mhz_init();         /* [M2] Dedicated ADC clock */

    /* [M4] Asynchronous ADC clock /1, dual interleaved, DELAY=3 cycles,
     * MDMA=10 for 12-bit data, one-shot DMA for the common 32-bit CDR.
     * Must be configured before either ADC is enabled.
     */
    ADC123_COMMON->CCR &= ~(ADC_CCR_CKMODE | ADC_CCR_PRESC |
                            ADC_CCR_DUAL | ADC_CCR_DELAY |
                            ADC_CCR_MDMA | ADC_CCR_DMACFG);
    ADC123_COMMON->CCR |= (ADC_CCR_DUAL_0 | ADC_CCR_DUAL_1 |
                            ADC_CCR_DUAL_2 | ADC_CCR_DELAY_1 |
                            ADC_CCR_MDMA_1); /* [C12] one-shot: common DMACFG=0 */

    /* [M1] ADC2 exits deep-power-down and enables its regulator. */
    ADC2->CR &= ~ADC_CR_DEEPPWD;
    ADC2->CR |= ADC_CR_ADVREGEN;

    /* NUEVO: Exit Deep-power-down */
    ADC1->CR &= ~ADC_CR_DEEPPWD;

    /* NUEVO: Enable ADC voltage regulator */
    ADC1->CR |= ADC_CR_ADVREGEN;

    /* NUEVO: Wait for regulator stabilization */
    for(volatile uint32_t i=0; i<4000U; i++){
        __NOP();
    }

    /* Calibrate both ADCs (single-ended). */
    ADC1->CR |= ADC_CR_ADCAL;
    ADC2->CR |= ADC_CR_ADCAL;
    while(ADC1->CR & ADC_CR_ADCAL){}
    while(ADC2->CR & ADC_CR_ADCAL){}

    /* NUEVO: Wait after calibration */
    for(volatile uint32_t i=0; i<100U; i++){
        __NOP();
    }

    /* NUEVO: Clear ADC ready flag */
    ADC1->ISR = ADC_ISR_ADRDY;
    ADC2->ISR = ADC_ISR_ADRDY;

    /* Enable both ADCs: slave first, then master. */
    ADC2->CR |= CR_ADON;
    while(!(ADC2->ISR & ADC_ISR_ADRDY)){}
    ADC1->CR |= CR_ADON;
    while(!(ADC1->ISR & ADC_ISR_ADRDY)){}

    /* [M5] PA1/IN6 is a SLOW ADC channel: SMP6=001 => 6.5 cycles. */
    ADC1->SMPR1 = (ADC1->SMPR1 & ~ADC_SMPR1_SMP6) | ADC_SMPR1_SMP6_0;
    ADC2->SMPR1 = (ADC2->SMPR1 & ~ADC_SMPR1_SMP6) | ADC_SMPR1_SMP6_0;

    /* Conversion sequence length */
    ADC1->SQR1 = ADC_SEQ_LEN_1;

    /* Conversion sequence start */
    ADC1->SQR1 |= ADC_CH6;
    ADC2->SQR1 = ADC_SEQ_LEN_1 | ADC_CH6; /* [M1] Same analog input */

    adc_dma_init(); /* [M3] Configure DMA before starting conversions */
}


void start_convertion(void){

    /* [F2] Reset error diagnostics for every new capture. */
    capture_dma_flags = capture_adc1_flags = capture_adc2_flags = 0U;
    capture_remaining_words = ADC_DMA_WORDS;
    capture_error = 0U;

    /* [C2] Rearm DMA for exactly 200 pairs; no conversions before B1. */
    DMA1_Channel1->CCR &= ~DMA_CCR_EN;
    DMA1->IFCR = DMA_IFCR_CGIF1;
    DMA1_Channel1->CNDTR = ADC_DMA_WORDS;

    /* [F4] Re-arm the ADC common one-shot DMA request mechanism.
     * MDMA fields are writable while ADSTART=0 (both ADCs stopped).
     * The prior capture consumed the one-shot request sequence. */
    ADC123_COMMON->CCR &= ~ADC_CCR_MDMA;
    ADC123_COMMON->CCR |= ADC_CCR_MDMA_1;

    /* [M4] Both ADCs in continuous mode; dual DMA requests via CCR.MDMA. */
    ADC1->CFGR |= CFGR_CONT | ADC_CFGR_OVRMOD; /* [F3] MDMA uses ADC_CCR, not CFGR.DMAEN */
    ADC2->CFGR |= CFGR_CONT | ADC_CFGR_OVRMOD;

    /* NUEVO: Clear previous EOC flag */
    ADC1->ISR = SR_EOC | ADC_ISR_OVR;
    ADC2->ISR = SR_EOC | ADC_ISR_OVR;

    DMA1_Channel1->CCR |= DMA_CCR_EN; /* [M3] Enable DMA before ADC */

    /* Start ADC conversion */
    ADC1->CR |= CR2_ADSTART;
}


/* [C3] Wait until the 200 DMA words have arrived, then stop BOTH ADCs.
 * Never read ADC_DR during interleaved acquisition: DMA reads ADC_CDR.
 * Returns 1 on completed capture, 0 on DMA/ADC error or timeout.
 * No UART operation is performed during the 50-us acquisition.
 */
int adc_capture_wait(void){
    uint32_t timeout = 2000000U; /* watchdog only; NOT a 50-us timer */
    uint32_t dma_flags;
    uint32_t adc_flags;
    int completed = 0;

    while(timeout-- != 0U){
        dma_flags = DMA1->ISR;
        if(dma_flags & DMA_ISR_TEIF1){
            capture_error = 1U;
            break;
        }
        /* Important: TCIF proves the full 200 paired DMA writes occurred. */
        if(dma_flags & DMA_ISR_TCIF1){
            completed = 1;
            break;
        }
        adc_flags = ADC1->ISR | ADC2->ISR;
        if(adc_flags & ADC_ISR_OVR){
            capture_error = 2U;
            break;
        }
    }
    if(!completed && capture_error == 0U){ capture_error = 3U; }

    /* Freeze diagnostics before attempting to stop the converters. */
    capture_dma_flags = DMA1->ISR;
    capture_adc1_flags = ADC1->ISR;
    capture_adc2_flags = ADC2->ISR;
    capture_remaining_words = DMA1_Channel1->CNDTR;

    /* ADC1 ADSTP stops both master and slave in dual mode. */
    if(ADC1->CR & ADC_CR_ADSTART){
        ADC1->CR |= ADC_CR_ADSTP;
        while(ADC1->CR & ADC_CR_ADSTART){}
    }
    DMA1_Channel1->CCR &= ~DMA_CCR_EN;
    __DMB();
    /* If DMA completed, later OVR after the one-shot boundary does not invalidate RAM. */
    return completed && ((capture_dma_flags & DMA_ISR_TEIF1) == 0U)
                     && (capture_remaining_words == 0U);
}

/* Diagnostic accessors. Evaluate ONLY after adc_capture_wait(). */
uint32_t adc_capture_error(void){ return capture_error; }
uint32_t adc_capture_dma_flags(void){ return capture_dma_flags; }
uint32_t adc_capture_adc1_flags(void){ return capture_adc1_flags; }
uint32_t adc_capture_adc2_flags(void){ return capture_adc2_flags; }
uint32_t adc_capture_remaining(void){ return capture_remaining_words; }

/* [C4] Extract 400 ordered 12-bit samples from 200 32-bit DMA words.
 * Even indices = ADC1 (master), odd indices = ADC2 (slave).
 * Call only after adc_capture_wait() returned 1.
 */
uint32_t adc_get_sample(uint32_t sample_index){
    if(sample_index >= ADC_CAPTURE_SAMPLES){return 0U;}
    uint32_t word = adc_dma_buffer[sample_index / 2U];
    return (sample_index & 1U) ? ((word >> 16U) & 0x0FFFU)
                                : (word & 0x0FFFU);
}

/* [C5] Nominal timing relative to the first MASTER sampling instant:
 * pair n: master = 19*n cycles, slave = 19*n + 10 cycles.
 * Nominal ADC clock is 76 MHz; times are integer nanoseconds rounded.
 * This is calculated timing, not hardware timestamping nor button latency.
 */
uint32_t adc_get_time_ns(uint32_t sample_index){
    if(sample_index >= ADC_CAPTURE_SAMPLES){return 0U;}
    uint32_t cycles = 19U * (sample_index / 2U)
                    + ((sample_index & 1U) ? 10U : 0U);
    return (cycles * 1000U + 38U) / 76U;
}

/* Preserve the original public function for compatibility. */
uint32_t adc_read(void){
    return adc_get_sample(ADC_CAPTURE_SAMPLES - 1U);
}
