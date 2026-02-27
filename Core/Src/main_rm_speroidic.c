#include "main.h"
#include "cmsis_os.h"
#include "FreeRTOS.h"
#include "task.h"
#include <stdio.h>
#include <string.h>
#include "main_rm_sp.h"

#if ENABLE_SPORADIC_DEMO

/* --- STM32 Hardware Handles --- */
extern UART_HandleTypeDef huart2;

/* --- GPIO Configuration --- */
/* Button 1: PC13 (Blue User Button). Ensure configured as GPIO_EXTI13 in CubeMX */
#define BUTTON1_PIN GPIO_PIN_13

/* --- Task Parameters --- */
/* --- Task Parameters (Corrected for RMS) --- */
/* Rule: Shorter Period = Higher Priority */

/* P2 is the FASTEST (4s), so it gets HIGHEST Priority */
#define P2_PERIOD   pdMS_TO_TICKS(4000)
#define P2_BUDGET   (3333330)               // ~1.0 second execution (25% load)
#define P2_PRIO     (tskIDLE_PRIORITY + 4)  // HIGHEST

/* P1 is SLOWER (6s), so it gets LOWER Priority */
#define P1_PERIOD   pdMS_TO_TICKS(6000)
#define P1_BUDGET   (5000000)               // ~1.5 second execution (25% load)
#define P1_PRIO     (tskIDLE_PRIORITY + 3)  // LOWER

/* Sporadic Tasks (Lowest Priority background work) */
/* S1 (4s deadline) > S2 (5s deadline) logic applied here too */
#define S1_BUDGET   (1333333)               // ~400ms
#define S1_PRIO     (tskIDLE_PRIORITY + 2)

#define S2_BUDGET   (1666666)               // ~500ms
#define S2_PRIO     (tskIDLE_PRIORITY + 1)

/* --- Task Handles --- */
TaskHandle_t xP1Handle = NULL;
TaskHandle_t xP2Handle = NULL;
TaskHandle_t xS1Handle = NULL;
TaskHandle_t xS2Handle = NULL;

/* --- Global Tick Tracking --- */
TickType_t xTotalTickCount = 0;
TickType_t xTaskTickP1 = 0;
TickType_t xTaskTickP2 = 0;
TickType_t xTaskTickS1 = 0;
TickType_t xTaskTickS2 = 0;

/* --- Function Prototypes --- */
void vPeriodicTask(void *pvParameters);
void vSporadicTask(void *pvParameters);
void UART_Log(char* msg, uint32_t val);
void Simulate_Work(uint32_t loops);

typedef struct {
    char* name_start;
    char* name_finish;
    TickType_t period;
    uint32_t budget;
} TaskParams_t;

/* Definitions of task parameters */
TaskParams_t paramsP1 = {"S_P1 ", "F_P1 ", P1_PERIOD, P1_BUDGET};
TaskParams_t paramsP2 = {"S_P2 ", "F_P2 ", P2_PERIOD, P2_BUDGET};
TaskParams_t paramsS1 = {"S_S1 ", "F_S1 ", 0,         S1_BUDGET};
TaskParams_t paramsS2 = {"S_S2 ", "F_S2 ", 0,         S2_BUDGET};

/* =====================================================================
 * Main Application Init
 * ===================================================================== */
void StartSporadic_Demo(void)
{
    /* 1. SAFETY: Force EXTI Interrupts to Lowest Priority to prevent FreeRTOS crashes */
    #if defined(EXTI4_15_IRQn)
        HAL_NVIC_SetPriority(EXTI4_15_IRQn, 3, 0);
    #endif

    #if defined(EXTI15_10_IRQn)
        HAL_NVIC_SetPriority(EXTI15_10_IRQn, 15, 0);
    #endif

    /* 2. Create Tasks (Stack size 512 words = 2KB to prevent Overflow) */
    BaseType_t status = pdPASS;
    status &= xTaskCreate(vPeriodicTask, "task1", 512, &paramsP1, P1_PRIO, &xP1Handle);
    status &= xTaskCreate(vPeriodicTask, "task2", 512, &paramsP2, P2_PRIO, &xP2Handle);
    status &= xTaskCreate(vSporadicTask, "sporadictask1", 512, &paramsS1, S1_PRIO, &xS1Handle);
    status &= xTaskCreate(vSporadicTask, "sporadictask2", 512, &paramsS2, S2_PRIO, &xS2Handle);

    /* ERROR CHECK: If heap is too small, tasks won't create. Blink LED forever. */
    /* If you see this blinking, increase configTOTAL_HEAP_SIZE in FreeRTOSConfig.h to 15360 (15KB) */
    if (status != pdPASS) {
        while(1) {
            HAL_GPIO_TogglePin(LD2_GPIO_Port, LD2_Pin);
            for(int i=0; i<100000; i++) __NOP(); // Fast blink
        }
    }

    /* 3. Suspend Sporadic tasks so they wait for button */
    if (xS1Handle != NULL) vTaskSuspend(xS1Handle);
    if (xS2Handle != NULL) vTaskSuspend(xS2Handle);
}

/* =====================================================================
 * Periodic Task Function
 * ===================================================================== */
void vPeriodicTask(void *pvParameters)
{
    TaskParams_t *p = (TaskParams_t *)pvParameters;
    TickType_t xLastWakeTime = xTaskGetTickCount();

    for (;;)
    {
        UART_Log(p->name_start, xTaskGetTickCount());

        HAL_GPIO_TogglePin(LD2_GPIO_Port, LD2_Pin);
        Simulate_Work(p->budget);

        UART_Log(p->name_finish, xTaskGetTickCount());

        vTaskDelayUntil(&xLastWakeTime, p->period);
    }
}

/* =====================================================================
 * Sporadic Task Function
 * ===================================================================== */
void vSporadicTask(void *pvParameters)
{
    TaskParams_t *p = (TaskParams_t *)pvParameters;

    for (;;)
    {
        /* 1. Runs immediately after Button Interrupt Resumes it */
        UART_Log(p->name_start, xTaskGetTickCount());

        Simulate_Work(p->budget);

        UART_Log(p->name_finish, xTaskGetTickCount());

        /* 2. Suspends itself to wait for next button press */
        vTaskSuspend(NULL);
    }
}

/* =====================================================================
 * Interrupt Handler (Alternating Logic)
 * ===================================================================== */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    static uint8_t task_toggle = 0;

    /* DEBOUNCE: Ignore interrupts if they happen faster than 200ms */
    static uint32_t last_press_tick = 0;

    TickType_t current_tick = xTaskGetTickCountFromISR();
    if ((current_tick - last_press_tick) < pdMS_TO_TICKS(200)) {
        return; // Too soon, ignore bounce
    }
    last_press_tick = current_tick;

    if (GPIO_Pin == BUTTON1_PIN)
    {
        /* Check if tasks are initialized */
        if (xS1Handle == NULL || xS2Handle == NULL) return;

        if (task_toggle == 0)
        {
            /* Release Sporadic Task 1 */
            if (xTaskResumeFromISR(xS1Handle) == pdTRUE) {
                xHigherPriorityTaskWoken = pdTRUE;
            }
            task_toggle = 1;
        }
        else
        {
            /* Release Sporadic Task 2 */
            if (xTaskResumeFromISR(xS2Handle) == pdTRUE) {
                xHigherPriorityTaskWoken = pdTRUE;
            }
            task_toggle = 0;
        }
    }

    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

/* =====================================================================
 * Helper Functions
 * ===================================================================== */
void Simulate_Work(uint32_t loops)
{
    volatile uint32_t i;
    for (i = 0; i < loops; i++) {
        __NOP();
    }
}

void UART_Log(char* msg, uint32_t val)
{
    char buf[32];
    sprintf(buf, "%s %lu\r\n", msg, val);

    /* PROTECT UART: Prevent tasks from interrupting each other while printing */
    taskENTER_CRITICAL();
    HAL_UART_Transmit(&huart2, (uint8_t*)buf, strlen(buf), 100);
    taskEXIT_CRITICAL();
}

void update_task_tick_counts(char * taskName, TickType_t currentTick) { (void)taskName; (void)currentTick; }
void log_task_switched_in(char *taskName) { (void)taskName; }
void log_task_switched_out(char *taskName) { (void)taskName; }
void log_task_to_ready_state(char *taskName) { (void)taskName; }
#endif
