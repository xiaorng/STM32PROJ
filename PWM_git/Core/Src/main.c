#include "main.h"
#include <stdio.h>
#include <string.h>
#include "pwm.h"
#include "uart_log.h"
#include "adc_sense.h"
#include <stdbool.h>
// ====== 由 CubeMX 在其它 .c 里定义，这里只声明 ======
extern ADC_HandleTypeDef  hadc1;
extern TIM_HandleTypeDef  htim2;   // PWM 定时器
extern TIM_HandleTypeDef  htim4;   // 1kHz 控制定时器
extern UART_HandleTypeDef huart2;

// 来自 adc_sense.c 的滤波结果（DMA+平均）
extern volatile uint16_t adc_avg;

// ====== 本文件内用的小工具 ======
static inline int clampi(int v, int lo, int hi){
    if(v < lo) return lo;
    if(v > hi) return hi;
    return v;
}

// ====== 状态/参数 ======
typedef enum { ST_IDLE=0, ST_RUN=1, ST_FAULT=2 } sys_state_t;
static volatile sys_state_t g_state = ST_IDLE;
static volatile uint8_t  arm_ok    = 1;
static volatile uint8_t  use_cl    = 0;      // 0=开环, 1=闭环
static volatile uint8_t  duty_cmd  = 10;     // 5..95 %
static volatile uint16_t target_adc= 2600;

#define KP      6
#define KI_Q    1
#define I_SHIFT 0
#define I_MAX   (4095*8)
#define I_MIN   (-I_MAX)

static int32_t I_acc = 0;
static volatile uint32_t ms = 0;

// ====== 运行/空闲 ======
static void enter_idle(void){ g_state = ST_IDLE; pwm_set_percent(0); }
static void enter_run(void){  if(arm_ok && g_state!=ST_FAULT) g_state = ST_RUN; }
static const char* mode_str(void){ return use_cl ? "CL":"OL"; }

// ====== 指令解析辅助 ======
static int parse_num2(const char *s, uint32_t *out){
    if(s[0]<'0'||s[0]>'9'||s[1]<'0'||s[1]>'9') return 0;
    *out = (uint32_t)(s[0]-'0')*10u + (uint32_t)(s[1]-'0');
    return 1;
}
static int parse_num4(const char *s, uint32_t *out){
    for(int i=0;i<4;i++) if(s[i]<'0'||s[i]>'9') return 0;
    *out = ((uint32_t)(s[0]-'0')*1000u)+((uint32_t)(s[1]-'0')*100u)
         + ((uint32_t)(s[2]-'0')*10u)+ (uint32_t)(s[3]-'0');
    return 1;
}

// ====== 1kHz 控制环 ======
static void control_tick_1kHz(void){
    uint16_t adc = adc_avg;

    if (g_state == ST_RUN){
        if (use_cl){
            // === A) 若发现方向相反，把下面这一行的符号变成负的：err = target - adc;
            int32_t err = (int32_t)target_adc - (int32_t)adc;

            // 饱和边界
            const int DUTY_MIN = 5, DUTY_MAX = 95;

            // 仅当未饱和或误差方向有利于“脱离饱和”时才积分（抗风up）
            bool at_upper = (duty_cmd >= DUTY_MAX);
            bool at_lower = (duty_cmd <= DUTY_MIN);
            bool freeze_I = ( (at_upper && err > 0) || (at_lower && err < 0) );

            if (!freeze_I){
                I_acc += err;
                if (I_acc > I_MAX) I_acc = I_MAX;
                if (I_acc < I_MIN) I_acc = I_MIN;
            }

            // 注意位移顺序：先乘后右移
            // 建议初始：KP=16, KI_Q=8, I_SHIFT=8  （等体检通过后再细调）
            int32_t pi = (int32_t)KP * err + ( ( (int32_t)KI_Q * I_acc ) >> I_SHIFT );

            // 把 PI 输出映射到占空微调，步子不要太大
            int d = (int)duty_cmd + (pi >> 7);   // 右移 7 只是“缩放旋钮”，必要时改成 >>6 或 >>8
            if (d < DUTY_MIN) d = DUTY_MIN;
            if (d > DUTY_MAX) d = DUTY_MAX;
            duty_cmd = (uint8_t)d;
        }

        pwm_set_percent(duty_cmd);
    }else{
        pwm_set_percent(0);
    }

    if ((ms % 100) == 0){
        log_printf("ms=%lu state=%d fault=0(OK) mode=%s duty%%%u adc_avg=%u target=%u arm=%u\r\n",
                   (unsigned long)ms, (int)g_state, mode_str(),
                   (unsigned)duty_cmd, (unsigned)adc, (unsigned)target_adc, (unsigned)arm_ok);
    }
}


// ====== 串口命令：逐行解析 ======
static void process_command_line(char *cmd){
    if(cmd[0]==0) return;

    if(!strcmp(cmd,"c") || !strcmp(cmd,"C")){ enter_idle(); log_printf("[CMD] clear -> IDLE\r\n"); return; }
    if(!strcmp(cmd,"k") || !strcmp(cmd,"K")){ enter_idle(); log_printf("[CMD] enter IDLE\r\n");    return; }
    if(!strcmp(cmd,"e") || !strcmp(cmd,"E")){ enter_run();  log_printf("[CMD] enter RUN\r\n");     return; }
    if(!strcmp(cmd,"s") || !strcmp(cmd,"S")){
        log_printf("[CMD] state=%d mode=%s duty%%%u adc=%u target=%u\r\n",
            (int)g_state, mode_str(), (unsigned)pwm_get_percent(), (unsigned)adc_avg, (unsigned)target_adc);
        return;
    }
    if((cmd[0]=='d'||cmd[0]=='D') && strlen(cmd)>=3){
        uint32_t v;
        if(parse_num2(&cmd[1], &v)){
            duty_cmd = (uint8_t)clampi((int)v,5,95);
            use_cl = 0;
            enter_run();
            log_printf("[CMD] open-loop duty=%u%%\r\n", (unsigned)duty_cmd);
        }
        return;
    }
    if((cmd[0]=='t'||cmd[0]=='T') && strlen(cmd)>=5){
        uint32_t v;
        if(parse_num4(&cmd[1], &v)){
            if(v>4095) v=4095;
            target_adc = (uint16_t)v;
            use_cl = 1;
            enter_run();
            log_printf("[CMD] closed-loop target=%u\r\n", (unsigned)target_adc);
        }
        return;
    }
    if (use_cl && (ms % 100 == 0)) {
        int32_t err = (int32_t)target_adc - (int32_t)adc_avg;
        log_printf("PI dbg: adc=%u tar=%u err=%ld duty=%u\r\n",
                   (unsigned)adc_avg, (unsigned)target_adc, (long)err, (unsigned)duty_cmd);
    }

}

static void uart_poll_cmd(void){
    uint8_t ch;
    if(HAL_UART_Receive(&huart2, &ch, 1, 0) != HAL_OK) return;

    static char line[32];
    static uint8_t idx = 0;

    if(ch=='\r' || ch=='\n'){
        if(idx){
            line[idx] = '\0';
            process_command_line(line);
            idx = 0;
        }
    }else{
        if(idx < sizeof(line)-1) line[idx++] = (char)ch;
        else idx = 0; // 溢出清零
    }
}

// ====== TIM4 中断：1ms ======
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim){
    if(htim->Instance == TIM4){
        ms++;
        control_tick_1kHz();
    }
}

// ====== 由 CubeMX 生成的 init 原型（在各自 .c 内实现） ======
void SystemClock_Config(void);
void MX_GPIO_Init(void);
void MX_DMA_Init(void);
void MX_USART2_UART_Init(void);
void MX_ADC1_Init(void);
void MX_TIM2_Init(void);
void MX_TIM4_Init(void);

// ====== 主函数 ======
int main(void)
{
    HAL_Init();
    SystemClock_Config();

    MX_GPIO_Init();
    MX_DMA_Init();
    MX_USART2_UART_Init();
    MX_ADC1_Init();
    MX_TIM2_Init();
    MX_TIM4_Init();

    uart_log_init(&huart2);
    log_printf("\r\nBOOT: e=run, k=idle, dXX, tXXXX\r\n");

    pwm_init(&htim2, TIM_CHANNEL_1);     // 由 pwm.c 提供
    pwm_set_percent(duty_cmd);           // 默认 10%

    adc_start_dma(&hadc1);               // 由 adc_sense.c 提供
    HAL_TIM_Base_Start_IT(&htim4);       // 1kHz 控制环

    for(;;){
        uart_poll_cmd();
    }
}

// ====== 时钟与错误处理（给链接用；你已有可留用已有实现） ======


// 84MHz: HSE/HSI->PLL，根据你项目里 CubeMX 已生成的 SystemClock_Config 保持一致。
// 若你已有实现，请删掉下面这个并用你现成的那个。
void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
    RCC_OscInitStruct.HSIState       = RCC_HSI_ON;
    RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    RCC_OscInitStruct.PLL.PLLState   = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource  = RCC_PLLSOURCE_HSI;
    RCC_OscInitStruct.PLL.PLLM       = 16;
    RCC_OscInitStruct.PLL.PLLN       = 336;
    RCC_OscInitStruct.PLL.PLLP       = RCC_PLLP_DIV4;   // SYSCLK 84MHz
    RCC_OscInitStruct.PLL.PLLQ       = 7;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) Error_Handler();

    RCC_ClkInitStruct.ClockType      = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                                     | RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider  = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;   // 42MHz
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;   // 84MHz
    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK) Error_Handler();
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
