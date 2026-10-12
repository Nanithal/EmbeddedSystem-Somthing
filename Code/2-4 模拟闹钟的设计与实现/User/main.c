#include "stm32f10x.h"


/***********所用板子的相关引脚*****************

						LED灯对应引脚
						LED1----PB0
						LED2----PB1
						...........
						LED8----PB7
						
						独立按键对应引脚
						K1-----PA12
						K2-----PA13
						...........
						K4-----pA15
						
						八段数码管对应引脚
						a----PC8
						b----PC9
						........
						dp---PC15
						
						位选对应引脚
						A---PB12
						B---PB13
						C---OPB14

*****************************************/


// 所要显示的字符对应的段码（共阴极）
const uint16_t seg[10] = {0x3f00, 0x0600, 0x5b00, 0x4f00, 0x6600, 0x6d00, 0x7d00, 0x0700, 0x7f00, 0x6f00};

void delay(uint16_t count)
{
	while (count--);
}

void Show_LED()
{
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Pin = (u8)(0x0f);
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB,&GPIO_InitStructure);
	
	GPIO_SetBits(GPIOB, (uint16_t)(0x00ff));
	int idx = 1;
	while (1)
	{
		for (u8 count = 0; count < 4; count++)
		{
			GPIO_SetBits(GPIOB,(uint16_t)(0x00ff));
			GPIO_ResetBits(GPIOB, (uint16_t)(idx << count));
			delay(5000);
		}
		delay((uint16_t)5000000);
	}
}



void Show_SetTime(uint16_t time)
{
	
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8 | GPIO_Pin_9 | GPIO_Pin_10 | GPIO_Pin_11 | GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	
	GPIO_SetBits(GPIOB, (uint16_t)(0xff00));
	
	u8 idx = 7;
	GPIO_ResetBits(GPIOC, (uint16_t)(7<<12));
	while(time)
	{
		uint16_t show_time = time % 10;
		delay(5000);
		
		GPIO_ResetBits(GPIOB, seg[show_time]);
		GPIO_ResetBits(GPIOC, (uint16_t)(idx << 12));
	}
	
}

void Clock_Num_Set()
{
	uint16_t time = 0;
	
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12 | GPIO_Pin_14 | GPIO_Pin_15;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	
	GPIO_SetBits(GPIOA, GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15);
	
	if (GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_12) == 1)
	{
		while (GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_13) == 0)
		{
			if (GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_14) == 0)
				time++;
		  if (GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_15) == 0)
				time--;
		}
	}
	if (GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_13) == 0)
	{
		Show_SetTime(time);
	}
}
int main(void)
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA|RCC_APB2Periph_GPIOB|RCC_APB2Periph_GPIOC|RCC_APB2Periph_AFIO, ENABLE);
	GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable, ENABLE);
	
}
