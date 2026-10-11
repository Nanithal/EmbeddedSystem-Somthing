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

// 四个LED灯整体一起亮/灭
void solve1()
{
	GPIO_InitTypeDef GPIO_InitStructure;												//GPIO初始化结构体
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0|GPIO_Pin_2|GPIO_Pin_4|GPIO_Pin_6;	    // 四个流水灯对应引脚
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;									// 推挽输出模式
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;									//GPIO速度
	GPIO_Init(GPIOB, &GPIO_InitStructure);												//GPIO初始化
	
	while (1)
	{
		// LED灯为共阳连接
		
		// 当前LED灯输出低电平，点亮LED灯
		GPIO_ResetBits(GPIOB, GPIO_Pin_0 | GPIO_Pin_2 | GPIO_Pin_4 | GPIO_Pin_6);	
		delay(10000000);
		
		// 当前LED灯输出高电平，熄灭LED灯
		GPIO_SetBits(GPIOB, GPIO_Pin_0 | GPIO_Pin_2 | GPIO_Pin_4 | GPIO_Pin_6);	
		delay(10000000);
	}
}

// 流水灯依次循环点亮
void solve2()
{
	GPIO_InitTypeDef GPIO_InitStructure;												//GPIO初始化结构体
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0|GPIO_Pin_2|GPIO_Pin_4|GPIO_Pin_6;	    // 四个流水灯对应引脚
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;									// 推挽输出模式
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;									//GPIO速度
	GPIO_Init(GPIOB, &GPIO_InitStructure);												//GPIO初始化
	
	uint16_t pin[4] = { GPIO_Pin_0, GPIO_Pin_2, GPIO_Pin_4, GPIO_Pin_6 };	//四个LED灯引脚编号数组
	u8 idx = 0;	//当前需要点亮的LED灯的下标
	while (1)
	{
		// 当前LED灯输出低电平，点亮LED灯
		GPIO_ResetBits(GPIOB, pin[idx]);
		// 其他LED灯输出高电平，熄灭LED灯
		GPIO_SetBits(GPIOB, pin[(idx + 1) % 4] | pin[(idx + 2) % 4] | pin[(idx + 3) % 4]);
		
		idx = (idx + 1) % 4;	//LED灯下标循环
		delay(1000000);
	}
}


int main(void)
{
	//-------------标准库初始化-------------
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB|RCC_APB2Periph_AFIO, ENABLE);			// GPIOB时钟，复用时钟
	GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable, ENABLE);							// 打开SW下载调试复用功能
	
	//solve1();
	solve2();
	
}
