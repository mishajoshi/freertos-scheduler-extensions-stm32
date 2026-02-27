#include <string.h>
#include "main_rm.h"

#include "main.h"          // LD2_GPIO_Port / LD2_Pin
#include "uart_config.h"


#include "FreeRTOS.h"
#include "task.h"
#include <stdio.h>
#if CHAPTER_1
/* --- Lab-style parameters --- */
/* Periodic task: 5s period, ~1s execution budget (conceptually) */
#define RM_TASK1_PERIOD    (pdMS_TO_TICKS(5000))   // 5 seconds
#define RM_TASK1_BUDGET    (pdMS_TO_TICKS(1000))   // not used directly here, but kept for clarity
#define RM_TASK1_PRIORITY  (tskIDLE_PRIORITY + 1)

/* Handle for the RM task */
static TaskHandle_t xTask1Handle = NULL;

/* Tick accounting (for trace hooks / future extensions) */
TickType_t xTotalTickCount = 0;
TickType_t xTaskTick1      = 0;
TickType_t xTaskTick2      = 0;
TickType_t xTaskTick3      = 0;
TickType_t xTaskTick4      = 0;

/* Forward declaration of the task */
static void vRMTask1(void *pvParameters);

/* =====================================================================
 *  RM_Demo_Init
 *  - Called from main() after UART + GPIO init
 *  - Creates the periodic RM task
 * ===================================================================== */
void RM_Demo_Init(void)
{
    /* Ensure LED is OFF initially */
    HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);

    /* UART is already initialized & bound via uart_init_from_hal(&huart2) in main.c */

    xTaskCreate(
        vRMTask1,
        "task1",                    // name used in trace hooks
        configMINIMAL_STACK_SIZE,
        NULL,
        RM_TASK1_PRIORITY,
        &xTask1Handle
    );
}

/* =====================================================================
 *  vRMTask1
 *  - Simple periodic task
 *  - Logs start tick (S1) and finish tick (F1) to UART each job
 *  - Uses vTaskDelayUntil for precise periodic releases
 * ===================================================================== */
static void vRMTask1(void *pvParameters)
{
    (void) pvParameters;

    TickType_t xNextReleaseTime;
    const TickType_t xPeriod = RM_TASK1_PERIOD;   // 5s period (change if you want faster output)

    /* First release time = now */
    xNextReleaseTime = xTaskGetTickCount();

    for (;;)
    {
        /* --- Job release: log S1 --- */
        TickType_t startTick = xTaskGetTickCount();
        uart_write_string("S1 ");
        uart_write_uint32((uint32_t)startTick);   // prints "S1 <tick>\r\n"

        /* Turn LED ON to show task running */
        HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_SET);

        /* --- Simulated execution time (C1) ---
         * Adjust loop iterations if needed to approx your 1s budget.
         * For the lab screenshot, exact duration is not critical.
         */
        volatile uint32_t i;
        for (i = 0; i < 50000; i++)
        {
            __NOP();
        }

        /* Turn LED OFF when job "finishes" */
        HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);

        /* --- Job finish: log F1 --- */
        TickType_t finishTick = xTaskGetTickCount();
        uart_write_string("F1 ");
        uart_write_uint32((uint32_t)finishTick);

        /* --- Wait until next periodic release --- */
        vTaskDelayUntil(&xNextReleaseTime, xPeriod);
    }
}

/* =====================================================================
 *  Trace hook helper functions
 *  - These are called from FreeRTOS trace macros defined in FreeRTOS.h
 *  - We keep them minimal (no LCD), just tick accounting
 * ===================================================================== */

void log_task_switched_in(char *taskName)
{
    (void)taskName;
    /* Optional: add UART debug if you want to see task switches */
}

void log_task_switched_out(char *taskName)
{
    (void)taskName;
    /* Optional: add UART debug here too */
}

void log_task_to_ready_state(char *taskName)
{
    (void)taskName;
    /* Not used right now, but required by trace macro */
}

/* Called every tick via traceTASK_INCREMENT_TICK in FreeRTOS.h */
void update_task_tick_counts(char *taskName, TickType_t currentTick)
{
    /* Accumulate execution time per task based on tick difference */
    if (strcmp(taskName, "task1") == 0)
        xTaskTick1 += (currentTick - xTotalTickCount);
    else if (strcmp(taskName, "task2") == 0)
        xTaskTick2 += (currentTick - xTotalTickCount);
    else if (strcmp(taskName, "task3") == 0)
        xTaskTick3 += (currentTick - xTotalTickCount);
    else if (strcmp(taskName, "task4") == 0)
        xTaskTick4 += (currentTick - xTotalTickCount);

    xTotalTickCount = currentTick;
}
#endif

