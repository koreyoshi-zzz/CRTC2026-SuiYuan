/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "tim.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include"ps2.h"
#include"motor.h"
#include"servo.h"
#include"debug_config.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define STICK_DEADZONE   12
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
/* 摇杆线性映射：中位 128，死区 ±12，满偏 → ±1000
 * 实际摇杆不会完全顶到 0 或 255，末端留出余量 */
static int16_t stick_to_speed(uint8_t v)
{
  int16_t x = (int16_t)v - 128;
  if (x > -STICK_DEADZONE && x < STICK_DEADZONE) return 0;
  return (int16_t)(x * 1000 / 128);
}

/* 检查摇杆是否全部回中（用于 START 解锁前的安全条件） */
static uint8_t sticks_centered(const PS2_State_t *s)
{
  return (s->lx > 128 - STICK_DEADZONE && s->lx < 128 + STICK_DEADZONE &&
          s->ly > 128 - STICK_DEADZONE && s->ly < 128 + STICK_DEADZONE &&
          s->rx > 128 - STICK_DEADZONE && s->rx < 128 + STICK_DEADZONE &&
          s->ry > 128 - STICK_DEADZONE && s->ry < 128 + STICK_DEADZONE);
}
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_TIM1_Init();
  MX_TIM2_Init();
  MX_TIM4_Init();
  /* USER CODE BEGIN 2 */
  Motor_Init();
  Servo_Init();
  PS2_Init();

  /* ---- 分模块调试入口 ----
   * 当 debug_config.h 里的 DEBUG_MODE 不是 NORMAL 时，
   * 程序会进入对应的调试函数（它们内部各自是死循环，不会返回）。
   * 想跑正式程序，把 DEBUG_MODE 改回 DEBUG_MODE_NORMAL 即可。 */
#if DEBUG_MODE == DEBUG_MODE_SERVO
  Debug_Servo_Run();
#elif DEBUG_MODE == DEBUG_MODE_MOTOR
  Debug_Motor_Run();
#elif DEBUG_MODE == DEBUG_MODE_PS2
  Debug_PS2_Run();
#endif
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  PS2_State_t ps2;                        /* 手柄状态：每帧刷新一次 */
  uint16_t last_btn = 0;                  /* 上一帧按键位图，用来做边沿检测 */
  uint8_t  unlocked = 0;                  /* 0=上电锁定  1=已解锁 */
  float ang0 = 90.0f, ang1 = 90.0f, ang2 = 90.0f;
  while (1)
  {
	  if (PS2_Poll(&ps2))                         /* 读到一帧有效数据 */
	  {
		  HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_RESET);  /* PC13 低电平点亮：在线 */

		  /* ---- 边沿检测：这一帧“刚按下”的键（避免按住反复触发） ---- */
		  uint16_t pressed = ps2.buttons & (uint16_t)(~last_btn);

		  /* ---- 启动控制：SELECT 随时锁定；
		   * START 仅在「未锁定 + R1 已松开 + 四摇杆全部归中」时才解锁，
		   * 防止手柄放在未收好的位置导致机器人上电即冲出启动区 ---- */
		  if (pressed & PS2_BTN_SELECT)
			  unlocked = 0;                                   /* SELECT：锁定 */
		  if (!unlocked && (pressed & PS2_BTN_START) &&
			  !(ps2.buttons & PS2_BTN_R1) && sticks_centered(&ps2))
		  {
			  unlocked = 1;                                   /* START：安全解锁 */
		  }

		  /* ---- 底盘：按技术文档 5.6/5.7 映射 ----
		   * 左摇杆上下 → 前进 / 后退
		   * 左摇杆左右 → 左右平移
		   * 右摇杆左右 → 原地旋转
		   * R1 按住才行驶，松开立即停车；R2 按住为精细档（三个分量减半） */
		  if (unlocked && (ps2.buttons & PS2_BTN_R1))
		  {
			  int16_t forward = -stick_to_speed(ps2.ly);  /* 左摇杆上下 → 前进/后退 */
			  int16_t strafe  =  stick_to_speed(ps2.lx);  /* 左摇杆左右 → 左右平移 */
			  int16_t rotate  =  stick_to_speed(ps2.rx);  /* 右摇杆左右 → 旋转 */

			  if (ps2.buttons & PS2_BTN_R2)                /* R2 精细档 */
			  {
				  forward /= 2;
				  strafe  /= 2;
				  rotate  /= 2;
			  }
			  Motor_Drive(forward, strafe, rotate);
		  }
		  else
		  {
			  Motor_Drive(0, 0, 0);                       /* 未解锁 / 未按 R1：停车 */
		  }

		  /* ---- 舵机：由摇杆外的按键控制（十字键调 1/2 号，△/□ 调 3 号） ---- */
		  if (ps2.buttons & PS2_BTN_UP)       ang0 += 2.0f;   /* ↑   舵机1 + */
		  if (ps2.buttons & PS2_BTN_DOWN)     ang0 -= 2.0f;   /* ↓   舵机1 - */
		  if (ps2.buttons & PS2_BTN_RIGHT)    ang1 += 2.0f;   /* →   舵机2 + */
		  if (ps2.buttons & PS2_BTN_LEFT)     ang1 -= 2.0f;   /* ←   舵机2 - */
		  if (ps2.buttons & PS2_BTN_TRIANGLE) ang2 += 2.0f;   /* △   舵机3 + */
		  if (ps2.buttons & PS2_BTN_SQUARE)   ang2 -= 2.0f;   /* □   舵机3 - */
		  Servo_SetAngle(SERVO_SG90_1, ang0);
		  Servo_SetAngle(SERVO_SG90_2, ang1);
		  Servo_SetAngle(SERVO_MG995,  ang2);

		  if (pressed & PS2_BTN_CROSS)  HAL_GPIO_TogglePin(RELAY_GPIO_Port, RELAY_Pin); /* ✕ 继电器 */
		  last_btn = ps2.buttons;                     /* 存档，供下一帧做边沿比较 */
	  }
	  else                                          /* 手柄掉线 */
	  {
		  HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_SET);    /* 灯灭 */
		  Motor_Drive(0, 0, 0);                         /* 立即停车（安全兜底） */
		  unlocked = 0;                                 /* 掉线后重新上锁，恢复后需再次 START 解锁 */
		  last_btn = 0;
	  }
	  HAL_Delay(15);                                /* 约 60Hz 轮询手柄 */

	  /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
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
