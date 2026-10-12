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

#include "stm32f10x.h"                  // Device header

/**********************************************************

				一个数码管的八段对应引脚
				且a为字节的最低位，dp为字节的最高位
				a---PC8
				b---PC9
				......
				dp---PC15
				
				位选对应的引脚
				A----PB12
				B----PB13
				C----PB14
**********************************************************/

// 所要显示的字符对应的段码（共阴极）
const uint16_t seg[10] = {0x3f00, 0x0600, 0x5b00, 0x4f00, 0x6600, 0x6d00, 0x7d00, 0x0700, 0x7f00, 0x6f00};


// 简单的延迟函数
void delay(uint32_t count)
{
	while (count--);
}

// 单个数码管的动态显示
void solve1()
{
	GPIO_InitTypeDef GPIO_InitStructure;												//GPIO初始化结构体
	
	// 段选引脚初始化
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8 | GPIO_Pin_9 | GPIO_Pin_10 | GPIO_Pin_11 | GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15;	    // 段选信号引脚
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;									// 推挽输出
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;	// 输出速度
	GPIO_Init(GPIOC, &GPIO_InitStructure);												// GPIO初始化
	
	// 位选引脚初始化
	GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_14;	    // 位选信号引脚
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	
	int count = 0;	// 当前显示的数字
	
	while (1)
	{
		GPIO_ResetBits(GPIOC, (uint16_t)(0xff00));	// 将GPIOC高八位清零，熄灭八段LED灯
		GPIO_ResetBits(GPIOB, (uint16_t)(0x7000));	// 位选三个引脚置为0，即选中第一个数码显示管
		GPIO_SetBits(GPIOC, (uint16_t)(seg[count]));	// 输出所要显示的数字的段码
		delay(500000);
	  count = (count + 1) % 10;
	}
	
}

// 多个数码管的相同显示，在八个数码管上依次（同时）显示相同的数字（每过一轮，显示的数字发生变化）
void solve2()
{
	GPIO_InitTypeDef GPIO_InitStructure;												//GPIO初始化结构体
	
	// 段选引脚初始化
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8 | GPIO_Pin_9 | GPIO_Pin_10 | GPIO_Pin_11 | GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15;	    // 段选信号引脚
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;									// 推挽输出
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;	// 输出速度
	GPIO_Init(GPIOC, &GPIO_InitStructure);												// GPIO初始化
	
	// 位选引脚初始化
	GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_14;	    // 位选信号引脚
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	
	u8 idx = 0;	// 当前显示数字的数码管编号
	u8 num = 0;	// 当前数码管显示的数字
	
	while (1)
	{
		
		// 去除鬼影效果
		GPIO_ResetBits(GPIOC, (uint16_t)(0xff00));
		GPIO_ResetBits(GPIOB, (uint16_t)(0x7000));
		
		GPIO_SetBits(GPIOB, (uint16_t)(idx) << 12);
		GPIO_SetBits(GPIOC, (uint16_t)(seg[num]));
		delay(200000);
		idx = (idx + 1) % 8;
		if (idx == 0)
			num = (num + 1) % 10;
	}
	
}

// 多个数码管的不同动态显示，8个数码管依次（同时）显示不同的内容（显示的内容一直保持不变）
void solve3()
{
	GPIO_InitTypeDef GPIO_InitStructure;												//GPIO初始化结构体
	
	// 段选引脚初始化
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8 | GPIO_Pin_9 | GPIO_Pin_10 | GPIO_Pin_11 | GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15;	    // 段选信号引脚
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;									// 推挽输出
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;	// 输出速度
	GPIO_Init(GPIOC, &GPIO_InitStructure);												// GPIO初始化
	
	// 位选引脚初始化
	GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_14;	    // 位选信号引脚
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	
	u8 idx = 0;	// 当前显示数字的数码管编号
	u8 num [8] = {0, 1, 2, 3, 4, 5, 6, 7};	// 每个数码管对应的将要显示的数字
	u8 add = 0;
	while (1)
	{
		
		// 去除鬼影效果
		GPIO_ResetBits(GPIOC, (uint16_t)(0xff00));
		GPIO_ResetBits(GPIOB, (uint16_t)(0x7000));
		
		GPIO_SetBits(GPIOB, (uint16_t)(idx) << 12);
		GPIO_SetBits(GPIOC, (uint16_t)(seg[num[idx]]));
		delay(5000);
		idx = (idx + 1) % 8;
	}

}

int main(void)
{
	//-------------标准库初始化-------------
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB|RCC_APB2Periph_GPIOC|RCC_APB2Periph_AFIO, ENABLE);			// GPIOB时钟，GPIOC时钟，复用时钟
	GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable, ENABLE);							// 打开SW下载调试复用功能
	
	// solve1();
	// solve2();
	solve3();

}

