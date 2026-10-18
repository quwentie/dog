#ifndef __SERVO_H
#define __SERVO_H

#include <stdint.h>

void Servo_Init(void);
void Servo_Angle1(float Angle);
void Servo_Angle2(float Angle);
void Servo_Angle3(float Angle);
void Servo_Angle4(float Angle);
void WServo_Angle(float Angle);

/* Last commanded angles, not measured mechanical positions. Legs are 1..4. */
float Servo_GetAngle(uint8_t leg);
float WServo_GetAngle(void);

#endif

