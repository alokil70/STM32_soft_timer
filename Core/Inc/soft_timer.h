#ifndef SOFTTIMER_H
#define SOFTTIMER_H

#include "main.h"
#include <stdint.h>
#include <stdbool.h>

#define CFG_SOFTTIMER_MAX_AMOUNT		( 50U )						// [dimensionless]
#define CFG_SOFTTIMER_TICK_PERIOD_US	( 100U )					// [us]

typedef enum SOFTTIMER_STATUS
{
	SOFTTIMER_STATUS_SUCCESS = 0U, /**< Status Success if initialization is successful */
	SOFTTIMER_STATUS_FAILURE /**< Status Failure if initialization is failed */
} SOFTTIMER_STATUS_t;

typedef enum SOFTTIMER_STATE
{
	SOFTTIMER_STATE_NOT_INITIALIZED = 0U, /**< The timer is in uninitialized state */
	SOFTTIMER_STATE_RUNNING, /**< The timer is in running state */
	SOFTTIMER_STATE_STOPPED /**< The timer is in stop state */
} SOFTTIMER_STATE_t;

typedef enum SOFTTIMER_MODE
{
	SOFTTIMER_MODE_ONE_SHOT = 0U, /**< timer type is one shot */
	SOFTTIMER_MODE_PERIODIC /**< timer type is periodic */
} SOFTTIMER_MODE_t;

typedef void (*SOFTTIMER_CALLBACK_t)(void *args);

void HardTimerTick_Handler(void);

void SOFTTIMER_Start(void);

void SOFTTIMER_Stop(void);

uint32_t SOFTTIMER_CreateTimer(uint32_t period, SOFTTIMER_MODE_t mode,
		SOFTTIMER_CALLBACK_t callback, void *args);

uint32_t SOFTTIMER_CreateTimerFromISR(uint32_t period, SOFTTIMER_MODE_t mode,
		SOFTTIMER_CALLBACK_t callback, void *args);

SOFTTIMER_STATUS_t SOFTTIMER_StartTimer(uint32_t id);

SOFTTIMER_STATUS_t SOFTTIMER_StartTimerFromISR(uint32_t id);

SOFTTIMER_STATUS_t SOFTTIMER_StopTimer(uint32_t id);

SOFTTIMER_STATUS_t SOFTTIMER_StopTimerFromISR(uint32_t id);

SOFTTIMER_STATUS_t SOFTTIMER_RestartTimer(uint32_t id, uint32_t microsec);

SOFTTIMER_STATUS_t SOFTTIMER_RestartTimerFromISR(uint32_t id, uint32_t microsec);

SOFTTIMER_STATUS_t SOFTTIMER_DeleteTimer(uint32_t id);

SOFTTIMER_STATUS_t SOFTTIMER_DeleteTimerFromISR(uint32_t id);

uint32_t SOFTTIMER_GetTime(void);

uint32_t SOFTTIMER_GetTickCount(void);

SOFTTIMER_STATE_t SOFTTIMER_GetTimerState(uint32_t id);

#endif
