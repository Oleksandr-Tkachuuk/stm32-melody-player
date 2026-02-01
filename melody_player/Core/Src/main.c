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

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "speaker.h"
#include "ws2812.h"
#include "font8x8.h"
#include "melodies.h"
#include "bt.h"
#include "string.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
TIM_HandleTypeDef htim1;
TIM_HandleTypeDef htim2;
DMA_HandleTypeDef hdma_tim1_ch1;

UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */
bt_context_t bt_ctx;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_TIM1_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_TIM2_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */


typedef enum {
    SYS_STOPPED = 0,
    SYS_PLAYING
} system_state_t;

system_state_t system_state = SYS_STOPPED;


typedef struct {
    uint8_t  melody_id;
    uint16_t step_index;
    uint32_t step_time_left_ms;
    uint16_t current_freq;
} player_ctx_t;

player_ctx_t player = {
    .melody_id = 0,
    .step_index = 0,
    .step_time_left_ms = 0,
    .current_freq = 0
};


// ===== LED MODE 2: Spectral Wave =====
static uint8_t wave_buf[8][8];   // инерция яркости
static uint8_t wave_phase = 0;
static uint8_t wave_div = 0;
static uint8_t hills[8][8];   // яркость пикселей


static uint8_t height_from_freq(uint16_t freq)
{
    if (freq == 0) return 0;

    if (freq < 300) return 2;
    if (freq < 400) return 3;
    if (freq < 500) return 4;
    if (freq < 600) return 5;
    if (freq < 700) return 6;
    return 7;
}



static void color_from_height_xy(uint8_t y, uint8_t x,
                                 uint8_t *r, uint8_t *g, uint8_t *b)
{
    // y: 0 (низ) ... 7 (верх)

    // --- ВЕРТИКАЛЬНЫЙ ГРАДИЕНТ ---
    // желтый -> оранжевый -> красный
    const uint8_t r0 = 255, g0 = 255, b0 = 0;    // низ (желтый)
    const uint8_t r1 = 255, g1 = 140, b1 = 0;    // середина (оранжевый)
    const uint8_t r2 = 255, g2 = 0,   b2 = 0;    // верх (красный)

    uint8_t ty = (y * 255) / 7;

    uint8_t br, bg, bb;

    if (ty < 128) {
        // низ -> середина
        uint8_t t = ty * 2;
        br = r0 + ((r1 - r0) * t >> 8);
        bg = g0 + ((g1 - g0) * t >> 8);
        bb = b0 + ((b1 - b0) * t >> 8);
    } else {
        // середина -> верх
        uint8_t t = (ty - 128) * 2;
        br = r1 + ((r2 - r1) * t >> 8);
        bg = g1 + ((g2 - g1) * t >> 8);
        bb = b1 + ((b2 - b1) * t >> 8);
    }

    // --- ЛЁГКАЯ ЯРКОСТЬ ПО X (не цвет!) ---
    uint8_t lum = 170 + (x * 80) / 7;   // 170..250

    *r = (br * lum) >> 8;
    *g = (bg * lum) >> 8;
    *b = (bb * lum) >> 8;
}

static const uint8_t RK_bitmap[8] = {
		 	0b00000000, // y = 0
		    0b11101001, // y = 1   R R R . | K . . K
		    0b10011010, // y = 2   R . . R | K . K .
		    0b11101100, // y = 3   R R R . | K K . .
		    0b11001010, // y = 4   R R . . | K . K .
		    0b10101001, // y = 5   R . R . | K . . K
		    0b10011001, // y = 6   R . . R | K . . K
		    0b00000000  // y = 7
};



void scheduler_tick_1ms(void)
{
    if (system_state != SYS_PLAYING)
        return;

    if (player.step_time_left_ms > 0) {
        player.step_time_left_ms--;
        return;
    }

    const melody_t *m = &g_melodies[player.melody_id];

    if (player.step_index >= m->length) {
        player.step_index = 0;   // loop мелодии
        return;
    }

    const melody_step_t *step = &m->steps[player.step_index];

    // --- AUDIO ---
    player.current_freq = step->freq_hz;
    Speaker_Set_Tone(step->freq_hz, 80);

    // --- LED (ТОЛЬКО ПО MODE) ---
    if (bt_ctx.led_mode == 0) {
        // режим 0 — цвет по частоте
        WS2812_ShowNoteColor(player.current_freq);
    }
    else if (bt_ctx.led_mode == 1) {

        // --- 1. Сдвиг всей карты вправо ---
        for (int y = 0; y < 8; y++) {
            for (int x = 7; x > 0; x--) {
                hills[y][x] = hills[y][x - 1];
            }
            hills[y][0] = 0;
        }

        // --- 2. Новая "гора" слева ---
        uint8_t h = height_from_freq(player.current_freq);

        for (int y = 0; y < h; y++) {
            hills[7 - y][0] = 1;   // логическое "есть пиксель"
        }

        // --- 3. Получаем цвет как в MODE 0 ---
        uint8_t r = 0, g = 0, b = 0;
        WS2812_Clear();

        // Используем ту же карту, что и MODE 0
        // (просто берём цвет текущей ноты)
        for (int i = 0; i < MAX_LED; i++) {
            // временно заполним, потом перерисуем
            Set_LED(i, 0, 0, 0);
        }

        WS2812_ShowNoteColor(player.current_freq);

        // --- 4. Перерисовываем только нужные пиксели ---
        for (int y = 0; y < 8; y++) {
            for (int x = 0; x < 8; x++) {
                if (hills[y][x]) {
                    // Берём цвет ноты
                    // WS2812_ShowNoteColor уже его выставил,
                    // поэтому просто оставляем пиксель включённым
                    // (ничего не делаем)
                } else {
                    Set_LED_XY(x, y, 0, 0, 0);
                }
            }
        }

        WS2812_Send();
    }

    else if (bt_ctx.led_mode == 2) {

        // --- 1. Сдвиг всей карты вправо ---
        for (int y = 0; y < 8; y++) {
            for (int x = 7; x > 0; x--) {
                hills[y][x] = hills[y][x - 1];
            }
            hills[y][0] = 0;
        }

        // --- 2. Новая "гора" слева ---
        uint8_t h = height_from_freq(player.current_freq);

        for (int y = 0; y < h; y++) {
            hills[7 - y][0] = 255;   // снизу вверх
        }

        // --- 3. Отрисовка с ВЕРТИКАЛЬНЫМ ГРАДИЕНТОМ ---
        WS2812_Clear();

        for (int y = 0; y < 8; y++) {
            for (int x = 0; x < 8; x++) {
                if (hills[y][x]) {

                    uint8_t r, g, b;
                    uint8_t gy = 7 - y;   // инверсия по вертикали
                    color_from_height_xy(gy, x, &r, &g, &b);



                    Set_LED_XY(x, y, r, g, b);
                }
            }
        }

        WS2812_Send();
    }

    else if (bt_ctx.led_mode == 3) {

        static uint8_t toggle = 0;

        if (toggle) {
            WS2812_Clear();
            WS2812_Send();
        } else {

            // 1. Сначала применяем цвет как в MODE 0
            WS2812_ShowNoteColor(player.current_freq);

            // 2. Маскируем всё, кроме букв RK
            for (int y = 0; y < 8; y++) {
                for (int x = 0; x < 8; x++) {

                    // Проверяем бит bitmap
                    if (!(RK_bitmap[y] & (1 << (7 - x)))) {
                        Set_LED_XY(x, y, 0, 0, 0);
                    }
                }
            }

            WS2812_Send();
        }

        toggle ^= 1;
    }

    // --- NEXT STEP ---
    player.step_time_left_ms = step->dur_ms;
    player.step_index++;
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
  MX_DMA_Init();
  MX_TIM1_Init();
  MX_USART2_UART_Init();
  MX_TIM2_Init();
  /* USER CODE BEGIN 2 */
  Speaker_Init(&htim2, TIM_CHANNEL_2);
  WS2812_Init();
  bt_init(&huart2, &bt_ctx);

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
      bt_process_rx();

      if (bt_has_new_command()) {

          if (bt_ctx.state == BT_STATE_PLAYING) {
              system_state = SYS_PLAYING;
              player.melody_id = bt_ctx.melody_id % MELODY_COUNT;
              player.step_index = 0;
              player.step_time_left_ms = 0;

              memset(hills, 0, sizeof(hills));

              // --- RESET LED WAVE ---
              memset(wave_buf, 0, sizeof(wave_buf));
              wave_phase = 0;
              wave_div = 0;
          }
          else {
              system_state = SYS_STOPPED;
              Speaker_Set_Tone(0, 0);
              player.current_freq = 0;
              WS2812_Clear();
              WS2812_Send();
          }

          bt_clear_new_command_flag();
      }
  }

}
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */

  /* USER CODE END 3 */


/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};


  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
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

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_TIM1;
  PeriphClkInit.Tim1ClockSelection = RCC_TIM1CLK_PLLCLK;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief TIM1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM1_Init(void)
{

  /* USER CODE BEGIN TIM1_Init 0 */

  /* USER CODE END TIM1_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};
  TIM_BreakDeadTimeConfigTypeDef sBreakDeadTimeConfig = {0};

  /* USER CODE BEGIN TIM1_Init 1 */

  /* USER CODE END TIM1_Init 1 */
  htim1.Instance = TIM1;
  htim1.Init.Prescaler = 0;
  htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim1.Init.Period = 89;
  htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim1.Init.RepetitionCounter = 0;
  htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
  if (HAL_TIM_Base_Init(&htim1) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim1, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_Init(&htim1) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterOutputTrigger2 = TIM_TRGO2_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim1, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCNPolarity = TIM_OCNPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  sConfigOC.OCIdleState = TIM_OCIDLESTATE_RESET;
  sConfigOC.OCNIdleState = TIM_OCNIDLESTATE_RESET;
  if (HAL_TIM_PWM_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  sBreakDeadTimeConfig.OffStateRunMode = TIM_OSSR_DISABLE;
  sBreakDeadTimeConfig.OffStateIDLEMode = TIM_OSSI_DISABLE;
  sBreakDeadTimeConfig.LockLevel = TIM_LOCKLEVEL_OFF;
  sBreakDeadTimeConfig.DeadTime = 0;
  sBreakDeadTimeConfig.BreakState = TIM_BREAK_DISABLE;
  sBreakDeadTimeConfig.BreakPolarity = TIM_BREAKPOLARITY_HIGH;
  sBreakDeadTimeConfig.BreakFilter = 0;
  sBreakDeadTimeConfig.Break2State = TIM_BREAK2_DISABLE;
  sBreakDeadTimeConfig.Break2Polarity = TIM_BREAK2POLARITY_HIGH;
  sBreakDeadTimeConfig.Break2Filter = 0;
  sBreakDeadTimeConfig.AutomaticOutput = TIM_AUTOMATICOUTPUT_DISABLE;
  if (HAL_TIMEx_ConfigBreakDeadTime(&htim1, &sBreakDeadTimeConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM1_Init 2 */

  /* USER CODE END TIM1_Init 2 */
  HAL_TIM_MspPostInit(&htim1);

}

/**
  * @brief TIM2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM2_Init(void)
{

  /* USER CODE BEGIN TIM2_Init 0 */

  /* USER CODE END TIM2_Init 0 */

  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};


  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 0;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 4294967295;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_PWM_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */
  HAL_TIM_MspPostInit(&htim2);

}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 9600;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  huart2.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart2.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA1_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA1_Channel2_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel2_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel2_IRQn);

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOA_CLK_ENABLE();

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
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
