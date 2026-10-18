#include <assert.h>
#include <setjmp.h>
#include <stdio.h>
#include <string.h>
#include "stm32f10x.h"
#include "BlueTooth.h"
#include "PetAction.h"
#include "OLED.h"
#include "Servo.h"

void USART1_IRQHandler(void);
void USART3_IRQHandler(void);
void TIM3_IRQHandler(void);
int pet_firmware_main(void);
extern uint16_t PAnumbers, TiaoTurn, TiaoTurn2;

USART_TypeDef TestUSART1, TestUSART3;
GPIO_TypeDef TestGPIOA, TestGPIOB;
TIM_TypeDef TestTIM3;
const uint8_t Face_sleep[1] = {0}, Face_stare[1] = {1};
const uint8_t Face_happy[1] = {2}, Face_mania[1] = {3};
const uint8_t Face_very_happy[1] = {4}, Face_eyes[1] = {5};
const uint8_t Face_hello[1] = {6};
static uint32_t Primask, Reads[2], Frames, Writes, Ticks, Trigger;
static uint32_t WritesAtTrigger, RequestAtTrigger;
static uint32_t Led1, Led2;
static uint8_t TriggerCommand, TriggerSource;
static int InIRQ, PreemptVoice, NextByte;
static float TailAngle;
static const uint8_t *LastImage;
static unsigned Runs;
static float LegAngles[4];
static jmp_buf FirmwareExit;
static uint32_t StopFirmware;
typedef struct { unsigned channel; float angle; uint32_t tick; } Output;
static Output Trace[1024];
static unsigned TraceCount;
static int RecordTrace;
static uint16_t Pulses[5];

static void send_status(unsigned source, uint8_t data, uint32_t status)
{
    USART_TypeDef *port = source ? USART3 : USART1;
    port->SR = status;
    port->DR = data;
    InIRQ++;
    if (source) { USART3_IRQHandler(); } else { USART1_IRQHandler(); }
    InIRQ--;
}
static void send(unsigned source, uint8_t data)
{
    send_status(source, data, USART_SR_RXNE);
}
static void command(unsigned source, uint8_t data)
{
    send(source, data);
    BlueTooth_Process();
}
uint32_t __get_PRIMASK(void) { return Primask; }
void __disable_irq(void) { Primask = 1; }
void __set_PRIMASK(uint32_t value) { Primask = value; }
uint16_t USART_ReceiveData(USART_TypeDef *port)
{
    unsigned source = port == USART3;
    uint16_t data = (uint16_t)port->DR;
    Reads[source]++;
    port->SR = 0; /* Model SR/DR receive/error acknowledgement. */
    if (NextByte >= 0) {
        port->DR = (uint32_t)NextByte;
        port->SR = USART_SR_RXNE;
        NextByte = -1;
    }
    if (source && PreemptVoice) {
        assert(Primask == 0);
        PreemptVoice = 0;
        send(0, 0x30);
    }
    return data;
}
void RCC_APB2PeriphClockCmd(uint32_t p, int e) { (void)p; (void)e; }
void RCC_APB1PeriphClockCmd(uint32_t p, int e) { (void)p; (void)e; }
void GPIO_Init(GPIO_TypeDef *p, GPIO_InitTypeDef *c) { (void)p; (void)c; }
void USART_Init(USART_TypeDef *p, USART_InitTypeDef *c)
{
    (void)p;
    assert(c->USART_BaudRate == 9600 && c->USART_WordLength == 8);
    assert(c->USART_Parity == 0 && c->USART_StopBits == 1);
}
void USART_ITConfig(USART_TypeDef *p, uint32_t i, int e)
{ (void)p; (void)i; (void)e; }
void USART_Cmd(USART_TypeDef *p, int e) { (void)p; (void)e; }
void NVIC_PriorityGroupConfig(uint32_t g) { (void)g; }
void NVIC_Init(NVIC_InitTypeDef *c) { (void)c; }
int TIM_GetITStatus(TIM_TypeDef *p, uint32_t i) { (void)p; (void)i; return SET; }
void TIM_ClearITPendingBit(TIM_TypeDef *p, uint32_t i) { (void)p; (void)i; }
void PWM_LED1(uint16_t v) { Led1 = v; }
void PWM_LED2(uint16_t v) { Led2 = v; }
void PWM_Init(void) {}
void OLED_Init(void) {}
void OLED_Clear(void) { assert(!InIRQ); }
void OLED_ShowImage(int16_t x, int16_t y, uint8_t w, uint8_t h, const uint8_t *p)
{
    assert(!InIRQ && x == 0 && y == 0 && w == 128 && h == 64);
    LastImage = p;
}
void OLED_Update(void) { assert(!InIRQ); Frames++; }
static void servo(float angle)
{
    assert(!InIRQ && angle >= 0 && angle <= 180);
    Writes++;
}
static void pwm(unsigned channel, uint16_t pulse)
{
    float v = channel < 4 ? Servo_GetAngle((uint8_t)(channel + 1)) : WServo_GetAngle();
    servo(v);
    assert(pulse >= 500 && pulse <= 2500);
    Pulses[channel] = pulse;
    if (channel < 4) { LegAngles[channel] = v; } else { TailAngle = v; }
    if (RecordTrace) {
        assert(TraceCount < sizeof Trace / sizeof Trace[0]);
        Trace[TraceCount++] = (Output){channel, v, Ticks};
    }
}
void PWM_SetCompare1(uint16_t v) { pwm(0, v); }
void PWM_SetCompare2(uint16_t v) { pwm(1, v); }
void PWM_SetCompare3(uint16_t v) { pwm(2, v); }
void PWM_SetCompare4(uint16_t v) { pwm(3, v); }
void PWM_WSetCompare(uint16_t v) { pwm(4, v); }
void Delay_ms(uint32_t ms)
{
    assert(!InIRQ);
    /* No real sleeps: each tick is an injection point for an incoming command. */
    while (ms--) {
        Ticks++;
        assert(Ticks < 20000);
        if (StopFirmware && Ticks == StopFirmware) { longjmp(FirmwareExit, 1); }
        if (Trigger && Ticks == Trigger) {
            WritesAtTrigger = Writes;
            RequestAtTrigger = Action_RequestId;
            send(TriggerSource, TriggerCommand);
        }
    }
}

static void reset(void)
{
    assert(!InIRQ);
    Primask = 0;
    memset(Reads, 0, sizeof Reads);
    TestUSART1.SR = TestUSART3.SR = 0;
    PreemptVoice = 0;
    NextByte = -1;
    Action_Mode = Face_Mode = WeiBa = Sustainedmove = 0;
    SpeedDelay = 200;
    SwingDelay = 6;
    AllLed = 1;
    BreatheLed = 0;
    Action_RequestId = 0;
    PAnumbers = 2;
    TiaoTurn = TiaoTurn2 = 0;
    Frames = Writes = Ticks = Trigger = 0;
    StopFirmware = 0;
    TailAngle = 90;
    TraceCount = 0;
    RecordTrace = 0;
    Servo_Init();
    BlueTooth_Init();
    BlueTooth_Process();
    assert(Frames == 1 && LastImage == Face_sleep);
    Frames = 0;
}

static void test_action_protocol(void)
{
    static const uint8_t cases[][3] = {
        {0x29,0,0}, {0x30,1,1}, {0x31,2,5}, {0x32,3,1},
        {0x33,4,2}, {0x34,5,2}, {0x35,6,2}, {0x36,7,2},
        {0x37,8,4}, {0x41,10,2}, {0x42,11,2}, {0x43,13,6},
        {0x48,14,6}, {0x49,15,6}, {0x50,ACTION_MODE_AFFECTION,1}
    };
    static const uint8_t *const faces[] = {
        Face_sleep,Face_stare,Face_happy,Face_mania,Face_very_happy,Face_eyes,Face_hello
    };
    unsigned s, i;
    for (s = 0; s < 2; s++) {
        for (i = 0; i < sizeof cases / sizeof cases[0]; i++) {
            reset();
            send(s, cases[i][0]);
            assert(Reads[s] == 1 && Action_Mode == 0 && Frames == 0);
            BlueTooth_Process();
            assert(Action_Mode == cases[i][1] && Face_Mode == cases[i][2]);
            assert(Sustainedmove == s && Action_RequestId == 1);
            assert(LastImage == faces[cases[i][2]]);
            BlueTooth_Process();
            assert(Frames == (cases[i][2] != 0));
            Runs++;
        }
    }
}

static void test_ancillary_and_invalid_commands(void)
{
    unsigned source, i;
    for (source = 0; source < 2; source++) {
        uint32_t request;
        reset();
        command(!source, 0x33);
        request = Action_RequestId;
        for (i = 0; i < 6; i++) { command(source, 0x38); }
        assert(SpeedDelay == 200 && Face_Mode == 2);
        for (i = 0; i < 3; i++) { command(source, 0x39); }
        assert(SwingDelay == 3 && Face_Mode == 3);
        command(source, 0x39);
        assert(SwingDelay == 9 && Face_Mode == 4);
        command(source, 0x45); assert(AllLed == 0);
        command(source, 0x44); assert(AllLed == 1);
        command(source, 0x46); assert(BreatheLed == 1);
        command(source, 0x47); assert(BreatheLed == 0);
        assert(Action_Mode == 4 && Sustainedmove == !source);
        assert(Action_RequestId == request);
        for (i = 0; i < 256; i++) {
            if (i == 0x29 || i == 0x50 || (i >= 0x30 && i <= 0x39) || (i >= 0x40 && i <= 0x49)) { continue; }
            command(source, (uint8_t)i);
            assert(Action_Mode == 4 && Sustainedmove == !source);
            assert(Action_RequestId == request && Face_Mode == 4);
        }
        Runs++;
    }
}

static void test_receiver_recovery(void)
{
    unsigned source, i;
    static const uint32_t errors[] = {USART_SR_PE,USART_SR_FE,USART_SR_NE,USART_SR_ORE};
    for (source = 0; source < 2; source++) {
        reset();
        NextByte = 0x32;
        send(source, 0x30);
        assert(Reads[source] == 1);
        assert((source ? USART3 : USART1)->SR == USART_SR_RXNE);
        BlueTooth_Process();
        assert(Action_Mode == 1);
        InIRQ++;
        if (source) { USART3_IRQHandler(); } else { USART1_IRQHandler(); }
        InIRQ--;
        BlueTooth_Process();
        assert(Action_Mode == 3 && Reads[source] == 2);
        for (i = 0; i < sizeof errors / sizeof errors[0]; i++) {
            send_status(source, 0x33, USART_SR_RXNE | errors[i]);
            BlueTooth_Process();
            assert(Action_Mode == 3);
        }
        send_status(source, 0, USART_SR_ORE); /* Error without RXNE still clears DR. */
        assert((source ? BlueTooth_RxErrors : Voice_RxErrors) == 5);
        assert((source ? Voice_RxErrors : BlueTooth_RxErrors) == 0);
        send_status(source, 0, 0);
        assert(Reads[source] == 7);
        command(source, 0x31);
        assert(Action_Mode == 2);
        Runs++;
    }
}

static void test_queue_and_nested_receive(void)
{
    unsigned i;
    reset();
    for (i = 0; i < 62; i++) { send(i & 1, 0); }
    send(0, 0x30);
    send(1, 0x33); /* 64th event is dropped, not an overwrite. */
    assert(Serial_RxOverflow == 1);
    BlueTooth_Process();
    assert(Action_Mode == 1 && Sustainedmove == 0);
    for (i = 0; i < 200; i++) {
        command(i & 1, 0x31);
        assert(Action_Mode == 2 && Sustainedmove == (i & 1));
    }
    reset();
    PreemptVoice = 1;
    send(1, 0x33);
    BlueTooth_Process();
    assert(Reads[0] == 1 && Reads[1] == 1);
    assert(Action_Mode == 4 && Sustainedmove == 1 && Action_RequestId == 2);
    assert(Primask == 0);
    Primask = 1;
    send(0, 0x30);
    assert(Primask == 1);
    Primask = 0;
    BlueTooth_Process();
    assert(Action_Mode == 1);
    Runs++;
}

static void schedule(uint32_t tick, unsigned source, uint8_t data)
{
    Trigger = tick;
    TriggerSource = (uint8_t)source;
    TriggerCommand = data;
}

static void test_tail_toggle(void)
{
    unsigned source;
    for (source = 0; source < 2; source++) {
        reset();
        command(source, 0x40);
        assert(WeiBa == 1 && Action_Mode == 9);
        schedule(70, source, 0x40);
        Action_SwingTail();
        assert(Ticks == 70 && WeiBa == 0 && Action_Mode == 2);
        assert(TailAngle == 90 && Writes == WritesAtTrigger + 1);
        command(source, 0x40);
        command(source, 0x33);
        command(source, 0x40);
        assert(WeiBa == 0 && Action_Mode == 4 && TailAngle == 90);
        Runs++;
    }
}

typedef void (*Action)(void);
static const struct { uint8_t mode, command; Action run; } Actions[] = {
    {0,0x29,Action_relaxed_getdowm}, {1,0x30,Action_sit},
    {2,0x31,Action_upright}, {3,0x32,Action_getdowm},
    {4,0x33,Action_advance}, {5,0x34,Action_back},
    {6,0x35,Action_Lrotation}, {7,0x36,Action_Rrotation},
    {8,0x37,Action_Swing}, {9,0x40,Action_SwingTail},
    {10,0x41,Action_JumpU}, {11,0x42,Action_JumpD},
    {12,0x31,Action_upright2}, {13,0x43,Action_Hello},
    {14,0x48,Action_stretch}, {15,0x49,Action_Lstretch},
    {ACTION_MODE_AFFECTION,0x50,Action_Affection}
};
static void test_action_cancellation(void)
{
    /* Sample both sides of old delay/phase boundaries, including late stretch phases. */
    static const uint32_t ticks[] = {1,6,15,59,60,79,80,81,199,200,201,399,400,479,480,800,1600,3200,4800};
    unsigned a, t, source;
    for (source = 0; source < 2; source++) {
        for (a = 0; a < sizeof Actions / sizeof Actions[0]; a++) {
            for (t = 0; t < sizeof ticks / sizeof ticks[0]; t++) {
                reset();
                command(source, Actions[a].command);
                Action_Mode = Actions[a].mode; /* mode 12 is an internal stand variant. */
                schedule(ticks[t], !source, Actions[a].mode == 0 ? 0x30 : 0x29);
                Actions[a].run();
                if (Ticks >= Trigger) {
                    assert(Ticks == Trigger);
                    assert(Action_Mode == (Actions[a].mode == 0 ? 1 : 0));
                    assert(Action_RequestId == RequestAtTrigger + 1);
                    /* Affection cancellation centers the tail in the new command handler. */
                    assert(Writes == WritesAtTrigger + (Actions[a].mode == ACTION_MODE_AFFECTION));
                }
                Runs++;
            }
        }
    }
    /* A repeated jump command must survive, too, rather than become stand. */
    for (a = 10; a <= 11; a++) {
        reset();
        command(0, Actions[a].command);
        schedule(1, 1, Actions[a].command);
        Actions[a].run();
        assert(Ticks == 1 && Action_Mode == a && Sustainedmove == 1);
        assert(Action_RequestId == 2 && Writes == WritesAtTrigger);
        Runs++;
    }
    for (a = 0; a < sizeof Actions / sizeof Actions[0]; a++) {
        if (Actions[a].mode == 9 || Actions[a].mode == 12) { continue; }
        reset();
        command(0, Actions[a].command);
        schedule(1, 1, Actions[a].command);
        Actions[a].run();
        assert(Ticks == 1 && Action_Mode == Actions[a].mode);
        assert(Action_RequestId == 2 && Sustainedmove == 1);
        assert(Writes == WritesAtTrigger);
        Runs++;
    }
}

static void test_normal_completion_and_counters(void)
{
    unsigned a;
    for (a = 4; a <= 7; a++) {
        reset();
        command(0, Actions[a].command);
        Actions[a].run();
        assert(Ticks == 3200 && Writes == 32 && PAnumbers == 0 && Action_Mode == 2);
        reset();
        command(1, Actions[a].command);
        schedule(5000, 0, 0x29);
        Actions[a].run();
        assert(Ticks == 5000 && PAnumbers == 0 && Action_Mode == 0);
        Runs += 2;
    }
    reset(); command(0, 0x41); Action_JumpU();
    assert(Ticks == 480 && Writes == 4 && Action_Mode == 2 && TiaoTurn == 1);
    command(0, 0x41); Action_JumpU();
    assert(Ticks == 960 && Writes == 8 && TiaoTurn == 0);
    reset(); command(0, 0x42); Action_JumpD();
    assert(Ticks == 400 && Writes == 4 && Action_Mode == 12 && TiaoTurn2 == 1);
    command(0, 0x42); Action_JumpD();
    assert(Ticks == 800 && Writes == 8 && TiaoTurn2 == 0);
    reset(); command(0, 0x48); Action_stretch();
    assert(Ticks == 4880 && Writes == 642 && Action_Mode == 15);
    Action_Lstretch();
    assert(Action_Mode == 2);
    Runs += 3;
}

static void test_lights_remain_operable(void)
{
    reset();
    command(0, 0x45); TIM3_IRQHandler(); assert(Led1 == 0 && Led2 == 0);
    command(1, 0x44); TIM3_IRQHandler(); assert(Led1 == 20000 && Led2 == 20000);
    command(0, 0x46); TIM3_IRQHandler(); assert(Led1 == 100 && Led2 == 100);
    command(1, 0x47); TIM3_IRQHandler(); assert(Led1 == 20000 && Led2 == 20000);
    Runs++;
}

static void test_firmware_main_dispatch(void)
{
    reset();
    schedule(10, 1, 0x30);
    StopFirmware = 200;
    if (setjmp(FirmwareExit) == 0) {
        pet_firmware_main();
        assert(0); /* Firmware normally runs forever. */
    }
    assert(Action_Mode == 1 && Sustainedmove == 1);
    assert(LastImage == Face_stare && Action_RequestId == 1);
    assert(LegAngles[0] == 90 && LegAngles[1] == 90);
    assert(LegAngles[2] == 20 && LegAngles[3] == 20);
    StopFirmware = 0;
    Runs++;
}

static float distance(float a, float b)
{
    float d = a - b;
    return d < 0 ? -d : d;
}

static float angle_at(unsigned channel, uint32_t tick)
{
    float angle = 90;
    unsigned i;
    for (i = 0; i < TraceCount; i++) {
        if (Trace[i].channel == channel && Trace[i].tick <= tick) { angle = Trace[i].angle; }
    }
    return angle;
}

static void check_smooth_trace(const float initial[5])
{
    float previous[5];
    unsigned i, channel;
    memcpy(previous, initial, sizeof previous);
    for (i = 0; i < TraceCount; i++) {
        float low, high;
        channel = Trace[i].channel;
        low = channel == 4 ? 75 : 65;
        high = channel == 4 ? 105 : 90;
        if (initial[channel] < low) { low = initial[channel]; }
        if (initial[channel] > high) { high = initial[channel]; }
        assert(Trace[i].angle >= low && Trace[i].angle <= high);
        assert(distance(Trace[i].angle, previous[channel]) <= 5.001f);
        previous[channel] = Trace[i].angle;
    }
}

static void test_commanded_angles_and_pwm(void)
{
    reset();
    Servo_Angle1(45); Servo_Angle2(45); Servo_Angle3(135); Servo_Angle4(135);
    WServo_Angle(90);
    assert(Servo_GetAngle(1) == 45 && Servo_GetAngle(2) == 45);
    assert(Servo_GetAngle(3) == 135 && Servo_GetAngle(4) == 135);
    assert(Pulses[0] == 1000 && Pulses[1] == 2000);
    assert(Pulses[2] == 2000 && Pulses[3] == 1000 && Pulses[4] == 1500);
    assert(WServo_GetAngle() == 90);
    Runs++;
}

static void test_affection_completion(void)
{
    unsigned source, enabled, pose, i;
    for (source = 0; source < 2; source++) {
        for (enabled = 0; enabled < 2; enabled++) {
            for (pose = 0; pose < 2; pose++) {
                float initial[5] = {90,90,90,90,90};
                unsigned peaks = 0;
                reset();
                WeiBa = (uint8_t)enabled;
                if (pose) {
                    Servo_Angle1(20); Servo_Angle2(20);
                    Servo_Angle3(160); Servo_Angle4(160); WServo_Angle(30);
                    initial[0] = initial[1] = 20;
                    initial[2] = initial[3] = 160;
                    initial[4] = 30;
                }
                RecordTrace = 1;
                command(source, 0x50);
                Action_Affection();
                assert(Ticks == 4200 && TraceCount == 620);
                assert(Action_Mode == 2 && Action_RequestId == 1 && WeiBa == enabled);
                assert(Frames == 2 && Face_Mode == 2 && LastImage == Face_happy);
                assert(TailAngle == 90);
                for (i = 0; i < 4; i++) {
                    assert(LegAngles[i] == 90);
                    assert(angle_at(i, 399) == 70 && angle_at(i, 2399) == 70);
                    assert(angle_at(i, 1099) == (i % 2 ? 85 : 65));
                    assert(angle_at(i, 1849) == (i % 2 ? 65 : 85));
                }
                for (i = 0; i < TraceCount; i++) {
                    if (Trace[i].channel == 4 && Trace[i].angle == 105) { peaks++; }
                }
                assert(peaks == 3);
                assert(angle_at(4, 2699) == 105 && angle_at(4, 2899) == 75);
                assert(angle_at(4, 3099) == 105 && angle_at(4, 3299) == 75);
                assert(angle_at(4, 3499) == 105 && angle_at(4, 3699) == 75);
                check_smooth_trace(initial);
                RecordTrace = 0;
                Action_upright();
                assert(Action_Mode == (enabled ? 9 : 2));
                Runs++;
            }
        }
    }
}

static void test_affection_takeover(void)
{
    static const uint32_t boundaries[] = {
        1,20,399,400,699,700,1099,1100,1249,1250,1849,1850,
        1999,2000,2399,2400,2499,2500,2699,2700,2899,2900,
        3699,3700,3799,3800,4199,4200
    };
    static const uint8_t commands[] = {0x29,0x31,0x41,0x50,0x40};
    unsigned source, enabled, c, t;
    for (source = 0; source < 2; source++) {
        for (enabled = 0; enabled < 2; enabled++) {
            for (c = 0; c < sizeof commands / sizeof commands[0]; c++) {
                for (t = 0; t < sizeof boundaries / sizeof boundaries[0]; t++) {
                    uint16_t expected_mode;
                    reset();
                    WeiBa = (uint8_t)enabled;
                    command(source, 0x50);
                    schedule(boundaries[t], !source, commands[c]);
                    Action_Affection();
                    expected_mode = commands[c] == 0x29 ? 0 :
                        commands[c] == 0x31 ? 2 : commands[c] == 0x41 ? 10 :
                        commands[c] == 0x50 ? ACTION_MODE_AFFECTION : enabled ? 2 : 9;
                    assert(Ticks == Trigger && Action_RequestId == 2);
                    assert(Action_Mode == expected_mode && Sustainedmove == !source);
                    assert(WeiBa == (commands[c] == 0x40 ? !enabled : enabled));
                    assert(Writes == WritesAtTrigger + (commands[c] != 0x50));
                    assert(Face_Mode == (commands[c] == 0x29 ? 0 : commands[c] == 0x31 ? 5 :
                        commands[c] == 0x41 ? 2 : 1));
                    if (commands[c] != 0x50) { assert(TailAngle == 90); }
                    else {
                        float initial[5];
                        unsigned leg;
                        for (leg = 0; leg < 4; leg++) { initial[leg] = Servo_GetAngle((uint8_t)(leg+1)); }
                        initial[4] = WServo_GetAngle();
                        Ticks = Trigger = 0;
                        RecordTrace = 1;
                        Action_Affection();
                        assert(Ticks == 4200 && Action_Mode == 2 && Action_RequestId == 2);
                        assert(WeiBa == enabled);
                        check_smooth_trace(initial);
                    }
                    Runs++;
                }
            }
        }
    }
}

static void test_affection_ancillary_and_dispatch(void)
{
    static const uint8_t commands[] = {0x38,0x39,0x44,0x45,0x46,0x47,0xFF};
    unsigned i, enabled;
    for (enabled = 0; enabled < 2; enabled++) {
        for (i = 0; i < sizeof commands / sizeof commands[0]; i++) {
            reset();
            WeiBa = (uint8_t)enabled;
            command(1, 0x50);
            schedule(2700, 0, commands[i]);
            Action_Affection();
            assert(Ticks == 4200 && Action_Mode == 2 && Action_RequestId == 1);
            assert(WeiBa == enabled && Sustainedmove == 1 && TailAngle == 90);
            Runs++;
        }
    }
    reset();
    SpeedDelay = 120;
    command(1, 0x50);
    schedule(4199, 0, 0x38); /* Speed-limit face must not leak into the final happy pose. */
    Action_Affection();
    BlueTooth_Process();
    assert(Ticks == 4200 && Face_Mode == 2 && LastImage == Face_happy);
    Runs++;
    reset();
    schedule(10, 1, 0x50);
    StopFirmware = 4300;
    if (setjmp(FirmwareExit) == 0) { pet_firmware_main(); assert(0); }
    assert(Action_Mode == 2 && WeiBa == 0 && Action_RequestId == 1);
    assert(LastImage == Face_happy && TailAngle == 90);
    assert(LegAngles[0] == 90 && LegAngles[1] == 90);
    assert(LegAngles[2] == 90 && LegAngles[3] == 90);
    StopFirmware = 0;
    Runs++;
}

int main(void)
{
    test_action_protocol();
    test_ancillary_and_invalid_commands();
    test_receiver_recovery();
    test_queue_and_nested_receive();
    test_tail_toggle();
    test_action_cancellation();
    test_normal_completion_and_counters();
    test_lights_remain_operable();
    test_firmware_main_dispatch();
    test_commanded_angles_and_pwm();
    test_affection_completion();
    test_affection_takeover();
    test_affection_ancillary_and_dispatch();
    printf("PASS: %u regression scenarios (production command/action/face/timer code)\n", Runs);
    return 0;
}
