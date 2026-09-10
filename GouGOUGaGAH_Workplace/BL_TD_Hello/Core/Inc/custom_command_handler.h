 /* custom_command_handler.h
 *
 *  Created on: 8 sept. 2026
 *      Author: geii
 */

#ifndef INC_CUSTOM_COMMAND_HANDLER_H_
#define INC_CUSTOM_COMMAND_HANDLER_H_

typedef struct{
	int per;
	int lastTime;
}LedBlink;
extern LedBlink ledsBlink[3];

void updateLedBlinks();

#endif /* INC_CUSTOM_COMMAND_HANDLER_H_ */
