#include "main_edf.h"
#include "main.h"
#include "cmsis_os.h"
#include <stdio.h>
#include <string.h>

#if ENABLE_EDF_DEMO

/* --- STM32 Hardware Handles --- */
extern UART_HandleTypeDef huart2;

/* --- Task Parameters (Chapter 3 Lab Requirements) --- */
/* Note: 1 Tick = 10ms */

#if SCHEDULABLE
/* Task 1: Period 1000ms (100 ticks), Exec ~200ms */
#define T1_PERIOD   pdMS_TO_TICKS(40000)
#define T1_EXEC     (600000)

/* Task 2: Period 1500ms (150 ticks), Exec ~500ms */
#define T2_PERIOD   pdMS_TO_TICKS(15000)
#define T2_EXEC     (120000)

/* Task 3: Period 3000ms (300 ticks), Exec ~500ms */
#define T3_PERIOD   pdMS_TO_TICKS(30000)
#define T3_EXEC     (180000)

/* Task 4: Period 4000ms (400 ticks), Exec ~1000ms */
#define T4_PERIOD   pdMS_TO_TICKS(10000)
#define T4_EXEC     (240000)

#endif

#if NON_SCHEDULABLE
/* Task 1: Period 1000ms (100 ticks), Exec ~200ms */
#define T1_PERIOD   pdMS_TO_TICKS(40000)
#define T1_EXEC     (600000000)

/* Task 2: Period 1500ms (150 ticks), Exec ~500ms */
#define T2_PERIOD   pdMS_TO_TICKS(15000)
#define T2_EXEC     (120000)

/* Task 3: Period 3000ms (300 ticks), Exec ~500ms */
#define T3_PERIOD   pdMS_TO_TICKS(30000)
#define T3_EXEC     (180000)

/* Task 4: Period 4000ms (400 ticks), Exec ~1000ms */
#define T4_PERIOD   pdMS_TO_TICKS(10000)
#define T4_EXEC     (240000)

#endif

/* --- Task Handles --- */
TaskHandle_t xHandle1, xHandle2, xHandle3, xHandle4;

/* --- Global Tick Tracking --- */
TickType_t xTotalTickCount = 0;
TickType_t xTaskTick1 = 0;
TickType_t xTaskTick2 = 0;
TickType_t xTaskTick3 = 0;
TickType_t xTaskTick4 = 0;

/* --- Helpers --- */
void UART_Log_EDF(char* msg, uint32_t val)
{
    char buf[32];
    sprintf(buf, "%s %lu\r\n", msg, val);

    /* SAFE LOGGING: Use SuspendAll instead of Critical Section.
     * Critical Section disables interrupts (SysTick), which can hang HAL_UART_Transmit.
     * SuspendAll just stops the scheduler, which is safer for UART. */
    vTaskSuspendAll();
    HAL_UART_Transmit(&huart2, (uint8_t*)buf, strlen(buf), 100);
    xTaskResumeAll();
}

void Simulate_Work_EDF(uint32_t loops)
{
    volatile uint32_t i;
    for (i = 0; i < loops; i++) { __NOP(); }
}

/* --- Generic Periodic Task for EDF --- */
void vEDFTask(void *pvParameters)
{
    /* We pass the execution loops as the parameter */
    uint32_t exec_loops = (uint32_t)pvParameters;

    char *pcTaskName = pcTaskGetName(NULL);
    char startMsg[8];
    char endMsg[8];

    // Create "S1 ", "F1 " strings based on task name
    // Assumes names are "task1", "task2" etc.
    sprintf(startMsg, "S%c ", pcTaskName[4]);
    sprintf(endMsg, "F%c ", pcTaskName[4]);

    /* Determine Period based on loops (Hack for generic function logic) */
    TickType_t myPeriod = 0;
    /* Explicitly cast macros to uint32_t to ensure comparison works */
    if(exec_loops == (uint32_t)T1_EXEC) myPeriod = T1_PERIOD;
    else if(exec_loops == (uint32_t)T2_EXEC) myPeriod = T2_PERIOD;
    else if(exec_loops == (uint32_t)T3_EXEC) myPeriod = T3_PERIOD;
    else if(exec_loops == (uint32_t)T4_EXEC) myPeriod = T4_PERIOD;

    /* Initialise xNextWakeTime */
    TickType_t xNextWakeTime = xTaskGetTickCount();

    for (;;)
    {
        UART_Log_EDF(startMsg, xTaskGetTickCount());
        HAL_GPIO_TogglePin(LD2_GPIO_Port, LD2_Pin);

        Simulate_Work_EDF(exec_loops);

        UART_Log_EDF(endMsg, xTaskGetTickCount());

        /* * EDF MAGIC HAPPENS HERE:
         * When we call vTaskDelayUntil, the task blocks.
         * When it wakes up, the kernel (xTaskIncrementTick) will calculate:
         * New Deadline = Current Time + xTaskPeriod (stored in TCB)
         * And insert it into the ReadyListEDF sorted by that deadline.
         */
        vTaskDelayUntil(&xNextWakeTime, myPeriod);
    }
}

/* --- Main Init --- */
void StartEDF_Demo(void)
{
    /* IMPORTANT: You must have implemented xTaskPeriodicCreate in tasks.c
     * If you updated task.h correctly, this extern is not strictly needed,
     * but it ensures the compiler finds it even if headers are tricky.
     */
    extern BaseType_t xTaskPeriodicCreate( TaskFunction_t pxTaskCode,
                                    const char * const pcName,
                                    const configSTACK_DEPTH_TYPE usStackDepth,
                                    void * const pvParameters,
                                    UBaseType_t uxPriority,
                                    TaskHandle_t * const pxCreatedTask,
                                    TickType_t xPeriod );

    /* DEBUG: Print startup message to verify UART/Main linkage */
    UART_Log_EDF("EDF Demo Initializing...", 0);

    BaseType_t status = pdPASS;

    /* Create Tasks using the EDF API */
    /* Priority (5th arg) is ignored by EDF logic, but we pass 1 */

    status &= xTaskPeriodicCreate(vEDFTask, "task1", 512, (void*)T1_EXEC, 1, &xHandle1, T1_PERIOD);
    status &= xTaskPeriodicCreate(vEDFTask, "task2", 512, (void*)T2_EXEC, 1, &xHandle2, T2_PERIOD);
    status &= xTaskPeriodicCreate(vEDFTask, "task3", 512, (void*)T3_EXEC, 1, &xHandle3, T3_PERIOD);
    status &= xTaskPeriodicCreate(vEDFTask, "task4", 512, (void*)T4_EXEC, 1, &xHandle4, T4_PERIOD);

    /* Heap Check - Blink Rapidly if Creation Fails */
    if (status != pdPASS) {
        UART_Log_EDF("ERROR: Heap Full!", 0);
        while(1) {
            HAL_GPIO_TogglePin(LD2_GPIO_Port, LD2_Pin);
            for(volatile int i=0; i<50000; i++);
        }
    }
}

/* --- Trace Hooks --- */
void update_task_tick_counts(char * taskName, TickType_t currentTick) { (void)taskName; (void)currentTick; }
void log_task_switched_in(char *taskName) { (void)taskName; }
void log_task_switched_out(char *taskName) { (void)taskName; }
void log_task_to_ready_state(char *taskName) { (void)taskName; }

#endif /* ENABLE_EDF_DEMO */
