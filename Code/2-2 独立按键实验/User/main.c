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

void delay(uint32_t count)
{
	while (count--);
}

// K1按键实现LED1的亮与灭
void solve1()
{	
	GPIO_InitTypeDef GPIO_InitStructure;		//GPIO初始化结构体
	
	// LED灯初始化									
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;	    // LED灯引脚
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;									// 推挽输出
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;	//GPIO输出速度
	GPIO_Init(GPIOB, &GPIO_InitStructure);												// GPIO初始化
	
	// K1按键初始化
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	
	while (1)
	{
		// K1按键 - PA12
		if (GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_12))	//PA12为高电平，说明没有按下去
			GPIO_SetBits(GPIOB, GPIO_Pin_0);
		else
			GPIO_ResetBits(GPIOB, GPIO_Pin_0);
	}
}

// K1按键循环实现四个LED灯的亮灭
void solve2()
{
	GPIO_InitTypeDef GPIO_InitStructure;		//GPIO初始化结构体
	
	// LED灯初始化									
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_2 | GPIO_Pin_4 | GPIO_Pin_6;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;									// 推挽输出
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;	//GPIO输出速度
	GPIO_Init(GPIOB, &GPIO_InitStructure);												// GPIO初始化
	
	// K1按键初始化
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	
	int idx = 0;	// 当前的LED灯的下标
	uint16_t pin[4] = { GPIO_Pin_0 , GPIO_Pin_2 , GPIO_Pin_4 , GPIO_Pin_6};
	GPIO_SetBits(GPIOB, GPIO_Pin_0 | GPIO_Pin_2 | GPIO_Pin_4 | GPIO_Pin_6);	// 一开始四个LED灯都不亮
	
	// while循环体内部会高速执行达不到效果
	//while (0)
	//{
	//	if (GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_12) == 0)	//PA12为低电平，说明按下去
	//	{
	//		if (++idx > 4)
	//			idx = 1;
	//	}
	//	if(idx > 0)
	//	{
	//		GPIO_ResetBits(GPIOB, pin[idx - 1]);
	//		GPIO_SetBits(GPIOB, pin[(idx) % 4] | pin[(idx + 1) % 4] | pin[(idx + 2) % 4] ); 
	//	}
	//}
	
	// 改进：低电平持续一段时间（延时消抖），才认为是真正按下操作
	while (1)
	{
		if (GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_12) == 0)	//PA12为低电平，说明按下去
		{
			delay(250000);	// 根据正常按下按钮过程修改
			if (GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_12) == 0)
			{
				if (++idx > 4)
					idx = 1;
			}
		}
		if(idx > 0)
		{
			GPIO_ResetBits(GPIOB, pin[idx - 1]);
			GPIO_SetBits(GPIOB, pin[(idx) % 4] | pin[(idx + 1) % 4] | pin[(idx + 2) % 4] );
		}
	}
}

// 采用边沿响应来实现按键的一次按压操作只执行一次响应
void solve3()
{
	GPIO_InitTypeDef GPIO_InitStructure;		//GPIO初始化结构体
	
	// LED灯初始化									
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_2 | GPIO_Pin_4 | GPIO_Pin_6;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;									// 推挽输出
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;	//GPIO输出速度
	GPIO_Init(GPIOB, &GPIO_InitStructure);												// GPIO初始化
	
	// K1按键初始化
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	
	u8 idx = 0;
	u8 lastLevel = 1;	// 上一次的引脚电平（默认是高电平）
	u8 curLevel = 1;	// 当前的引脚电平
	uint16_t pin[4] = { GPIO_Pin_0 , GPIO_Pin_2 , GPIO_Pin_4 , GPIO_Pin_6};
	GPIO_SetBits(GPIOB, GPIO_Pin_0 | GPIO_Pin_2 | GPIO_Pin_4 | GPIO_Pin_6);
	
	while (1)
	{
		// 低电平检测
		if (GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_12) == 0 )	// 低电平，对应按钮按下
		{
			delay(5000);
			if (GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_12) == 0 )
				curLevel = 0;
		}
		else	// 高电平检测
		{
			delay(5000);
			if (GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_12) == 1)
				curLevel = 1;
		}
		// 按下检测 - 下降沿检测
		if (lastLevel == 1 && curLevel == 0 )
		{
			if (++idx > 4)
				idx = 1;
		}
		lastLevel = curLevel;
		if (idx > 0)
		{
			GPIO_ResetBits(GPIOB, pin[idx - 1] );
			GPIO_SetBits(GPIOB, pin[(idx) % 4] | pin[(idx + 1) % 4] | pin[(idx + 2) % 4] );
		}
	}
	
}
int main(void)
{
	//-------------标准库初始化-------------
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA|RCC_APB2Periph_GPIOB|RCC_APB2Periph_AFIO, ENABLE);			// GPIOA时钟，GPIOB时钟，复用时钟
	GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable, ENABLE);							// 打开SW下载调试复用功能/
	
	//solve1();
	//solve2();
	solve3();
}


