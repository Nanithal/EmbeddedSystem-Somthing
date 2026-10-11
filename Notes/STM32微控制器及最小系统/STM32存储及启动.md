# STM32 存储及启动

> 学习笔记 · STM32 存储器架构、启动流程与固件升级
> 记录时间：2026-02-14

---

## 一、总览：三块存储器 + 五种启动方式

STM32 的存储与启动可以概括为两条主线：

```
① 存储器：程序放哪、数据放哪、参数放哪
② 启动：上电后从哪取第一句指令、向量表在哪、如何跳转
```

```
┌──────────────────────── STM32 存储体系 ────────────────────────┐
│                                                                │
│  片内（芯片自带）                                               │
│  ├─ 主 Flash        程序代码 + 常量（掉电不丢）                 │
│  ├─ 系统存储器       ST 出厂 Bootloader（串口/USB 下载用）      │
│  ├─ 选项字节 OTP     读保护、写保护、复位后自举配置              │
│  ├─ SRAM            运行数据、堆栈、缓存                        │
│  ├─ 备份域 SRAM      VBAT 供电，掉电不丢（RTC 备份寄存器）      │
│  └─ 外设寄存器区     内存映射 I/O                               │
│                                                                │
│  片外（可选扩展）                                               │
│  ├─ FSMC/FMC 接口    NOR/NAND/SRAM/SDRAM                       │
│  └─ SDIO / QSPI      SD 卡、串行 Flash                          │
└────────────────────────────────────────────────────────────────┘
```

> 核心结论：**STM32 采用统一编址的存储器映射，代码、数据、外设都在 4 GB 地址空间内，用普通指针就能访问。**

---

## 二、存储器映射总表

Cortex-M 的 4 GB 空间分为 8 个 512 MB 区域，STM32 在其中填充具体器件：

| 地址区间 | 区域名 | STM32 中的实际内容 |
| --- | --- | --- |
| `0x0000_0000 – 0x0007_FFFF` | **别名区/启动区** | 由 BOOT 配置决定映射到 Flash / 系统存储器 / SRAM |
| `0x0800_0000 – 0x0807_FFFF` | **主 Flash** | 用户程序（F103C8T6 为 64 KB） |
| `0x1FFF_F000 – 0x1FFF_F7FF` | **系统存储器** | ST 出厂 Bootloader（约 2 KB，只读） |
| `0x1FFF_F800 – 0x1FFF_F80F` | **选项字节** | 读/写保护、BOR 等级等 |
| `0x2000_0000 – 0x2000_4FFF` | **SRAM** | 运行数据（F103C8T6 为 20 KB） |
| `0x4000_0000 – 0x4002_3FFF` | **外设** | APB1/APB2/AHB 外设寄存器 |
| `0x4200_0000` | **外设位带别名区** | 位带原子操作 |
| `0xE000_0000 – 0xE00F_FFFF` | **PPB 私有外设总线** | NVIC、SysTick、SCB、调试组件 |
| `0x6000_0000` / `0xA000_0000` | 外部存储/设备 | FSMC/FMC 挂接外部器件 |

### 关键地址速记

| 地址 | 含义 |
| --- | --- |
| `0x0000_0000` | 启动后取的**初始 MSP 值**（向量表第 0 项） |
| `0x0000_0004` | **复位向量**（Reset_Handler 地址） |
| `0x0800_0000` | STM32 主 Flash 起始（写代码时链接地址） |
| `0x2000_0000` | SRAM 起始（栈顶常在 SRAM 末尾） |
| `0x1FFF_F000` | 系统存储器（BOOT0=1 时启动） |
| `0x1FFF_F800` | 选项字节 |

> **注意**：地址 `0x0000_0000` 与 `0x0800_0000` 内容相同（前者是后者的别名），这是为了兼容 Cortex-M 从 0 地址取向量的规定。

---

## 三、片内 Flash

### 1. 组织方式

| 项目   | F1 系列                   | F4 系列                       | 说明             |
| ---- | ----------------------- | --------------------------- | -------------- |
| 起始地址 | `0x0800_0000`           | `0x0800_0000`               | 统一             |
| 擦除单位 | **页（Page）**，1 KB 或 2 KB | **扇区（Sector）**，16 KB~128 KB | F4 扇区大，升级分区要算好 |
| 写入单位 | 半字（16 位）                | 字节/半字/字                     | F1 只能按半字写      |
| 擦写寿命 | 约 1 万次                  | 约 1 万次                      | 频繁写需磨损均衡       |
| 数据保持 | 20 年（常温）                | 20 年                        | —              |
| 编程电压 | 内部产生（2.7~3.6 V）         | 内部产生                        | 无需外部高压         |
| 读保护  | RDP 选项字节                | RDP 等级 0/1/2                | 保护代码不被读出       |

### 2. Flash 编程四步（F1）

```
① 解锁：FLASH->KEYR = 0x45670123; FLASH->KEYR = 0xCDEF89AB;
② 擦除：FLASH->CR |= PER; 写地址; 置 STRT; 等待 BSY 清零
   （页擦除：先设 AR 地址 → PER=1 → STRT=1）
③ 写入：FLASH->CR |= PG; 向目标地址写半字; 等待 BSY 清零
④ 上锁：FLASH->CR |= LOCK;
```

> HAL 封装：`HAL_FLASH_Unlock()` → `HAL_FLASHEx_Erase()` → `HAL_FLASH_Program()` → `HAL_FLASH_Lock()`。

### 3. 关键注意点

- **必须先擦除再写入**：Flash 只能把 1 写成 0，擦除是把整页恢复为 1。
- **擦除是整页/整扇区**：改一个字节也要擦整页，所以参数存储要预留缓冲区。
- **写操作期间不能取指**（同一 Bank 内）：会停顿 CPU，升级时注意时序。
- **掉电风险**：擦写过程中掉电会导致该页数据损坏，重要数据要双备份 + 校验。
- **选项字节改动后需复位生效**，且误设 RDP 等级 2 后芯片不可再调试。

---

## 四、片内 SRAM

| 项目 | 说明 |
| --- | --- |
| 容量 | F103C8T6：20 KB；F407：192 KB（含 64 KB CCM）；H7：可达 1 MB |
| 起始 | `0x2000_0000` |
| 用途 | 全局/静态变量、堆（heap）、栈（stack）、DMA 缓冲区 |
| 掉电 | 数据丢失（备份域 SRAM 除外） |

### 多块 RAM（F4/F7/H7 特性）

| 块 | 地址 | 特点 |
| --- | --- | --- |
| SRAM1 | `0x2000_0000` | 通用，可被 DMA 访问 |
| SRAM2 | `0x2001_C000` | 通用 |
| **CCM RAM** | `0x1000_0000` | 内核紧耦合，**不能被 DMA 访问**，放栈与热点数据可提速 |

> **CCM 的坑**：把 DMA 缓冲区放在 CCM 会导致传输失败，链接脚本需正确分区。

### 栈与堆的方向

```
高地址  0x2000_5000  ← 栈顶（初始 MSP），栈向低地址增长 ↓
        ...
        ↓ 栈（stack）：局部变量、函数调用现场
        ↑ 堆（heap）：malloc 动态分配
        .bss   未初始化/初值为 0 的全局变量
        .data  已初始化的全局变量（启动时从 Flash 拷入）
低地址  0x2000_0000
```

**`.data` 与 `.bss` 的区别（面试常问）**

| 段 | 内容 | 存储在 Flash？ | 启动时动作 |
| --- | --- | --- | --- |
| `.text` | 代码、常量 | 是 | 无需搬移（XIP 直接执行） |
| `.data` | 已初始化的全局/静态变量 | **是（初值）** | 从 Flash **拷贝**到 SRAM |
| `.bss` | 未初始化或初值为 0 的变量 | 否（不占 Flash） | 在 SRAM 中**清零** |
| `stack/heap` | 栈与堆 | 否 | 运行时使用 |

---

## 五、启动流程详解

### 1. 硬件启动（上电到 main）

```
上电 / 复位
   ↓
① BOOT 引脚采样（复位后第 4 个 SYSCLK 上升沿锁存）
   ↓
② 硬件把选定的存储区**映射到 0x0000_0000**（别名）
   ↓
③ 从 0x0000_0000 取第 0 个字 → 装入 MSP（栈顶地址）
   ↓
④ 从 0x0000_0004 取第 1 个字 → 装入 PC（Reset_Handler 地址）
   ↓
⑤ 执行启动文件 startup_stm32f10x_md.s → Reset_Handler
   ↓
⑥ SystemInit()：配置时钟树（HSE + PLL）、向量表基址、FPU（F4）
   ↓
⑦ __main / 库函数：拷贝 .data、清零 .bss、初始化堆栈
   ↓
⑧ 调用 main() → HAL_Init() → 外设初始化 → while(1) 主循环
```

### 2. 向量表结构（前 16 项为系统异常）

| 偏移 | 内容 | 说明 |
| --- | --- | --- |
| `0x00` | 初始 MSP | 栈顶地址（SRAM 末尾） |
| `0x04` | Reset_Handler | 复位入口 |
| `0x08` | NMI_Handler | 不可屏蔽中断 |
| `0x0C` | HardFault_Handler | 硬件错误（调试重点） |
| `0x10` | MemManage_Handler | 存储器管理错误 |
| `0x14` | BusFault_Handler | 总线错误 |
| `0x18` | UsageFault_Handler | 用法错误 |
| `0x2C` | SVC_Handler | 系统服务调用 |
| `0x38` | PendSV_Handler | 可挂起系统调用（RTOS 切换） |
| `0x3C` | SysTick_Handler | 系统滴答 |
| `0x40` 起 | IRQ0…IRQn | 外设中断（WWDG、PVD、EXTI0…） |

> 启动文件里所有中断服务函数都有**弱定义（WEAK）**，你在 C 代码中重名定义即可覆盖。

### 3. BOOT 引脚与启动模式

| BOOT1 | BOOT0 | 启动模式 | 说明 |
| --- | --- | --- | --- |
| x | **0** | **主 Flash** | 正常运行用户程序（产品常态） |
| 0 | **1** | **系统存储器** | 运行 ST 出厂 Bootloader，可用串口/USB/CAN 下载 |
| 1 | 1 | **内嵌 SRAM** | 调试用（掉电即丢，需调试器加载） |

**工程实践**

- **正常产品**：BOOT0 通过 10 kΩ 电阻下拉到地，固定从 Flash 启动。
- **升级场景**：软件触发跳转到系统存储器（`__HAL_RCC_...` + 跳转），或直接实现 IAP。
- **常见坑**：BOOT0 悬空会因干扰误入 Bootloader 导致"程序不运行"，务必下拉。

### 4. 从系统存储器启动（串口 ISP）

```
① BOOT0 = 1，复位进入 ST Bootloader
② 用 STM32CubeProgrammer / Flash Loader Demonstrator 连接 USART1
   （PA9=TX, PA10=RX，常用 115200 8N1，部分型号支持 USB DFU）
③ 上位机发送同步字节 0x7F，握手成功后下载
④ 下载完毕 BOOT0 拨回 0，复位运行
```

> 量产产线常用 SWD 批量烧录；无调试口的产品用 ISP 或 IAP。

---

## 六、向量表重定位（IAP/RTOS 必备）

当程序不放在 `0x0800_0000`（例如在 Bootloader 之后的 App 区），必须重定位向量表，否则中断会跳到错误位置。

### 方法一：设置 SCB->VTOR（推荐）

```c
/* App 起始地址 = 0x08004000（Bootloader 占 16 KB） */
#define APP_ADDR  0x08004000
#define APP_OFFSET (APP_ADDR - 0x08000000)

int main(void)
{
    /* 早期设置向量表偏移，F1 用 NVIC_SetVectorTable */
    SCB->VTOR = APP_ADDR & 0x1FFFFF80;   /* F4/F7/H7 */
    /* NVIC_SetVectorTable(NVIC_VectTab_FLASH, APP_OFFSET);  F1 写法 */

    HAL_Init();
    SystemClock_Config();
    /* ... */
}
```

### 方法二：在链接脚本中定义并拷贝向量表

```c
extern uint32_t _sidata, _sapp_vectors, _eapp_vectors;
/* 把 App 的向量表拷到 SRAM 再设 VTOR 指向 SRAM，可支持动态修改 */
```

### 重定位要点

- **VTOR 必须 32 字节（F1）/ 128 字节（F4+）对齐**。
- **在打开任何中断之前设置**，否则一个中断就可能跑飞。
- App 的链接地址必须与应用区起始一致（见下节链接脚本）。
- 若使用 RTOS，`PendSV/SysTick` 也在向量表中，重定位同样影响它们。

---

## 七、链接脚本与工程配置

### 1. Keil 中的两个地址设置

| 配置项 | 位置 | 示例（App） |
| --- | --- | --- |
| **IROM1 Start/Size** | Options → Target | Start `0x08004000`，Size `0x3C000`（48 KB） |
| **IRAM1 Start/Size** | Options → Target | Start `0x20000000`，Size `0x5000`（20 KB） |

> 这两个值必须与启动文件和 `VTOR` 设置一致，否则下载后无法运行。

### 2. GCC/CMake 的链接脚本（.ld）

```ld
/* STM32F103C8T6：64 KB Flash / 20 KB RAM，划分 Boot 16 KB + App 48 KB */
MEMORY
{
  FLASH (rx)  : ORIGIN = 0x08004000, LENGTH = 48K   /* App 区 */
  RAM   (xrw) : ORIGIN = 0x20000000, LENGTH = 20K
}

_estack = ORIGIN(RAM) + LENGTH(RAM);   /* 初始 MSP = RAM 末尾 */

SECTIONS
{
  .isr_vector : { KEEP(*(.isr_vector)) } >FLASH     /* 向量表放最前 */
  .text       : { *(.text*) *(.rodata*) } >FLASH
  _sidata = LOADADDR(.data);
  .data : { _sdata = .; *(.data*) _edata = .; } >RAM AT> FLASH
  .bss  : { _sbss = .; *(.bss*) *(COMMON) _ebss = .; } >RAM
}
```

### 3. 启动文件中的段搬运（对应上面符号）

```asm
Reset_Handler:
    LDR  R0, =_sidata      ; .data 在 Flash 中的源地址
    LDR  R1, =_sdata       ; .data 在 RAM 中的目标地址
    LDR  R2, =_edata
CopyLoop:
    CMP  R1, R2
    ITTT LT
    LDRLT R3, [R0], #4
    STRLT R1!, {R3}
    BLT  CopyLoop
    ; 之后清零 .bss 并跳转 __main
```

---

## 八、Flash 分区规划实践

### 例：F103C8T6（64 KB Flash / 20 KB RAM）双分区升级方案

| 分区 | 地址范围 | 大小 | 内容 |
| --- | --- | --- | --- |
| Bootloader | `0x0800_0000 – 0x0800_3FFF` | 16 KB | 升级协议、校验、跳转逻辑 |
| App A | `0x0800_4000 – 0x0800_BFFF` | 32 KB | 应用程序（运行区） |
| App B | `0x0800_C000 – 0x0801_3FFF` | — | 超出 64 KB，需换更大容量芯片 |
| 参数区 | 最后 2 页 | 2 KB | 版本号、标志位、配置参数 |

> **容量现实**：双分区需要 ≥ 2 倍程序容量。F103C8T6 只有 64 KB，实际项目常改为"**单分区 + Bootloader + 标志位回滚**"或换用 128 KB 以上芯片（如 C8T6 实际常有 128 KB，但不建议依赖）。

### 标志位与参数存储（模拟 EEPROM）

```c
/* 直接在 Flash 中划一页做参数区，结构体 + CRC 校验 + 双备份 */
typedef struct {
    uint32_t magic;        /* 0x5A5A1234 标识有效 */
    uint32_t version;
    uint8_t  params[64];
    uint32_t crc;          /* 校验整个结构体 */
} param_t;

/* 写入流程：擦页 → 写主份 → 校验 → 写备份 → 校验 → 更新标志 */
```

**设计要点**

- **双备份 + 交替写入**：一份损坏时用另一份，延长擦写寿命。
- **每页预留多个槽位**：顺序写入，写满再统一擦除（磨损均衡）。
- **写入前必须校验电源**（可用 PVD 检测低压）。
- 频繁写参数时考虑外挂 EEPROM（如 AT24C02）或 FRAM。

---

## 九、IAP / OTA 固件升级

### 1. IAP 基本原理

```
┌────────────┐   ①上电运行    ┌─────────────────────────────┐
│ Bootloader │ ─────────────→ │ 检查升级标志 / 接收新固件    │
└─────┬──────┘                └───────────┬─────────────────┘
      │ ②无升级请求                        │ ③有升级请求
      ↓                                    ↓
┌────────────┐                  ┌──────────────────────┐
│ 跳转 App    │                  │ 接收数据写入 App B 区 │
│ (设 VTOR)   │                  │ 校验 CRC → 置标志     │
└────────────┘                  └──────────┬───────────┘
                                           ↓
                                     ④ 复位 → 运行新固件
```

### 2. 跳转到应用程序的代码

```c
typedef void (*pFunction)(void);

void jump_to_app(uint32_t app_addr)
{
    uint32_t sp = *(volatile uint32_t *)app_addr;        /* 取新栈顶 */
    uint32_t pc = *(volatile uint32_t *)(app_addr + 4);  /* 取复位向量 */

    /* 合法性检查：栈顶应在 SRAM 范围 */
    if ((sp & 0x2FFE0000) != 0x20000000) return;

    __disable_irq();                 /* 关总中断 */
    HAL_RCC_DeInit();                /* 复位时钟到默认 */
    HAL_DeInit();                    /* 复位外设 */
    SysTick->CTRL = 0; SysTick->LOAD = 0; SysTick->VAL = 0;
    for (int i = 0; i < 8; i++) {    /* 清除所有中断使能与挂起位 */
        NVIC->ICER[i] = 0xFFFFFFFF;
        NVIC->ICPR[i] = 0xFFFFFFFF;
    }
    SCB->VTOR = app_addr;            /* 向量表搬到 App */
    __set_MSP(sp);                   /* 设置主栈指针 */
    __enable_irq();
    ((pFunction)pc)();               /* 跳转执行 */
}
```

### 3. 升级方案对比

| 方案 | 优点 | 缺点 | 适用 |
| --- | --- | --- | --- |
| 系统存储器 ISP | 无需自己写 Bootloader | 需 BOOT0 操作/上位机 | 产线与研发 |
| 单分区 IAP | 省 Flash | 升级中断电变砖，无回滚 | 小容量、风险可接受 |
| 双分区（A/B） | 可回滚、升级失败不影响运行 | 需 2 倍空间 | 联网设备首选 |
| 外部 Flash 缓存 | 不占片内空间、可断点续传 | 需外挂器件 | 大固件、OTA |
| 差分升级 | 传输量小 | 实现复杂 | 流量敏感的 IoT |

### 4. 可靠性设计清单

- **固件完整性**：CRC32 / SHA256 校验，不通过则丢弃。
- **版本比对**：防止降级或重复升级。
- **升级标志用双备份**：避免掉电丢标志。
- **看门狗配合**：升级过程喂狗，超时自动回滚。
- **回滚机制**：新固件首次启动需"确认成功"（写标记），否则重启回旧分区。
- **传输层**：串口 / CAN / 以太网 / BLE，加超时与重传。
- **升级期间关中断慎用**：写 Flash 会停顿总线，注意通信超时。

---

## 十、常见问题排查

| 现象 | 原因 | 排查方向 |
| --- | --- | --- |
| 下载成功但不运行 | BOOT0 未下拉、链接地址与分区不符 | 查 BOOT0 电平与 IROM 起始地址 |
| 跳转 App 后中断不响应 | **忘记设置 VTOR** 或设置时机太晚 | 检查 SCB->VTOR 与对齐 |
| 跳转后卡死 | 未关中断/未复位外设、栈顶非法 | 校验 sp/pc、DeInit 外设 |
| Flash 写入失败 | 未解锁、未擦除直接写、地址越界 | 按解锁→擦除→写→上锁流程 |
| 写 Flash 后程序异常 | 在 Bank 内写导致取指停顿，或中断未屏蔽 | 写时关中断，避开代码所在扇区 |
| 参数保存后丢失 | 未校验、掉电在擦写中途、写到错误页 | 加 CRC + 双备份 + PVD 检测 |
| 出现 HardFault | 栈溢出、CCM 被 DMA 访问、空指针 | 查栈帧 PC、检查 DMA 缓冲位置 |
| 读保护开启后连不上 | RDP 生效 | 用 BOOT0=1 进系统存储器解除 |
| 升级后无法启动 | 向量表/分区/校验不匹配 | 逐项核对 Boot 与 App 约定 |
| Flash 数据逐年损坏 | 超过数据保持年限或频繁擦写 | 定期刷新、磨损均衡、外挂 EEPROM |

### 调试备忘

- **查看 Flash/RAM 占用**：Keil 编译输出的 Program Size / `arm-none-eabi-size`。
- **定位 HardFault**：读栈帧中 PC 值 → 在反汇编中定位指令。
- **验证向量表**：用调试器查看 `0x0800_0000` 处的两个 32 位数（栈顶 + 复位向量）。
- **检查段搬运**：确认 `.data` 的 Flash 源地址与 RAM 目标地址符号正确。

---

## 十一、术语速查

| 缩写 | 全称 | 含义 |
| --- | --- | --- |
| MSP | Main Stack Pointer | 主栈指针（初始值＝SRAM 末尾） |
| VTOR | Vector Table Offset Register | 向量表偏移寄存器 |
| .text | — | 代码段 |
| .data | — | 已初始化数据段（需从 Flash 搬运） |
| .bss | Block Started by Symbol | 未初始化数据段（需清零） |
| IAP | In-Application Programming | 在应用编程（自升级） |
| ISP | In-System Programming | 在系统编程（串口下载） |
| DFU | Device Firmware Upgrade | USB 固件升级 |
| RDP | Read-out Protection | 读出保护（选项字节） |
| OTP | One-Time Programmable | 一次性可编程区 |
| FMC/FSMC | Flexible (Static) Memory Controller | 外部存储器控制器 |
| CCM RAM | Core Coupled Memory | 内核紧耦合 RAM（不可 DMA） |
| PVD | Programmable Voltage Detector | 可编程电压检测器 |
| CRC | Cyclic Redundancy Check | 循环冗余校验 |
| XIP | eXecute In Place | 在 Flash 中直接执行 |
| BOR | Brown-Out Reset | 欠压复位 |
| VBAT | — | 备份域供电引脚 |

---

## 十二、小结

- STM32 的存储器是**统一编址**的：主 Flash 在 `0x0800_0000`，SRAM 在 `0x2000_0000`，外设在 `0x4000_0000`，启动区 `0x0000_0000` 是所选启动存储器的**别名**。
- 启动的本质只有两步：**取栈顶（第 0 字）→ 取复位向量（第 1 字）**，之后由启动文件搬 `.data`、清 `.bss`、调 `main`。
- BOOT0/BOOT1 决定从 **Flash / 系统存储器 / SRAM** 启动；产品务必把 BOOT0 下拉。
- 只要程序不在 `0x0800_0000`，就**必须重定位向量表（VTOR）**，并与链接脚本地址、Keil IROM 设置三者一致。
- 参数存储与固件升级的可靠性核心：**CRC 校验 + 双备份 + 掉电保护 + 可回滚**。

> 下一步计划：整理一份"Bootloader + 双分区 + CRC 校验 + 回滚"的完整可运行代码框架，并记录 F103/F407/H743 三款芯片的 Flash 扇区差异对照表。
