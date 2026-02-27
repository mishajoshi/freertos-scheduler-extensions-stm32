#ifndef MAIN_APP_SPORADIC_H_
#define MAIN_APP_SPORADIC_H_

/* Set to 1 to enable the Sporadic Demo definitions */
#define ENABLE_SPORADIC_DEMO  0

#include "FreeRTOS.h"
#include "task.h"

#if ENABLE_SPORADIC_DEMO

/* Init function – we’ll call this from main.c before starting the scheduler */
void StartSporadic_Demo(void);

/* Tick accounting variables (used by trace hook) */
/* These must match the variables defined in main_app_sporadic.c */
extern TickType_t xTotalTickCount;
extern TickType_t xTaskTickP1;
extern TickType_t xTaskTickP2;
extern TickType_t xTaskTickS1;
extern TickType_t xTaskTickS2;

/* Functions used by trace macros (defined in FreeRTOSConfig.h) */
void log_task_switched_in(char *taskName);
void log_task_switched_out(char *taskName);
void log_task_to_ready_state(char *taskName);
void update_task_tick_counts(char *taskName, TickType_t currentTick);

#endif /* ENABLE_SPORADIC_DEMO */

#endif /* MAIN_APP_SPORADIC_H_ */
