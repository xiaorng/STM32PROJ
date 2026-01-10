# STM32PROJ

Personal STM32 practice projects (STM32F411RE / NUCLEO-F411RE).

## Repo layout

- `proje/`
  - UART non-blocking + CLI + PWM + ADC temperature + breathing LED (baseline utilities)

- `FOC_F411_Base/FOC_F411_Base/`
  - Motor control base project (TIM1 PWM CH1/CH2/CH3 ready)
  - `App/` : application logic (CLI commands, control loop, etc.)
  - `platform/` : hardware abstraction (PWM / motor driver TB6612, etc.)

## Build / Flash

Open `.ioc` with STM32CubeMX → Generate Code → open project with STM32CubeIDE → Build & Debug.

## Notes
- UART used for CLI (115200 8N1)
- TIM1 provides PWM outputs on PA8/PA9/PA10 (CH1/CH2/CH3)


# Open `.ioc` with STM32CubeMX → Generate Code → open project with STM32CubeIDE → Build \& Debug.

# 

# \## Notes

# \- UART used for CLI (115200 8N1)

# \- TIM1 provides PWM outputs on PA8/PA9/PA10 (CH1/CH2/CH3)

# 

