\# SYRLIST — STM32F411 非阻塞串口 CLI + PWM 调光/呼吸 + ADC 内部温度



一个可复用的 STM32F4 小框架：把“能跑”升级为“像工程”的那种。

核心目标：\*\*串口不阻塞、可交互调试、可观测、可扩展\*\*。



---



\## 功能概览



\- \*\*UART2 非阻塞收发\*\*

  - RX：中断逐字节入 \*\*Ring Buffer\*\*

  - TX：中断分段续发（避免 `printf` 卡死主循环）

\- \*\*CLI 命令行\*\*

  - `help / status / led / pwm / breath / temp`（见下）

\- \*\*PWM 亮度控制\*\*

  - 支持手动占空比（0–100）

  - 支持呼吸灯模式（周期/最小/最大）

\- \*\*ADC 内部温度\*\*

  - 读取 MCU \*\*内部温度传感器\*\*（die temperature），非环境温度



---



\## 硬件与工具



\- Board: \*\*NUCLEO-F411RE / STM32F411RETx\*\*

\- 串口：\*\*USART2\*\*（通常走 ST-LINK VCP，PA2/PA3）

\- PWM：\*\*TIM2 CH1\*\*（具体引脚以 `.ioc`/`main.c` 生成配置为准）

\- IDE: \*\*STM32CubeIDE\*\*

\- Toolchain: `arm-none-eabi-gcc` + STM32 HAL



---



\## 快速开始



\### 1) 克隆仓库

```bash

git clone https://github.com/xiaorng/SYRLIST.git




