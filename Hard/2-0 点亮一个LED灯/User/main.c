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

// 延时函数
void delay(uint32_t count) {
    while(count--);
}

void solve1()
{
	// 配置PB引脚结构体
	GPIO_InitTypeDef GPIO_InitStructure;												//GPIO初始化结构体
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;	    //LED灯对应GPIO引脚 
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;									// 推挽输出模式
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;									//GPIO速度设置
	GPIO_Init(GPIOB, &GPIO_InitStructure);												//GPIO初始化
	
	while (1)
	{
				// 点亮 LED (输出低电平)
        GPIO_ResetBits(GPIOB, GPIO_Pin_0);
        delay(500000);
        
        // 熄灭 LED (输出高电平)
        GPIO_SetBits(GPIOB, GPIO_Pin_0);
        delay(500000);
	}
}


int main(void)
{
	// 标准库初始化
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB|RCC_APB2Periph_AFIO, ENABLE);			//GPIOB时钟，复用时钟
	GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable, ENABLE);							//打开SW下载调试复用功能
	
	solve1();
}
