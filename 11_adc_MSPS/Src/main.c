#include <stdio.h>
#include <stdint.h>
#include "stm32l4xx.h"
#include "uart.h"
#include "adc.h"

#define USER_BUTTON  (1U<<13) /* B1 = PC13, active LOW */

uint32_t sensor_value;

int main(void){

    pa1_adc_init();
    uart2_tx_init();

    /* [C7] Enable PC13 digital input (B1 USER). */
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOCEN;
    GPIOC->MODER &= ~(3U << 26); /* PC13 = input */

    printf("#READY_V3,hclk_hz=%lu,adcclk_hz=76000000,press_B1_to_capture_50us\r\n",
           80000000UL); /* Freq. nominal fijada en adc_clock_76mhz_init() */

    while(1){
        /* [C8] Wait for blue button press. LOW = pressed. */
        while((GPIOC->IDR & USER_BUTTON) != 0U){}

        /* [C9] Capture and STORE first 400 samples before any USB/USART output. */
        start_convertion();
        if(adc_capture_wait()){
            printf("#CAPTURE,fs_nominal_hz=8000000,window_us=50,samples=400\r\n");
            printf("index,time_ns,delta_ns,adc12\r\n");

            /* [C10] UART2 -> ST-LINK -> USB Virtual COM Port; after capture only. */
            uint32_t previous_ns = 0U;
            for(uint32_t i = 0U; i < ADC_CAPTURE_SAMPLES; i++){
                uint32_t time_ns = adc_get_time_ns(i);
                uint32_t delta_ns = (i == 0U) ? 0U : (time_ns - previous_ns);
                previous_ns = time_ns;
                sensor_value = adc_get_sample(i);
                printf("%lu,%lu,%lu,%lu\r\n",
                       (unsigned long)i,
                       (unsigned long)time_ns,
                       (unsigned long)delta_ns,
                       (unsigned long)sensor_value);
            }
            printf("#END\r\n");
        }else{
            /* [F2] Distinguish DMA transfer failure, ADC overrun and timeout. */
            const char *reason = (adc_capture_error() == 1U) ? "DMA_TRANSFER" :
                                 (adc_capture_error() == 2U) ? "ADC_OVERRUN" :
                                 "TIMEOUT";
            printf("#ERROR,reason=%s,dma_isr=0x%08lX,adc1_isr=0x%08lX,adc2_isr=0x%08lX,remaining=%lu\r\n",
                   reason,
                   (unsigned long)adc_capture_dma_flags(),
                   (unsigned long)adc_capture_adc1_flags(),
                   (unsigned long)adc_capture_adc2_flags(),
                   (unsigned long)adc_capture_remaining());
        }

        /* [C11] A held button must not retrigger the capture. */
        while((GPIOC->IDR & USER_BUTTON) == 0U){}
    }
}
