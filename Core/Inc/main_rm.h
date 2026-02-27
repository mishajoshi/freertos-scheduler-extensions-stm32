#ifndef RM_DEMO_H_
#define RM_DEMO_H_

/* Enable switches for each RM demo */
#define CHAPTER_1            1
#define ENABLE_RM_PERODIC_1  0
#define ENABLE_RM_PERODIC_2  0
#include "FreeRTOS.h"
#include "task.h"

/* ============================================================
 *  Common declarations used by all RM demos
 * ============================================================ */

/* Tick accounting variables (used by trace hook) */
extern TickType_t xTotalTickCount;
extern TickType_t xTaskTick1;
extern TickType_t xTaskTick2;
extern TickType_t xTaskTick3;
extern TickType_t xTaskTick4;

/* Functions used by trace macros */
void log_task_switched_in(char *taskName);
void log_task_switched_out(char *taskName);
void log_task_to_ready_state(char *taskName);
void update_task_tick_counts(char *taskName, TickType_t currentTick);

/* ============================================================
 *  RM DEMO 1
 * ============================================================ */
#if CHAPTER_1

/* Init function – we’ll call this before starting the scheduler */
void RM_Demo_Init(void);

#endif /* CHAPTER_1 */

/* ============================================================
 *  RM DEMO 2
 * ============================================================ */
#if ENABLE_RM_PERODIC_1

/* Init function – we’ll call this before starting the scheduler */
void RM_Demo_Init(void);

#endif /* ENABLE_RM_PERODIC_1 */

/* ============================================================
 *  RM DEMO 3
 * ============================================================ */
#if ENABLE_RM_PERODIC_2

/* Init function – we’ll call this before starting the scheduler */
void RM_Demo_Init(void);

#endif /* ENABLE_RM_PERODIC_2 */

#endif /* RM_DEMO_H_ */
