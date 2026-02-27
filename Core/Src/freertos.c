/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
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
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"


/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
extern UART_HandleTypeDef huart2;

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
/* USER CODE BEGIN Variables */
osThreadId ledTaskHandle;

/* USER CODE END Variables */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

/* GetIdleTaskMemory prototype (linked to static allocation support) */
void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize );

/* USER CODE BEGIN GET_IDLE_TASK_MEMORY */
static StaticTask_t xIdleTaskTCBBuffer;
static StackType_t xIdleStack[configMINIMAL_STACK_SIZE];

void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize )
{
  *ppxIdleTaskTCBBuffer = &xIdleTaskTCBBuffer;
  *ppxIdleTaskStackBuffer = &xIdleStack[0];
  *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
  /* place for user code */
}
/* USER CODE END GET_IDLE_TASK_MEMORY */

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

#include "main.h"
#include <string.h>

extern UART_HandleTypeDef huart2;

static TaskHandle_t blinkTaskHandle = NULL;
static TaskHandle_t uartTaskHandle  = NULL;

static void StartBlinkTask(void *argument);
static void StartUartTask(void *argument);

void MX_FREERTOS_Init(void)
{
  // LED blink task
  xTaskCreate(
      StartBlinkTask,
      "BlinkTask",
      128,
      NULL,
      tskIDLE_PRIORITY + 1,
      &blinkTaskHandle);

  // UART print task
  xTaskCreate(
      StartUartTask,
      "UartTask",
      256,                      // a bit more stack for printf/strings
      NULL,
      tskIDLE_PRIORITY + 1,
      &uartTaskHandle);
}

static void StartBlinkTask(void *argument)
{
  (void)argument;

  for(;;)
  {
    HAL_GPIO_TogglePin(LD2_GPIO_Port, LD2_Pin);
    vTaskDelay(pdMS_TO_TICKS(500));
  }
}

static void StartUartTask(void *argument)
{
  (void)argument;

  const char *msg = "Hello from FreeRTOS UART!\r\n";

  for(;;)
  {
    HAL_UART_Transmit(&huart2, (uint8_t*)msg, strlen(msg), HAL_MAX_DELAY);
    uart_write_string("UART wrapper works!\r\n");
    vTaskDelay(pdMS_TO_TICKS(1000));   // every 1 second
  }
}

void vApplicationTickHook(void)
{
  // For now, do nothing.
  // Later we could use this to do extra tick-based actions if needed.
}

/* USER CODE END Application */

