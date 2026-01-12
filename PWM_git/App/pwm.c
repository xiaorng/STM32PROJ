
#include "pwm.h"

static TIM_HandleTypeDef *g_htim = NULL;
static uint32_t g_channel = 0;
static uint8_t g_percent = 0;

void pwm_init(TIM_HandleTypeDef *htim, uint32_t channel){
    g_htim = htim; g_channel = channel;
    HAL_TIM_PWM_Start(g_htim, g_channel);
}

static void apply_ccr_from_percent(uint8_t p){
    if(!g_htim) return;
    uint32_t arr = __HAL_TIM_GET_AUTORELOAD(g_htim);
    uint32_t ccr = (arr * p) / 100;
    __HAL_TIM_SET_COMPARE(g_htim, g_channel, ccr);
}

void pwm_set_percent(uint8_t percent){
    if(percent > 100) percent = 100;
    g_percent = percent;
    apply_ccr_from_percent(percent);
}
void pwm_apply_percent(uint8_t pct){ pwm_set_percent(pct); }
void pwm_shutdown(void){ pwm_set_percent(0); }
uint8_t pwm_get_percent(void){ return g_percent; }
