
/////////////// Chapter 2 : RM Periodic 4-Task Experiment ///////////////////
#include <main_rm.h>

#include <string.h>
#include "main.h"
#include "uart_config.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include <stdio.h>

#if ENABLE_RM_PERODIC_1

/* =====================================================================
 * TASK CONFIGURATION SECTION
 * ===================================================================== */

/* Periods in milliseconds (converted to ticks via pdMS_TO_TICKS).
 * Tick rate = 100 Hz => 1 tick = 10 ms.
 */
#define T1_PERIOD_MS   (1000u)   // 1 s
#define T2_PERIOD_MS   (3000u)
#define T3_PERIOD_MS   (2000u)
#define T4_PERIOD_MS   (4000u)

#define T1_PERIOD      (pdMS_TO_TICKS(T1_PERIOD_MS))
#define T2_PERIOD      (pdMS_TO_TICKS(T2_PERIOD_MS))
#define T3_PERIOD      (pdMS_TO_TICKS(T3_PERIOD_MS))
#define T4_PERIOD      (pdMS_TO_TICKS(T4_PERIOD_MS))

/* Execution budgets in loop iterations.
 * These were calibrated on the STM32F091RC to give approx:
 *  T1: ~100 ms, T2: ~320 ms, T3: ~50 ms, T4: ~1150 ms.
 */
#define T1_BUDGET      (1333333u)
#define T2_BUDGET      (2000000u)
#define T3_BUDGET      (2000000u)
#define T4_BUDGET      (3333333u)

/* =====================================================================
 * TASK PARAMETER STRUCTURE
 * ===================================================================== */

typedef struct {
    char        *name_start;      // e.g., "S1 "
    char        *name_finish;     // e.g., "F1 "
    TickType_t   period;          // Ti in ticks
    uint32_t     execution_budget;// busy-wait loop count
    UBaseType_t  priority;        // assigned automatically (RM)
    TaskHandle_t handle;          // task handle (optional)
} TaskParams_t;

/* Initialize parameter array: only periods & budgets are fixed here.
 * Priorities will be assigned at runtime based on Ti (RM policy).
 */
static TaskParams_t taskParams[] = {
    { "S1 ", "F1 ", T1_PERIOD, T1_BUDGET, 0, NULL },
    { "S2 ", "F2 ", T2_PERIOD, T2_BUDGET, 0, NULL },
    { "S3 ", "F3 ", T3_PERIOD, T3_BUDGET, 0, NULL },
    { "S4 ", "F4 ", T4_PERIOD, T4_BUDGET, 0, NULL },
};

#define NUM_TASKS   (sizeof(taskParams) / sizeof(taskParams[0]))

/* Forward declarations */
static void vGenericPeriodicTask(void *pvParameters);
static void Simulate_Work(uint32_t loop_count);
static void sortTasksByPeriod(void);

/* =====================================================================
 * sortTasksByPeriod
 *  - Simple bubble sort so that taskParams[] is ordered by ascending
 *    period (shorter period first).
 * ===================================================================== */
static void sortTasksByPeriod(void)
{
    for (int i = 0; i < (int)NUM_TASKS - 1; ++i) {
        for (int j = 0; j < (int)NUM_TASKS - 1 - i; ++j) {
            if (taskParams[j].period > taskParams[j + 1].period) {
                TaskParams_t tmp      = taskParams[j];
                taskParams[j]         = taskParams[j + 1];
                taskParams[j + 1]     = tmp;
            }
        }
    }
}

/* =====================================================================
 * RM_Demo_Init
 *  - Called from main() after HAL + UART init.
 *  - Assigns RM priorities automatically and creates the four tasks.
 * ===================================================================== */
void RM_Demo_Init(void)
{
    HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);

    /* 1. Sort tasks by period (shortest period first) */
    sortTasksByPeriod();

    /* 2. Assign Rate-Monotonic priorities:
     *    highest = configMAX_PRIORITIES - 1, then descending.
     *    Assumes NUM_TASKS <= configMAX_PRIORITIES - 1.
     */
    UBaseType_t highest = configMAX_PRIORITIES - 1;

    for (int i = 0; i < (int)NUM_TASKS; ++i) {
        taskParams[i].priority = highest - i;
    }

    /* 3. Create the tasks using the computed priorities */
    for (int i = 0; i < (int)NUM_TASKS; ++i) {
        char name[8];
        snprintf(name, sizeof(name), "task%d", i + 1);

        xTaskCreate(
            vGenericPeriodicTask,
            name,
            configMINIMAL_STACK_SIZE,
            (void *)&taskParams[i],
            taskParams[i].priority,
            &taskParams[i].handle
        );
    }
}

/* =====================================================================
 * vGenericPeriodicTask
 *  - Generic handler for all 4 tasks.
 *  - Logs start (Sx) and finish (Fx) ticks and runs busy-wait workload.
 * ===================================================================== */
static void vGenericPeriodicTask(void *pvParameters)
{
    TaskParams_t *p = (TaskParams_t *)pvParameters;
    TickType_t xNextReleaseTime = xTaskGetTickCount();

    for (;;) {
        /* 1. Log start time */
        TickType_t startTick = xTaskGetTickCount();

        taskENTER_CRITICAL();
        uart_write_string(p->name_start);
        uart_write_uint32((uint32_t)startTick);
        taskEXIT_CRITICAL();

        /* 2. Run simulated execution (busy-wait) */
        HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_SET);
        Simulate_Work(p->execution_budget);
        HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);

        /* 3. Log finish time */
        TickType_t finishTick = xTaskGetTickCount();

        taskENTER_CRITICAL();
        uart_write_string(p->name_finish);
        uart_write_uint32((uint32_t)finishTick);
        taskEXIT_CRITICAL();

        /* 4. Wait for next release */
        vTaskDelayUntil(&xNextReleaseTime, p->period);
    }
}

/* =====================================================================
 * Simulate_Work
 *  - Simple busy-wait loop to approximate execution time C_i.
 * ===================================================================== */
static void Simulate_Work(uint32_t loop_count)
{
    volatile uint32_t i;
    for (i = 0; i < loop_count; i++) {
        __NOP();
    }
}

/* =====================================================================
 * Trace hook stubs
 *  - Not used in this experiment but kept for compatibility.
 * ===================================================================== */
void log_task_switched_in(char *taskName)      { (void)taskName; }
void log_task_switched_out(char *taskName)     { (void)taskName; }
void log_task_to_ready_state(char *taskName)   { (void)taskName; }

void update_task_tick_counts(char *taskName, TickType_t currentTick)
{
    (void)taskName;
    (void)currentTick;
    /* No per-task tick accounting needed for this chapter. */
}

#endif /* ENABLE_RM_PERODIC_1 */















///////////////////////////////////////2nd part ////////////////////////////
#if ENABLE_RM_PERODIC_2


///////////////////////////////////////////// CHAPTER 2 /////////////////////////////



/////////////////////2nd part////////////////////
/* --- Task 1 Parameters --- */
/* Periodic task: 1s period, ~400ms budget (40%) */
#define T1_PERIOD       (pdMS_TO_TICKS(1000))   // 100 ticks
#define T1_PRIORITY     (tskIDLE_PRIORITY + 4)  // Highest Priority
#define T1_BUDGET       (1333333)               // ~40 ticks

/* --- Task 2 Parameters --- */
/* Periodic task: 2s period, ~600ms budget (30%) */
#define T2_PERIOD       (pdMS_TO_TICKS(2000))   // 200 ticks
#define T2_PRIORITY     (tskIDLE_PRIORITY + 3)  // High Priority
#define T2_BUDGET       (2000000)               // ~60 ticks

/* --- Task 3 Parameters --- */
/* Periodic task: 3s period, ~600ms budget (20%) */
#define T3_PERIOD       (pdMS_TO_TICKS(3000))   // 300 ticks
#define T3_PRIORITY     (tskIDLE_PRIORITY + 2)  // Medium Priority
#define T3_BUDGET       (2000000)               // ~60 ticks

/* --- Task 4 Parameters --- */
/* Periodic task: 4s period, ~1s budget (25%) -> Total U = 1.15 */
#define T4_PERIOD       (pdMS_TO_TICKS(4000))   // 400 ticks
#define T4_PRIORITY     (tskIDLE_PRIORITY + 1)  // Lowest Priority
#define T4_BUDGET       (3333333)               // ~100 ticks


/* Task Handles */
static TaskHandle_t xTask1Handle = NULL;
static TaskHandle_t xTask2Handle = NULL;
static TaskHandle_t xTask3Handle = NULL;
static TaskHandle_t xTask4Handle = NULL;

/* Tick accounting globals */
TickType_t xTotalTickCount = 0;
TickType_t xTaskTick1 = 0;
TickType_t xTaskTick2 = 0;
TickType_t xTaskTick3 = 0;
TickType_t xTaskTick4 = 0;

/* Function Prototypes */
static void vGenericPeriodicTask(void *pvParameters);
static void Simulate_Work(uint32_t loop_count);

/* Structure to pass parameters to generic task */
typedef struct {
    char* name_start;           // e.g., "S1 "
    char* name_finish;          // e.g., "F1 "
    TickType_t period;
    uint32_t execution_budget;  // Renamed from 'work_load' to 'execution_budget'
} TaskParams_t;

/* Initialize Task Parameter Structures */
static TaskParams_t param1 = {"S1 ", "F1 ", T1_PERIOD, T1_BUDGET};
static TaskParams_t param2 = {"S2 ", "F2 ", T2_PERIOD, T2_BUDGET};
static TaskParams_t param3 = {"S3 ", "F3 ", T3_PERIOD, T3_BUDGET};
static TaskParams_t param4 = {"S4 ", "F4 ", T4_PERIOD, T4_BUDGET};

/* =====================================================================
 * RM_Demo_Init_code
 * - Creates 4 periodic tasks
 * ===================================================================== */
void RM_Demo_Init(void)
{
    HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);

    /* Create Task 1 (Highest Priority) */
    xTaskCreate(vGenericPeriodicTask, "task1", configMINIMAL_STACK_SIZE,
                (void*)&param1, T1_PRIORITY, &xTask1Handle);

    /* Create Task 2 */
    xTaskCreate(vGenericPeriodicTask, "task2", configMINIMAL_STACK_SIZE,
                (void*)&param2, T2_PRIORITY, &xTask2Handle);

    /* Create Task 3 */
    xTaskCreate(vGenericPeriodicTask, "task3", configMINIMAL_STACK_SIZE,
                (void*)&param3, T3_PRIORITY, &xTask3Handle);

    /* Create Task 4 (Lowest Priority) */
    xTaskCreate(vGenericPeriodicTask, "task4", configMINIMAL_STACK_SIZE,
                (void*)&param4, T4_PRIORITY, &xTask4Handle);
}

/* =====================================================================
 * vGenericPeriodicTask
 * - Generic handler for all 4 tasks to reduce code duplication
 * ===================================================================== */
static void vGenericPeriodicTask(void *pvParameters)
{
    TaskParams_t *p = (TaskParams_t *)pvParameters;
    TickType_t xNextReleaseTime;

    /* Initialize release time */
    xNextReleaseTime = xTaskGetTickCount();

    for (;;)
    {
        /* 1. Log Start Time (Arrival/Start) */
        TickType_t startTick = xTaskGetTickCount();

        /* Enter critical section for UART safety */
        taskENTER_CRITICAL();
        uart_write_string(p->name_start);
        uart_write_uint32((uint32_t)startTick);
        taskEXIT_CRITICAL();

        /* LED ON to indicate CPU usage */
        HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_SET);

        /* 2. Execute Simulated Work (C) */
        /* Using execution_budget to drive the loop */
        Simulate_Work(p->execution_budget);

        HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);

        /* 3. Log Finish Time */
        TickType_t finishTick = xTaskGetTickCount();

        taskENTER_CRITICAL();
        uart_write_string(p->name_finish);
        uart_write_uint32((uint32_t)finishTick);
        taskEXIT_CRITICAL();

        /* 4. Wait for next period (Implicit Deadline) */
        vTaskDelayUntil(&xNextReleaseTime, p->period);
    }
}

/* Helper to burn CPU cycles */
static void Simulate_Work(uint32_t loop_count)
{
    volatile uint32_t i;
    for (i = 0; i < loop_count; i++)
    {
        __NOP();
    }
}

/* =====================================================================
 * Trace hooks
 * ===================================================================== */
void log_task_switched_in(char *taskName) { (void)taskName; }
void log_task_switched_out(char *taskName) { (void)taskName; }
void log_task_to_ready_state(char *taskName) { (void)taskName; }

void update_task_tick_counts(char *taskName, TickType_t currentTick)
{
    TickType_t diff = currentTick - xTotalTickCount;

    if (strcmp(taskName, "task1") == 0) xTaskTick1 += diff;
    else if (strcmp(taskName, "task2") == 0) xTaskTick2 += diff;
    else if (strcmp(taskName, "task3") == 0) xTaskTick3 += diff;
    else if (strcmp(taskName, "task4") == 0) xTaskTick4 += diff;

    xTotalTickCount = currentTick;
}
#endif

