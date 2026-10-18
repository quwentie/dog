#ifndef TEST_STM32F10X_H
#define TEST_STM32F10X_H
#include <stdint.h>

typedef struct { volatile uint32_t SR, DR; } USART_TypeDef;
typedef struct { uint32_t unused; } GPIO_TypeDef;
typedef struct { uint32_t unused; } TIM_TypeDef;
typedef struct { uint32_t GPIO_Mode, GPIO_Pin, GPIO_Speed; } GPIO_InitTypeDef;
typedef struct {
    uint32_t USART_BaudRate, USART_HardwareFlowControl, USART_Mode;
    uint32_t USART_Parity, USART_StopBits, USART_WordLength;
} USART_InitTypeDef;
typedef struct {
    uint32_t NVIC_IRQChannel, NVIC_IRQChannelCmd;
    uint32_t NVIC_IRQChannelPreemptionPriority, NVIC_IRQChannelSubPriority;
} NVIC_InitTypeDef;
extern USART_TypeDef TestUSART1, TestUSART3;
extern GPIO_TypeDef TestGPIOA, TestGPIOB;
extern TIM_TypeDef TestTIM3;
#define USART1 (&TestUSART1)
#define USART3 (&TestUSART3)
#define GPIOA (&TestGPIOA)
#define GPIOB (&TestGPIOB)
#define TIM3 (&TestTIM3)
#define USART_SR_PE (1u << 0)
#define USART_SR_FE (1u << 1)
#define USART_SR_NE (1u << 2)
#define USART_SR_ORE (1u << 3)
#define USART_SR_RXNE (1u << 5)
#define ENABLE 1
#define SET 1
#define GPIO_Mode_AF_PP 1
#define GPIO_Mode_IPU 2
#define GPIO_Speed_50MHz 50
#define GPIO_Pin_9 (1u << 9)
#define GPIO_Pin_10 (1u << 10)
#define GPIO_Pin_11 (1u << 11)
#define RCC_APB2Periph_GPIOA 1
#define RCC_APB2Periph_GPIOB 2
#define RCC_APB2Periph_USART1 4
#define RCC_APB1Periph_USART3 8
#define USART_HardwareFlowControl_None 0
#define USART_Mode_Rx 1
#define USART_Mode_Tx 2
#define USART_Parity_No 0
#define USART_StopBits_1 1
#define USART_WordLength_8b 8
#define USART_IT_RXNE 32
#define NVIC_PriorityGroup_2 2
#define USART1_IRQn 37
#define USART3_IRQn 39
#define TIM_IT_Update 1
uint32_t __get_PRIMASK(void);
void __disable_irq(void);
void __set_PRIMASK(uint32_t value);
uint16_t USART_ReceiveData(USART_TypeDef *port);
void RCC_APB2PeriphClockCmd(uint32_t peripheral, int enabled);
void RCC_APB1PeriphClockCmd(uint32_t peripheral, int enabled);
void GPIO_Init(GPIO_TypeDef *port, GPIO_InitTypeDef *config);
void USART_Init(USART_TypeDef *port, USART_InitTypeDef *config);
void USART_ITConfig(USART_TypeDef *port, uint32_t interrupt, int enabled);
void USART_Cmd(USART_TypeDef *port, int enabled);
void NVIC_PriorityGroupConfig(uint32_t group);
void NVIC_Init(NVIC_InitTypeDef *config);
int TIM_GetITStatus(TIM_TypeDef *timer, uint32_t interrupt);
void TIM_ClearITPendingBit(TIM_TypeDef *timer, uint32_t interrupt);
#endif
