/*
	单片机：STM32F103RCT6/STM32F103C8T6  倍频 72M 8路舵机控制
	舵机IO PA0~PA3 PB3~PB6
*/
#include "stm32f10x_conf.h"
#define tb_interrupt_open() {__enable_irq();}	//总中断打开
void rcc_init(void);							//主频设置
void delay_ms(unsigned int t);		//毫秒级别延时
void dj_io_init(void);				//舵机 IO 口初始化
void dj_io_set(u8 index, u8 level);	//舵机 IO 口高低电平设置
void TIM2_Int_Init(u16 arr,u16 psc);//舵机 定时器初始化
void gpioA_pin_set(unsigned char pin, unsigned char level);
void gpioB_pin_set(unsigned char pin, unsigned char level);

//舵机脉冲数组
int duoji_pulse[8] = {1500,1500,1500,1500,1500,1500,1500,1500} , i;

int main(void) {	
	rcc_init();
	dj_io_init();
	TIM2_Int_Init(20000,71);
	tb_interrupt_open();
	while(1) {	
		for(i=0;i<8;i++) {
			duoji_pulse[i] = 1000;//循环把8个舵机位置设定到1000
		}
		delay_ms(1000);
		for(i=0;i<8;i++) {
			duoji_pulse[i] = 2000;//循环把8个舵机位置设定到2000
		}
		delay_ms(1000);	
	}
}

void rcc_init(void) {
	ErrorStatus HSEStartUpStatus;
	RCC_DeInit();
	RCC_HSEConfig(RCC_HSE_ON); 
	HSEStartUpStatus = RCC_WaitForHSEStartUp();
	while(HSEStartUpStatus == ERROR);
	RCC_HCLKConfig(RCC_SYSCLK_Div1);//SYSCLK
	RCC_PCLK1Config(RCC_HCLK_Div2);//APB1  MAX = 36M
	RCC_PCLK2Config(RCC_HCLK_Div1);//APB2  MAX = 72M
	RCC_PLLConfig(RCC_PLLSource_HSE_Div1, RCC_PLLMul_9);
	RCC_PLLCmd(ENABLE); 
	while(RCC_GetFlagStatus(RCC_FLAG_PLLRDY) == RESET);
	RCC_SYSCLKConfig(RCC_SYSCLKSource_PLLCLK);
	while(RCC_GetSYSCLKSource() != 0x08);
}


void delay_ms(unsigned int t) {
	int t1;
	while(t--) {
		t1 = 7200;
		while(t1--);
	}
}
void dj_io_init(void)  {
	GPIO_InitTypeDef GPIO_InitStructure;	

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA|RCC_APB2Periph_GPIOB, ENABLE);
	
	GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_0|GPIO_Pin_1|GPIO_Pin_2|GPIO_Pin_3;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_3|GPIO_Pin_4|GPIO_Pin_5|GPIO_Pin_6 | GPIO_Pin_14;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOB, &GPIO_InitStructure);		
}

void dj_io_set(u8 index, u8 level) {
	switch(index) {
		case 0:gpioA_pin_set(0, level);break;
		case 1:gpioA_pin_set(1, level);break;
		case 2:gpioA_pin_set(2, level);break;
		case 3:gpioA_pin_set(3, level);break;
		case 4:gpioB_pin_set(3, level);break;
		case 5:gpioB_pin_set(4, level);break;
		case 6:gpioB_pin_set(5, level);break;
		case 7:gpioB_pin_set(6, level);break;
		default:break;
	}
}

void gpioA_pin_set(unsigned char pin, unsigned char level) {
	if(level) {
		GPIO_SetBits(GPIOA,1 << pin);
	} else {
		GPIO_ResetBits(GPIOA,1 << pin);
	}
}

void gpioB_pin_set(unsigned char pin, unsigned char level) {
	if(level) {
		GPIO_SetBits(GPIOB,1 << pin);
	} else {
		GPIO_ResetBits(GPIOB,1 << pin);
	}
}


void TIM2_Int_Init(u16 arr,u16 psc) {
	TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE); //①时钟 TIM2 使能	
	//定时器 TIM2 初始化
	TIM_TimeBaseStructure.TIM_Period = arr; //设置自动重装载寄存器周期的值
	TIM_TimeBaseStructure.TIM_Prescaler =psc; //设置时钟频率除数的预分频值
	TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1; //设置时钟分割
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up; //TIM 向上计数
	TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);  //②初始化 TIM2
	TIM_ARRPreloadConfig(TIM2, DISABLE);
	TIM_ITConfig(TIM2,TIM_IT_Update,ENABLE );  //③允许更新中断
	
	//中断优先级 NVIC 设置
	NVIC_InitStructure.NVIC_IRQChannel = TIM2_IRQn;  //TIM2 中断
	//NVIC_SetVectorTable(NVIC_VectTab_FLASH,0x0000);
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1; //先占优先级 0 级
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2;  //从优先级 2 级
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;  //IRQ 通道被使能
	NVIC_Init(&NVIC_InitStructure);  //④初始化 NVIC 寄存器
	TIM_Cmd(TIM2, ENABLE);  //⑤使能 TIM2
}

void TIM2_IRQHandler(void) {
	static u8 flag = 0;
	static u8 duoji_index1 = 0;
	int temp;
	if (TIM_GetITStatus(TIM2, TIM_IT_Update) != RESET) //检查 TIM2 更新中断发生与否
	{
		TIM_ClearITPendingBit(TIM2, TIM_IT_Update ); //清除 TIM2 更新中断标志
		
		if(duoji_index1 == 8) {
			duoji_index1 = 0;
		}
		
		if(!flag) {
			TIM2->ARR = ((unsigned int)(duoji_pulse[duoji_index1]));
			dj_io_set(duoji_index1, 1);
		} else {
			temp = 2500 - (unsigned int)(duoji_pulse[duoji_index1]);
			if(temp < 20)temp = 20;
			TIM2->ARR = temp;
			dj_io_set(duoji_index1, 0);
			duoji_index1 ++;
		}
		flag = !flag;
	}
} 


