#ifndef MAIN_APP_EDF_H_
#define MAIN_APP_EDF_H_

/* Set to 1 to enable the EDF Demo definitions */
#define ENABLE_EDF_DEMO 0
#define SCHEDULABLE 0
#define NON_SCHEDULABLE 0

#include "FreeRTOS.h"
#include "task.h"

#if ENABLE_EDF_DEMO

/* Init function – call this from main.c instead of StartSporadic_Demo */
void StartEDF_Demo(void);

/* Tick accounting variables */
extern TickType_t xTotalTickCount;
extern TickType_t xTaskTick1;
extern TickType_t xTaskTick2;
extern TickType_t xTaskTick3;
extern TickType_t xTaskTick4;

/* Trace hooks (called by FreeRTOS macros) */
void log_task_switched_in(char *taskName);
void log_task_switched_out(char *taskName);
void log_task_to_ready_state(char *taskName);
void update_task_tick_counts(char *taskName, TickType_t currentTick);

#endif /* ENABLE_EDF_DEMO */

#endif /* MAIN_APP_EDF_H_ */
