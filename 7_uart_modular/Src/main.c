#include <stdio.h>
#include <stdint.h>
#include "stm32l4xx.h"
#include "uart.h"


int main(void){

    uart2_tx_init();

    while(1){

    	printf("Hello from STM32L4.......\n\r");
    }
}

