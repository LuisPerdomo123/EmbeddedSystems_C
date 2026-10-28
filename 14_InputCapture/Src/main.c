#include <stdio.h>
#include <stdint.h>
#include "stm32l4xx.h"
#include "uart.h"
#include "adc.h"
#include "systick.h"
#include "timer.h"


/* CORRECCION 4: volatile evita que el compilador elimine u optimice
 * la escritura de timestamp; permite observar las capturas en Debug.
 * TIM3->CCR1 es de 16 bits y uint32_t permite almacenarlo sin signo. */
volatile uint32_t timestamp = 0;


/*Set uo: Connect a jumper wire from PA5 to PA6*/

int main(void){


	tim2_pa5_output_compare();
	tim3_pa6_input_capture();

    while(1){


    	/*Wait until edge is captured*/
    	while(!(TIM3->SR & SR_CC1IF)){}
    	/*Read capture value*/
    	timestamp = TIM3->CCR1;

    }
}

