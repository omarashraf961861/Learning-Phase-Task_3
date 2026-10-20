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
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* CAN message */
#define CAN_MSG_STD_ID     0x123U
#define CAN_MSG_STRING     "ASURT_LV"
#define CAN_MSG_LEN        (sizeof(CAN_MSG_STRING) - 1U)

/* Button */
#define BTN_PORT           GPIOA
#define BTN_PIN            GPIO_PIN_0
#define BTN_PRESSED_LEVEL  GPIO_PIN_RESET
#define DEBOUNCE_MS        30U

/* LED */
#define LED_PORT           GPIOC
#define LED_PIN            GPIO_PIN_13
#define LED_ON()           HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_RESET)
#define LED_OFF()          HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_SET)


#define TX_TIMEOUT_MS      20U

_Static_assert((sizeof(CAN_MSG_STRING) - 1U) <= 8U,
               "CAN_MSG_STRING must be 8 characters or less");
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
CAN_HandleTypeDef hcan;

/* USER CODE BEGIN PV */
/* frame */
static CAN_TxHeaderTypeDef txHeader;
static uint8_t             txData[8];
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_CAN_Init(void);
/* USER CODE BEGIN PFP */
static bool              Button_IsDown(void);
static void              CAN_Tx_Init(void);
static HAL_StatusTypeDef CAN_SendString(void);
static void              LED_Blink(uint8_t count, uint32_t on_ms, uint32_t off_ms);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

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

  /* clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize */
  MX_GPIO_Init();
  MX_CAN_Init();
  /* USER CODE BEGIN 2 */
  CAN_Tx_Init();

  /* Button state */
  bool     lastDown     = Button_IsDown();
  bool     stableDown   = lastDown;
  uint32_t lastChangeMs = HAL_GetTick();
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    bool down = Button_IsDown();


    if (down != lastDown)
    {
      lastDown     = down;
      lastChangeMs = HAL_GetTick();
    }

    if ((down != stableDown) &&
        ((HAL_GetTick() - lastChangeMs) >= DEBOUNCE_MS))
    {
      stableDown = down;

      if (stableDown)
      {
        if (CAN_SendString() == HAL_OK)
        {
          LED_Blink(1U, 40U, 1U);
        }
        else
        {
          LED_Blink(5U, 60U, 60U);
        }
      }
    }
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  *        8 MHz HSE -> PLL x9 -> SYSCLK 72 MHz
  *        AHB = 72 MHz, APB1 = 36 MHz (CAN clock), APB2 = 72 MHz
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

/**
  * @brief CAN Initialization Function
  *        500 kbit/s: 36 MHz / (4 x (1 + 15 + 2)) = 500 kHz, sample point ~88.9 %
  * @param None
  * @retval None
  */
static void MX_CAN_Init(void)
{

  /* USER CODE BEGIN CAN_Init 0 */

  /* USER CODE END CAN_Init 0 */

  /* USER CODE BEGIN CAN_Init 1 */

  /* USER CODE END CAN_Init 1 */
  hcan.Instance = CAN1;
  hcan.Init.Prescaler = 4;
  hcan.Init.Mode = CAN_MODE_NORMAL;
  hcan.Init.SyncJumpWidth = CAN_SJW_1TQ;
  hcan.Init.TimeSeg1 = CAN_BS1_15TQ;
  hcan.Init.TimeSeg2 = CAN_BS2_2TQ;
  hcan.Init.TimeTriggeredMode = DISABLE;
  hcan.Init.AutoBusOff = ENABLE;
  hcan.Init.AutoWakeUp = DISABLE;
  hcan.Init.AutoRetransmission = ENABLE;
  hcan.Init.ReceiveFifoLocked = DISABLE;
  hcan.Init.TransmitFifoPriority = DISABLE;
  if (HAL_CAN_Init(&hcan) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN CAN_Init 2 */

  /* USER CODE END CAN_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  *        PC13 = LED output (starts HIGH = off), PA0 = button input with pull-up
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);

  /*Configure GPIO pin : PC13 */
  GPIO_InitStruct.Pin = GPIO_PIN_13;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pin : PA0 */
  GPIO_InitStruct.Pin = GPIO_PIN_0;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
}

/* USER CODE BEGIN 4 */

/**
  * @brief  True while the button is held down (raw, not debounced).
  */
static bool Button_IsDown(void)
{
  return (HAL_GPIO_ReadPin(BTN_PORT, BTN_PIN) == BTN_PRESSED_LEVEL);
}

/**
  * @brief  Fill in the TX header and payload, then start CAN.
  *         Node A only sends, so there's no RX filter.
  */
static void CAN_Tx_Init(void)
{
  txHeader.StdId              = CAN_MSG_STD_ID;
  txHeader.ExtId              = 0U;
  txHeader.IDE                = CAN_ID_STD;
  txHeader.RTR                = CAN_RTR_DATA;
  txHeader.DLC                = CAN_MSG_LEN;
  txHeader.TransmitGlobalTime = DISABLE;

  memset(txData, 0, sizeof(txData));
  memcpy(txData, CAN_MSG_STRING, CAN_MSG_LEN);

  if (HAL_CAN_Start(&hcan) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  Queue the frame
  */
static HAL_StatusTypeDef CAN_SendString(void)
{
  uint32_t mailbox;
  uint32_t t0;

  if (HAL_CAN_GetTxMailboxesFreeLevel(&hcan) == 0U)
  {
    return HAL_BUSY;
  }

  if (HAL_CAN_AddTxMessage(&hcan, &txHeader, txData, &mailbox) != HAL_OK)
  {
    return HAL_ERROR;
  }

  /* The mailbox empties */
  t0 = HAL_GetTick();
  while (HAL_CAN_IsTxMessagePending(&hcan, mailbox) != 0U)
  {
    if ((HAL_GetTick() - t0) > TX_TIMEOUT_MS)
    {
      (void)HAL_CAN_AbortTxRequest(&hcan, mailbox);
      return HAL_TIMEOUT;
    }
  }

  return HAL_OK;
}

/*  Blink the  LED
  */
static void LED_Blink(uint8_t count, uint32_t on_ms, uint32_t off_ms)
{
  while (count > 0U)
  {
    LED_ON();
    HAL_Delay(on_ms);
    LED_OFF();
    HAL_Delay(off_ms);
    count--;
  }
}

/* USER CODE END 4 */

/*
 * Error handler
  */
void Error_Handler(void)
{

  GPIO_InitTypeDef gpio = {0};

  __disable_irq();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  gpio.Pin   = GPIO_PIN_13;
  gpio.Mode  = GPIO_MODE_OUTPUT_PP;
  gpio.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &gpio);

  while (1)
  {
    HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
    for (volatile uint32_t i = 0U; i < 400000U; i++)
    {
      __NOP();
    }
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
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
