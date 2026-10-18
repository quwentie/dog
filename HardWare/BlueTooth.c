#include "stm32f10x.h"
#include "BlueTooth.h"
#include "Servo.h"
#include "PetAction.h"
#include "Face_Config.h"

volatile uint16_t AllLed = 1;
volatile uint16_t BreatheLed = 0;
volatile uint16_t Sustainedmove = 0;
volatile uint16_t Action_Mode = 0;
volatile uint16_t SpeedDelay = 200;
volatile uint16_t SwingDelay = 6;
volatile uint16_t Face_Mode = 0;
volatile uint8_t WeiBa = 0;
volatile uint32_t Action_RequestId = 0;
volatile uint32_t Serial_RxOverflow = 0;
volatile uint32_t Voice_RxErrors = 0;
volatile uint32_t BlueTooth_RxErrors = 0;

/* Two interrupt producers, one foreground consumer. Capacity: 63 events. */
#define RX_QUEUE_SIZE 64
#define RX_QUEUE_MASK (RX_QUEUE_SIZE - 1)
typedef struct {
    uint8_t data;
    uint8_t continuous;
} RxEvent;
static volatile RxEvent RxQueue[RX_QUEUE_SIZE];
static volatile uint8_t RxHead;
static volatile uint8_t RxTail;
static uint16_t DisplayedFace = 0xFFFF;
static uint8_t TailCenterPending;

void BlueTooth_Init(void)
{
	RxHead = RxTail = 0;
	Serial_RxOverflow = Voice_RxErrors = BlueTooth_RxErrors = 0;
	DisplayedFace = 0xFFFF;
	TailCenterPending = 0;
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA,ENABLE);//开启GPIOA时钟
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1,ENABLE);//开启串口时钟
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB,ENABLE);//开启GPIOB时钟
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3,ENABLE);//开启串口时钟
	//语音
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode=GPIO_Mode_AF_PP;//复用推挽输出模式
	GPIO_InitStructure.GPIO_Pin=GPIO_Pin_9;//默认PA9是USART1_TX的复用
	GPIO_InitStructure.GPIO_Speed=GPIO_Speed_50MHz;
	GPIO_Init(GPIOA,&GPIO_InitStructure);
	GPIO_InitStructure.GPIO_Mode=GPIO_Mode_IPU;//复用上拉输入模式
	GPIO_InitStructure.GPIO_Pin=GPIO_Pin_10;//默认PA9是USART1_RX的复用
	GPIO_InitStructure.GPIO_Speed=GPIO_Speed_50MHz;
	GPIO_Init(GPIOA,&GPIO_InitStructure);
	//蓝牙
	GPIO_InitStructure.GPIO_Mode=GPIO_Mode_AF_PP;//复用推挽输出模式
	GPIO_InitStructure.GPIO_Pin=GPIO_Pin_10;//默认PB10是USART3_TX的复用
	GPIO_InitStructure.GPIO_Speed=GPIO_Speed_50MHz;
	GPIO_Init(GPIOB,&GPIO_InitStructure);
	GPIO_InitStructure.GPIO_Mode=GPIO_Mode_IPU;//复用上拉输入模式
	GPIO_InitStructure.GPIO_Pin=GPIO_Pin_11;//默认PB11是USART3_RX的复用
	GPIO_InitStructure.GPIO_Speed=GPIO_Speed_50MHz;
	GPIO_Init(GPIOB,&GPIO_InitStructure);

	USART_InitTypeDef UASRT_InitStructure;//USART初始化
	UASRT_InitStructure.USART_BaudRate=9600;//波特率9600
	UASRT_InitStructure.USART_HardwareFlowControl=USART_HardwareFlowControl_None;//不需要硬件流控制
	UASRT_InitStructure.USART_Mode=USART_Mode_Rx|USART_Mode_Tx;//接受与发送均打开
	UASRT_InitStructure.USART_Parity=USART_Parity_No;//不需要奇偶校验
	UASRT_InitStructure.USART_StopBits=USART_StopBits_1;//停止位为1
	UASRT_InitStructure.USART_WordLength=USART_WordLength_8b;//字长8位
	USART_Init(USART1,&UASRT_InitStructure);
	USART_Init(USART3,&UASRT_InitStructure);

	USART_ITConfig(USART1,USART_IT_RXNE,ENABLE);//语音接收中断配置，也就是如果接送到消息就直接中断
	USART_ITConfig(USART3,USART_IT_RXNE,ENABLE);//蓝牙接收中断配置，也就是如果接送到消息就直接中断

	//中断
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);//分组2
	//语音中断配置
	NVIC_InitTypeDef NVIC_InitStructure;
	NVIC_InitStructure.NVIC_IRQChannel=USART1_IRQn;//特定的通道
	NVIC_InitStructure.NVIC_IRQChannelCmd=ENABLE;//通道使能
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=1;//抢占式优先级为1
	NVIC_InitStructure.NVIC_IRQChannelSubPriority=1;//响应优先级为1
	NVIC_Init(&NVIC_InitStructure);
	//蓝牙中断配置
	NVIC_InitStructure.NVIC_IRQChannel=USART3_IRQn;//特定的通道
	NVIC_InitStructure.NVIC_IRQChannelCmd=ENABLE;//通道使能
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=2;//抢占式优先级为2
	NVIC_InitStructure.NVIC_IRQChannelSubPriority=1;//响应优先级为1
	NVIC_Init(&NVIC_InitStructure);

	USART_Cmd(USART1,ENABLE);//USART1使能打开
	USART_Cmd(USART3,ENABLE);//USART3使能打开
}

/* Preserve PRIMASK: USART1 can preempt USART3 while enqueueing. */
static void ReceiveEvent(uint8_t data, uint8_t continuous)
{
    uint32_t mask = __get_PRIMASK();
    uint8_t next;
    __disable_irq();
    next = (uint8_t)((RxHead + 1) & RX_QUEUE_MASK);
    if (next == RxTail) {
        Serial_RxOverflow++;
    } else {
        RxQueue[RxHead].data = data;
        RxQueue[RxHead].continuous = continuous;
        RxHead = next;
    }
    __set_PRIMASK(mask);
}

static void ReceiveInterrupt(USART_TypeDef *port, uint8_t continuous)
{
    /* SR then one DR read clears RXNE and receive errors on STM32F1. */
    uint32_t status = port->SR;
    uint16_t data;
    const uint32_t errors = USART_SR_ORE | USART_SR_FE | USART_SR_NE | USART_SR_PE;
    if ((status & (USART_SR_RXNE | errors)) == 0) {
        return;
    }
    data = USART_ReceiveData(port);
    if ((status & errors) != 0) {
        if (continuous) {
            BlueTooth_RxErrors++;
        } else {
            Voice_RxErrors++;
        }
        return; /* Discard ambiguous/error-marked data; next byte can recover. */
    }
    if ((status & USART_SR_RXNE) != 0) {
        ReceiveEvent((uint8_t)data, continuous);
    }
}

void USART1_IRQHandler(void)
{
    ReceiveInterrupt(USART1, 0);
}

void USART3_IRQHandler(void)
{
    ReceiveInterrupt(USART3, 1);
}

static void SelectAction(uint16_t mode, uint16_t face, uint8_t continuous)
{
    if (Action_Mode == ACTION_MODE_AFFECTION && mode != ACTION_MODE_AFFECTION) {
        TailCenterPending = 1;
    }
    Sustainedmove = continuous;
    Face_Mode = face;
    Action_Mode = mode;
    /* Same-mode requests also cancel and restart the old action. */
    Action_RequestId++;
}

/* Foreground only: ancillary/unknown bytes must not change movement policy. */
static void ProcessCommand(uint8_t data, uint8_t continuous)
{
    switch (data) {
    case 0x29: SelectAction(0, 0, continuous); break;
    case 0x30: SelectAction(1, 1, continuous); break;
    case 0x31: SelectAction(2, 5, continuous); break;
    case 0x32: SelectAction(3, 1, continuous); break;
    case 0x33: SelectAction(4, 2, continuous); break;
    case 0x34: SelectAction(5, 2, continuous); break;
    case 0x35: SelectAction(6, 2, continuous); break;
    case 0x36: SelectAction(7, 2, continuous); break;
    case 0x37: SelectAction(8, 4, continuous); break;
    case 0x38:
        if (SpeedDelay == 120) { Face_Mode = 3; }
        if (SpeedDelay > 100) { SpeedDelay -= 20; }
        else { SpeedDelay = 200; Face_Mode = 2; }
        break;
    case 0x39:
        if (SwingDelay == 4) { Face_Mode = 3; }
        if (SwingDelay > 3) { SwingDelay--; }
        else { SwingDelay = 9; Face_Mode = 4; }
        break;
    case 0x40:
        WeiBa = (uint8_t)!WeiBa;
        if (WeiBa) {
            SelectAction(9, 1, continuous);
        } else {
            TailCenterPending = 1;
            Face_Mode = 1;
            if (Action_Mode == 9 || Action_Mode == ACTION_MODE_AFFECTION) {
                SelectAction(2, 1, continuous);
            }
        }
        break;
    case 0x41: SelectAction(10, 2, continuous); break;
    case 0x42: SelectAction(11, 2, continuous); break;
    case 0x43: SelectAction(13, 6, continuous); break;
    case 0x44: AllLed = 1; break;
    case 0x45: AllLed = 0; break;
    case 0x46: BreatheLed = 1; break;
    case 0x47: BreatheLed = 0; break;
    case 0x48: SelectAction(14, 6, continuous); break;
    case 0x49: SelectAction(15, 6, continuous); break;
    case 0x50: SelectAction(ACTION_MODE_AFFECTION, 1, continuous); break;
    default: break;
    }
}

void BlueTooth_Process(void)
{
    /* Bound each batch so a continuous stream cannot starve the actions. */
    uint8_t end = RxHead;
    while (RxTail != end) {
        uint8_t data = RxQueue[RxTail].data;
        uint8_t continuous = RxQueue[RxTail].continuous;
        RxTail = (uint8_t)((RxTail + 1) & RX_QUEUE_MASK);
        ProcessCommand(data, continuous);
    }
    if (TailCenterPending) {
        TailCenterPending = 0;
        WServo_Angle(90);
    }
    /* All OLED access stays in foreground, including during action waits. */
    if (DisplayedFace != Face_Mode) {
        Face_Config();
        DisplayedFace = Face_Mode;
    }
}
