/*
 * bsp_pwm.h
 *
 *  Created on: 2025年12月30日
 *      Author: SYRLIST
 */

#ifndef INC_BSP_PWM_H_
#define INC_BSP_PWM_H_

#pragma once
#include <stdint.h>

void BSP_PWM_Start(void);
void BSP_PWM_Stop(void);
void BSP_PWM_SetDuty(uint16_t duty); // 0..ARR


#endif /* INC_BSP_PWM_H_ */
