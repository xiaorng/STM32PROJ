/*
 * platform_motor_b6612.h
 *
 *  Created on: 2026年1月9日
 *      Author: SYRLIST
 */

#ifndef PLATFORM_MOTOR_B6612_H_
#define PLATFORM_MOTOR_B6612_H_
#pragma once
#include <stdint.h>

void motor_init(void);
void motor_set(int16_t duty);   // -100~100，负数反转
void motor_brake(void);



#endif /* PLATFORM_MOTOR_B6612_H_ */
