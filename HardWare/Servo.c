#include "stm32f10x.h"                  // Device header
#include "PWM.h"
#include "Servo.h"

/* Power-on reference only; there is no position feedback. */
static float LastLegAngle[4] = {90, 90, 90, 90};
static float LastTailAngle = 90;

void Servo_Init(void)
{
	uint8_t leg;
	for (leg = 0; leg < 4; leg++) { LastLegAngle[leg] = 90; }
	LastTailAngle = 90;
	PWM_Init();
}

void Servo_Angle1(float Angle)//左上
{
	LastLegAngle[0] = Angle;
	PWM_SetCompare1(Angle / 180 * 2000 + 500);
}

void Servo_Angle2(float Angle)//右上
{
	LastLegAngle[1] = Angle;
	PWM_SetCompare2((180-Angle) / 180 * 2000 + 500);
}

void Servo_Angle3(float Angle)//左下
{
	LastLegAngle[2] = Angle;
	PWM_SetCompare3(Angle / 180 * 2000 + 500);
}

void Servo_Angle4(float Angle)//右下
{
	LastLegAngle[3] = Angle;
	PWM_SetCompare4((180-Angle) / 180 * 2000 + 500);
}

void WServo_Angle(float Angle)//尾巴
{
	LastTailAngle = Angle;
	PWM_WSetCompare(Angle / 180 * 2000 + 500);
}

float Servo_GetAngle(uint8_t leg)
{
    return (leg >= 1 && leg <= 4) ? LastLegAngle[leg - 1] : 90;
}

float WServo_GetAngle(void)
{
    return LastTailAngle;
}
