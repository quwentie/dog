#ifndef __BLUE_TOOTH_H
#define __BLUE_TOOTH_H

#include <stdint.h>

extern volatile uint16_t Action_Mode;
extern volatile uint16_t Face_Mode;
extern volatile uint16_t SpeedDelay;
extern volatile uint16_t SwingDelay;
extern volatile uint8_t WeiBa;
extern volatile uint16_t AllLed;
extern volatile uint16_t BreatheLed;
extern volatile uint16_t Sustainedmove;
extern volatile uint32_t Action_RequestId;
/* Diagnostics: dropped queue events and error-marked USART receives. */
extern volatile uint32_t Serial_RxOverflow;
extern volatile uint32_t Voice_RxErrors;
extern volatile uint32_t BlueTooth_RxErrors;

void BlueTooth_Init(void);
/* Call from foreground only, regularly during long-running actions. */
void BlueTooth_Process(void);

#endif
