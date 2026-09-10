#include "uart_terminal.h"
#include "custom_command_handler.h"

#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

LedBlink ledsBlink[3] = {0};

void custom_Command_Handler(int argc, char *argv[]) {

	if (strcmp(argv[0], "v") == 0) {
		UART_Terminal_Send(UART_TERMINAL_VERSION);
		return;
	}
	if (strcmp(argv[0], "clear") == 0) {
			UART_Terminal_Send("\033[2J\033[H");  // Codes ANSI pour effacer l'écran
			return;
	}
	if (strcmp(argv[0], "led1on") == 0) {
		HAL_GPIO_TogglePin(LED1_GPIO_Port, LED1_Pin);
		return;
	}
	if (strcmp(argv[0], "led1off") == 0) {
		HAL_GPIO_TogglePin(LED1_GPIO_Port, LED1_Pin);
		return;
	}
	if (strcmp(argv[0], "btn?") == 0) {
		UART_Terminal_Printf("\n\r SW1 : %d \n\r", HAL_GPIO_ReadPin(SW1_GPIO_Port, SW1_Pin));
		return;
	}
	if (strcmp(argv[0], "led") == 0) {
		int n = strtoul(argv[1], NULL, 10);
		int state = strtoul(argv[2], NULL, 10);
		if(( state != 1) && (state != 0) ){
			UART_Terminal_Printf("\n\r Invalid State: %d , Should be 0 or 1 \n\r", state);
			return;
		}

		switch (n) {
			case 1:
				HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, state);
				return;
			case 2:
				HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, state);
				break;
			case 3:
				HAL_GPIO_WritePin(LED3_GPIO_Port, LED3_Pin, state);
				break;
			default:
				UART_Terminal_Printf("\n\r Invalid Pin : %d \n\r", n);
				break;
		}
		return;

	}
	if (strcmp(argv[0], "blink") == 0) {
			int n = strtoul(argv[1], NULL, 10);
			int per = strtoul(argv[2], NULL, 10);
			int idx = n - 1;
			ledsBlink[idx].per = per;
			ledsBlink[idx].lastTime = HAL_GetTick();
			return;

	}
}

void updateLedBlinks(){
	uint32_t time = HAL_GetTick();
	for(int i = 0; i<3; i++){
		if(ledsBlink[i].per > 0){
			if(time - ledsBlink[i].lastTime >= ledsBlink[i].per){
				switch (i) {
					case 1:
						HAL_GPIO_TogglePin(LED1_GPIO_Port, LED1_Pin);
						return;
					case 2:
						HAL_GPIO_TogglePin(LED2_GPIO_Port, LED2_Pin);
						break;
					case 3:
						HAL_GPIO_TogglePin(LED3_GPIO_Port, LED3_Pin);
						break;
					default:
						UART_Terminal_Printf("\n\r Invalid Pin : %d \n\r", i);
						break;
				}
			}
		}
	}


}



