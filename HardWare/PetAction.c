#include "stm32f10x.h"                  // Device header
#include "Servo.h"
#include "Delay.h"
#include "BlueTooth.h"
#include "PetAction.h"

	/*舵机位置
	    1        2




	    3        4
	*/

	//舵机充当腿，括号里的值为0时表示的是腿向前进的方向甩，90度是站立

#define Chongfunumber 2  //动作重复次数

uint16_t PAnumbers=Chongfunumber;//动作重复次数

uint16_t TiaoTurn=0;
uint16_t TiaoTurn2=0;



/* Commands are applied only here/main, never by an ISR. Checking a request
 * after every millisecond also handles repeated requests for the same mode.
 * OLED transfers may add latency; this is not a nonblocking action scheduler.
 */
static uint8_t Action_IsCurrent(uint16_t mode, uint32_t request)
{
    return Action_Mode == mode && Action_RequestId == request
        && (mode != 9 || WeiBa != 0);
}

static uint8_t Action_Wait(uint16_t mode, uint32_t request, uint32_t ms)
{
    BlueTooth_Process();
    if (!Action_IsCurrent(mode, request)) { return 0; }
    while (ms--) {
        Delay_ms(1);
        BlueTooth_Process();
        if (!Action_IsCurrent(mode, request)) { return 0; }
    }
    return 1;
}

void Action_relaxed_getdowm(void)
{
	uint32_t request = Action_RequestId;
	if (!Action_Wait(0, request, 0)) { return; }
	Servo_Angle1(20);
	Servo_Angle2(20);
	if (!Action_Wait(0, request, 80)) { return; }
	Servo_Angle3(160);
	Servo_Angle4(160);
	WServo_Angle(90);
}

void Action_upright(void)//站立
{
	uint32_t request = Action_RequestId;
	if (!Action_Wait(2, request, 0)) { return; }
	Servo_Angle1(90);
	Servo_Angle2(90);
	if (!Action_Wait(2, request, 80)) { return; }
	Servo_Angle3(90);
	Servo_Angle4(90);

	if(WeiBa==1)
	{
		Action_Mode=9;
	}
}

void Action_upright2(void)//站立
{
	uint32_t request = Action_RequestId;
	if (!Action_Wait(12, request, 0)) { return; }
	Servo_Angle3(90);
	Servo_Angle4(90);
	if (!Action_Wait(12, request, 80)) { return; }
	Servo_Angle1(90);
	Servo_Angle2(90);

	if(WeiBa==1)
	{
		Action_Mode=9;
	}
}

void Action_getdowm(void)//趴下
{
	uint32_t request = Action_RequestId;
	if (!Action_Wait(3, request, 0)) { return; }
	Servo_Angle1(20);
	Servo_Angle2(20);
	if (!Action_Wait(3, request, 80)) { return; }
	Servo_Angle3(20);
	Servo_Angle4(20);

	if(WeiBa==1)
	{
		Action_Mode=9;
	}
}

void Action_sit(void)//坐下
{
	uint32_t request = Action_RequestId;
	if (!Action_Wait(1, request, 0)) { return; }
	Servo_Angle1(90);
	Servo_Angle2(90);
	if (!Action_Wait(1, request, 80)) { return; }
	Servo_Angle3(20);
	Servo_Angle4(20);

	if(WeiBa==1)
	{
		Action_Mode=9;
	}
}
//作者是Sngels_wyh只在抖音与B站


void Action_advance(void)//前进
{
	uint32_t request = Action_RequestId;
	if (!Action_Wait(4, request, 0)) { return; }

	while(Action_Mode==4)
	{
		PAnumbers=Chongfunumber;
			while((PAnumbers || Sustainedmove)&& Action_Mode==4)
			{
				Servo_Angle2(45);
				Servo_Angle3(45);
				if (!Action_Wait(4, request, SpeedDelay)) { return; }

				Servo_Angle1(135);
				Servo_Angle4(135);
				if (!Action_Wait(4, request, SpeedDelay)) { return; }

				Servo_Angle2(90);
				Servo_Angle3(90);
				if (!Action_Wait(4, request, SpeedDelay)) { return; }

				Servo_Angle1(90);
				Servo_Angle4(90);
				if (!Action_Wait(4, request, SpeedDelay)) { return; }


				Servo_Angle1(45);
				Servo_Angle4(45);
				if (!Action_Wait(4, request, SpeedDelay)) { return; }

				Servo_Angle2(135);
				Servo_Angle3(135);
				if (!Action_Wait(4, request, SpeedDelay)) { return; }

				Servo_Angle1(90);
				Servo_Angle4(90);
				if (!Action_Wait(4, request, SpeedDelay)) { return; }

				Servo_Angle2(90);
				Servo_Angle3(90);
				if (!Action_Wait(4, request, SpeedDelay)) { return; }


				if (PAnumbers > 0) { PAnumbers--; }
			}
			if(Sustainedmove!=1 && Action_Mode==4)
				Action_Mode=2;
	}
}

void Action_back(void)//后退
{
	uint32_t request = Action_RequestId;
	if (!Action_Wait(5, request, 0)) { return; }
	while(Action_Mode==5)
	{
		PAnumbers=Chongfunumber;
		while((PAnumbers || Sustainedmove) && Action_Mode==5 )
		{
			Servo_Angle2(135);
			Servo_Angle3(135);
			if (!Action_Wait(5, request, SpeedDelay)) { return; }

			Servo_Angle1(45);
			Servo_Angle4(45);
			if (!Action_Wait(5, request, SpeedDelay)) { return; }

			Servo_Angle2(90);
			Servo_Angle3(90);
			if (!Action_Wait(5, request, SpeedDelay)) { return; }

			Servo_Angle1(90);
			Servo_Angle4(90);
			if (!Action_Wait(5, request, SpeedDelay)) { return; }


			Servo_Angle1(135);
			Servo_Angle4(135);
			if (!Action_Wait(5, request, SpeedDelay)) { return; }

			Servo_Angle2(45);
			Servo_Angle3(45);
			if (!Action_Wait(5, request, SpeedDelay)) { return; }

			Servo_Angle1(90);
			Servo_Angle4(90);
			if (!Action_Wait(5, request, SpeedDelay)) { return; }

			Servo_Angle2(90);
			Servo_Angle3(90);
			if (!Action_Wait(5, request, SpeedDelay)) { return; }


			if (PAnumbers > 0) { PAnumbers--; }
		}
		if(Sustainedmove!=1 && Action_Mode==5)
		Action_Mode=2;


	}
}

void Action_Lrotation(void)//向左旋转
{
	uint32_t request = Action_RequestId;
	if (!Action_Wait(6, request, 0)) { return; }
	while(Action_Mode==6)
	{
		PAnumbers=Chongfunumber;
		PAnumbers=PAnumbers+Chongfunumber;
		while((PAnumbers || Sustainedmove) && Action_Mode==6)
		{
			Servo_Angle2(45);
			Servo_Angle3(135);
			if (!Action_Wait(6, request, SpeedDelay)) { return; }

			Servo_Angle1(45);
			Servo_Angle4(135);
			if (!Action_Wait(6, request, SpeedDelay)) { return; }

			Servo_Angle2(90);
			Servo_Angle3(90);
			if (!Action_Wait(6, request, SpeedDelay)) { return; }

			Servo_Angle1(90);
			Servo_Angle4(90);
			if (!Action_Wait(6, request, SpeedDelay)) { return; }


			if (PAnumbers > 0) { PAnumbers--; }
		}
		if(Sustainedmove!=1 && Action_Mode==6)
		Action_Mode=2;
	}

}


void Action_Rrotation(void)//向右旋转
{
	uint32_t request = Action_RequestId;
	if (!Action_Wait(7, request, 0)) { return; }
	while(Action_Mode==7)
	{
		PAnumbers=Chongfunumber;
		PAnumbers=PAnumbers+Chongfunumber;
		while((PAnumbers || Sustainedmove)  && Action_Mode==7)
		{
			Servo_Angle1(45);
			Servo_Angle4(135);
			if (!Action_Wait(7, request, SpeedDelay)) { return; }

			Servo_Angle2(45);
			Servo_Angle3(135);
			if (!Action_Wait(7, request, SpeedDelay)) { return; }

			Servo_Angle1(90);
			Servo_Angle4(90);
			if (!Action_Wait(7, request, SpeedDelay)) { return; }

			Servo_Angle2(90);
			Servo_Angle3(90);
			if (!Action_Wait(7, request, SpeedDelay)) { return; }


			if (PAnumbers > 0) { PAnumbers--; }
		}
		if(Sustainedmove!=1 && Action_Mode==7)
		Action_Mode=2;
	}

}

void Action_Swing(void)//摇摆
{
	uint32_t request = Action_RequestId;
	if (!Action_Wait(8, request, 0)) { return; }
	while(Action_Mode==8)
	{
		for(uint8_t i=30;i<150;i++)
		{
			Servo_Angle1(i);
			Servo_Angle2(i);
			Servo_Angle3(i);
			Servo_Angle4(i);
			if (!Action_Wait(8, request, SwingDelay)) { return; }

		}
		for(uint8_t i=150;i>30;i--)
		{
			Servo_Angle1(i);
			Servo_Angle2(i);
			Servo_Angle3(i);
			Servo_Angle4(i);
			if (!Action_Wait(8, request, SwingDelay)) { return; }

		}

	}
}


void Action_SwingTail(void)//摇尾巴
{
	uint32_t request = Action_RequestId;
	if (!Action_Wait(9, request, 0)) { return; }
	if (!Action_Wait(9, request, 60)) { return; }
	while(Action_Mode==9)
	{
		for(uint8_t i=30;i<150;i++)
		{
			WServo_Angle(i);
			if (!Action_Wait(9, request, SwingDelay)) { return; }

		}
		for(uint8_t i=150;i>30;i--)
		{
			WServo_Angle(i);
			if (!Action_Wait(9, request, SwingDelay)) { return; }

		}

	}
	if (!Action_Wait(9, request, 100)) { return; }
}

void Action_JumpU(void)//向前跳
{
	uint32_t request = Action_RequestId;
	if (!Action_Wait(10, request, 0)) { return; }
	if(TiaoTurn==0)
	{
		Servo_Angle1(140);
		Servo_Angle4(35);
		if (!Action_Wait(10, request, SpeedDelay)) { return; }

		Servo_Angle2(140);
		Servo_Angle3(35);
		if (!Action_Wait(10, request, SpeedDelay+80)) { return; }

		Action_Mode=2;
		TiaoTurn=1;
	}
	else
	{
		Servo_Angle2(140);
		Servo_Angle3(35);
		if (!Action_Wait(10, request, SpeedDelay)) { return; }

		Servo_Angle1(140);
		Servo_Angle4(35);
		if (!Action_Wait(10, request, SpeedDelay+80)) { return; }

		Action_Mode=2;
		TiaoTurn=0;
	}
}

void Action_JumpD(void)//向后跳
{
	uint32_t request = Action_RequestId;
	if (!Action_Wait(11, request, 0)) { return; }
	if(TiaoTurn2==0){
		Servo_Angle4(35);
		Servo_Angle1(140);
		if (!Action_Wait(11, request, SpeedDelay)) { return; }

		Servo_Angle3(35);
		Servo_Angle2(140);
		if (!Action_Wait(11, request, SpeedDelay)) { return; }

		Action_Mode=12;
		TiaoTurn2=1;
	}
	else
	{
		Servo_Angle3(35);
		Servo_Angle2(140);
		if (!Action_Wait(11, request, SpeedDelay)) { return; }

		Servo_Angle4(35);
		Servo_Angle1(140);
		if (!Action_Wait(11, request, SpeedDelay)) { return; }

		Action_Mode=12;
		TiaoTurn2=0;
	}
}

void Action_Hello(void)
{
	uint32_t request = Action_RequestId;
	if (!Action_Wait(13, request, 0)) { return; }
	Servo_Angle3(20);
	Servo_Angle4(45);
	if (!Action_Wait(13, request, 80)) { return; }
	Servo_Angle1(90);
	while(Action_Mode==13)
	{

		for(int i=0;i<=45;i++)
		{

			Servo_Angle2(i);
			if (!Action_Wait(13, request, SwingDelay)) { return; }
		}
		for(int i=45;i>0;i--)
		{

			Servo_Angle2(i);
			if (!Action_Wait(13, request, SwingDelay)) { return; }
		}

	}
}

void Action_stretch(void)//伸懒腰
{
	uint32_t request = Action_RequestId;
	if (!Action_Wait(14, request, 0)) { return; }
	Servo_Angle3(90);
	Servo_Angle4(90);
	if (!Action_Wait(14, request, 80)) { return; }
	for(int i=90;i>10;i--)
	{
		Servo_Angle1(i);
		Servo_Angle2(i);

		if (!Action_Wait(14, request, 15)) { return; }
	}
	for(int i=10;i<90;i++)
	{
		Servo_Angle1(i);
		Servo_Angle2(i);

		if (!Action_Wait(14, request, 15)) { return; }
	}
	for(int i=90;i<170;i++)
	{
		Servo_Angle3(i);
		Servo_Angle4(i);

		if (!Action_Wait(14, request, 15)) { return; }
	}

	for(int i=170;i>90;i--)
	{
		Servo_Angle3(i);
		Servo_Angle4(i);

		if (!Action_Wait(14, request, 15)) { return; }
	}
	if(Action_Mode==14)
	Action_Mode=15;
}

void Action_Lstretch(void)//后腿拉伸
{
	uint32_t request = Action_RequestId;
	int breakvalue=1;
	int temp=3;
	if (!Action_Wait(15, request, 0)) { return; }
	while(breakvalue)
	{
		Servo_Angle1(90);
		Servo_Angle2(20);
		if (!Action_Wait(15, request, 60)) { return; }
		Servo_Angle4(110);
		for(int i=90;i<180;i++)
		{

			Servo_Angle3(i);
			if (!Action_Wait(15, request, 6)) { return; }
		}
		while(temp && Action_Mode==15)
		{
			for(int i=180;i>150;i--)
			{

				Servo_Angle3(i);
				if (!Action_Wait(15, request, 15)) { return; }
			}
			temp--;
		}

		if (!Action_Wait(15, request, 100)) { return; }
		Servo_Angle1(90);
		Servo_Angle2(90);

		if (!Action_Wait(15, request, 80)) { return; }
		Servo_Angle3(90);
		Servo_Angle4(90);
		if (!Action_Wait(15, request, 100)) { return; }


		temp=3;

		Servo_Angle2(90);
		Servo_Angle1(20);

		if (!Action_Wait(15, request, 60)) { return; }
		Servo_Angle3(110);
		for(int i=90;i<180;i++)
		{

			Servo_Angle4(i);
			if (!Action_Wait(15, request, 6)) { return; }
		}
		while(temp && Action_Mode==15)
		{
			for(int i=180;i>150;i--)
			{

				Servo_Angle4(i);
				if (!Action_Wait(15, request, 15)) { return; }
			}
			temp--;
		}
		if(Action_Mode==15)
		Action_Mode=2;


		breakvalue=0;
	}
}

/* Move from the last commanded pose in 20 ms frames, matching the servo PWM
 * period. This preserves continuity when a repeated request restarts the move.
 */
static uint8_t Affection_Pose(uint32_t request, const float target[5], uint16_t frames)
{
    float start[5];
    float progress;
    uint16_t frame;
    uint8_t leg;
    if (!Action_Wait(ACTION_MODE_AFFECTION, request, 0)) { return 0; }
    for (leg = 0; leg < 4; leg++) { start[leg] = Servo_GetAngle(leg + 1); }
    start[4] = WServo_GetAngle();
    for (frame = 1; frame <= frames; frame++) {
        progress = (float)frame / frames;
        Servo_Angle1(start[0] + (target[0] - start[0]) * progress);
        Servo_Angle2(start[1] + (target[1] - start[1]) * progress);
        Servo_Angle3(start[2] + (target[2] - start[2]) * progress);
        Servo_Angle4(start[3] + (target[3] - start[3]) * progress);
        WServo_Angle(start[4] + (target[4] - start[4]) * progress);
        if (!Action_Wait(ACTION_MODE_AFFECTION, request, 20)) { return 0; }
    }
    return 1;
}

static uint8_t Affection_Tail(uint32_t request, float target, uint16_t frames)
{
    float start;
    uint16_t frame;
    if (!Action_Wait(ACTION_MODE_AFFECTION, request, 0)) { return 0; }
    start = WServo_GetAngle();
    for (frame = 1; frame <= frames; frame++) {
        WServo_Angle(start + (target - start) * (float)frame / frames);
        if (!Action_Wait(ACTION_MODE_AFFECTION, request, 20)) { return 0; }
    }
    return 1;
}

/* Ask for a pat: crouch, lean left/right, wag three times, then stand.
 * Nominal duration: 4200 ms plus foreground/interrupt overhead.
 * WeiBa is never overwritten: the user's previous tail-enable state survives.
 */
void Action_Affection(void)
{
    static const float crouch[5] = {70, 70, 70, 70, 90};
    static const float left[5] = {65, 85, 65, 85, 90};
    static const float right[5] = {85, 65, 85, 65, 90};
    static const float stand[5] = {90, 90, 90, 90, 90};
    uint32_t request = Action_RequestId;
    uint8_t cycle;
    if (!Action_Wait(ACTION_MODE_AFFECTION, request, 0)) { return; }
    Face_Mode = 1;
    if (!Affection_Pose(request, crouch, 20)) { return; }
    if (!Action_Wait(ACTION_MODE_AFFECTION, request, 300)) { return; }
    if (!Affection_Pose(request, left, 20)) { return; }
    if (!Action_Wait(ACTION_MODE_AFFECTION, request, 150)) { return; }
    if (!Affection_Pose(request, right, 30)) { return; }
    if (!Action_Wait(ACTION_MODE_AFFECTION, request, 150)) { return; }
    if (!Affection_Pose(request, crouch, 20)) { return; }
    Face_Mode = 2;
    if (!Affection_Tail(request, 75, 5)) { return; }
    for (cycle = 0; cycle < 3; cycle++) {
        if (!Affection_Tail(request, 105, 10)) { return; }
        if (!Affection_Tail(request, 75, 10)) { return; }
    }
    if (!Affection_Tail(request, 90, 5)) { return; }
    if (!Affection_Pose(request, stand, 20)) { return; }
    /* No old completion/cleanup runs after a newer request takes control. */
    Face_Mode = 2;
    Action_Mode = 2;
}
