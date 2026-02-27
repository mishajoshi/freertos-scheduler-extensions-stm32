# FreeRTOS Kernel Extensions – EDF & Sporadic Scheduling (STM32F091RC)

## Overview

This project extends the FreeRTOS kernel to support multiple real-time scheduling policies beyond the default fixed-priority scheduler. The system was implemented and validated on an STM32F091RC (ARM Cortex-M0).

Implemented scheduling policies:

- Fixed Priority (Rate Monotonic Scheduling)
- Earliest Deadline First (EDF)
- Fixed Priority with Sporadic Tasks (ISR-triggered release)

The project required direct modification of FreeRTOS kernel internals, including task management structures and ready list handling.

---

## Platform

- MCU: STM32F091RC (ARM Cortex-M0)
- RTOS: FreeRTOS (kernel modified)
- Toolchain: STM32CubeIDE
- Architecture: Single-core, preemptive scheduling

---

## Kernel Modifications

The following FreeRTOS core files were modified:

- `Middlewares/FreeRTOS/Source/tasks.c`
- `Middlewares/FreeRTOS/Source/include/tasks.h`
- `Middlewares/FreeRTOS/Source/include/FreeRTOS.h`
- `Core/Inc/FreeRTOSConfig.h`

Additional files added:

- `Core/Src/main_edf.c`
- `Core/Inc/main_edf.h`
- `Core/Src/main_rm.c`
- `Core/Inc/main_rm.h`
- `Core/Src/main_rm_sp.c`
- `Core/Inc/main_rm_sp.h`

---

## 1. Fixed Priority Scheduling (Rate Monotonic)

- Implemented periodic tasks using `vTaskDelayUntil()`
- Assigned priorities based on period (shorter period → higher priority)
- Verified task behavior through tick-based timing observation
- Evaluated schedulability using utilization analysis

---

## 2. Earliest Deadline First (EDF) Scheduler

Extended the FreeRTOS kernel to support dynamic-priority scheduling:

### Key Changes

- Introduced `configUSE_EDF_SCHEDULER` compile-time switch
- Added deadline-ordered ready list
- Extended Task Control Block (TCB) to store task period
- Implemented `xTaskPeriodicCreate()` API for periodic tasks
- Computed absolute deadline:
  - At task creation
  - Upon release from blocked state
- Modified `prvAddTaskToReadyList()` to insert tasks by deadline
- Modified `vTaskSwitchContext()` to select task with earliest deadline

EDF priority is determined dynamically at the job level based on absolute deadlines.

---

## 3. Sporadic Task Support

Implemented sporadic tasks at the application level:

- Tasks initially suspended using `vTaskSuspend()`
- Released via external interrupt using `xTaskResumeFromISR()`
- Context switching triggered using `portYIELD_FROM_ISR()`
- Sporadic tasks remain invisible to scheduler until explicitly resumed

This model supports asynchronous event-driven task activation while preserving fixed-priority scheduling guarantees.

---

## Real-Time Concepts Applied

- Utilization-based schedulability analysis
- Hyperperiod computation
- Worst-case response time evaluation
- Preemption and context switching behavior
- ISR-safe API usage
- Kernel-level ready list manipulation

---

## Repository Structure

```
freertos-scheduler-extensions-stm32/
├── Core/
│   ├── Inc/           # Headers (FreeRTOSConfig.h, main_*.h, uart_config.h)
│   ├── Src/           # Implementations (main_edf.c, main_rm.c, main_rm_speroidic.c, etc.)
│   └── Startup/       # STM32F091RC startup assembly
├── Drivers/
│   ├── CMSIS/         # ARM CMSIS-CORE library
│   └── STM32F0xx_HAL_Driver/  # STM32 Hardware Abstraction Layer
└── Middlewares/
    └── Third_Party/FreeRTOS/  # FreeRTOS kernel (modified: tasks.c, tasks.h, FreeRTOS.h)
```

**Key modifications:** `Middlewares/FreeRTOS/Source/tasks.c`, `tasks.h`, and `FreeRTOS.h` contain custom scheduler implementations for EDF and sporadic scheduling support.

---

## Key Takeaways

This project required direct modification of RTOS kernel internals, including:

- Task Control Block (TCB) structure
- Ready list management
- Context switching logic
- Tick interrupt behavior
- Dynamic priority assignment

The implementation reinforced the connection between real-time scheduling theory and its practical realization inside a production RTOS kernel.