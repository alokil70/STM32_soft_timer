#include <soft_timer.h>

#define ADDITIONAL_CNT_VAL				( 1U )

typedef struct SOFTTIMER_OBJECT
{
	struct SOFTTIMER_OBJECT *next; /**< pointer to next timer control block */
	struct SOFTTIMER_OBJECT *prev; /**< Pointer to previous timer control block */
	SOFTTIMER_CALLBACK_t callback; /**< Callback function pointer */
	SOFTTIMER_MODE_t mode; /**< SoftTimer Type (single shot or periodic) */
	SOFTTIMER_STATE_t state; /**< SoftTimer state */
	void *args; /**< Parameter to callback function */
	uint32_t id; /**< SoftTimer ID */
	uint32_t count; /**< SoftTimer count value */
	uint32_t reload; /**< SoftTimer Reload count value */
	bool delete_swtmr; /**< To delete the timer */
} SOFTTIMER_OBJECT_t;

SOFTTIMER_OBJECT_t timers_table[CFG_SOFTTIMER_MAX_AMOUNT];

SOFTTIMER_OBJECT_t *timers_list = NULL;

uint32_t timers_tracker = 0U;

volatile uint32_t tick_count = 0U;

static void SOFTTIMER_lInsertTimerList(uint32_t tbl_index);

static void SOFTTIMER_lRemoveTimerList(uint32_t tbl_index);

static void SOFTTIMER_lTimerHandler(void);

static void SOFTTIMER_lInsertTimerList(uint32_t tbl_index)
{
	SOFTTIMER_OBJECT_t *object_ptr;
	int32_t delta_ticks;
	int32_t timer_count;
	bool found_flag = false;
	/* Get timer time */
	timer_count = (int32_t) timers_table[tbl_index].count;
	/* Check if Timer list is NULL */
	if (NULL == timers_list)
	{
		/* Set this as first Timer */
		timers_list = &timers_table[tbl_index];
	}
	/* If not, find the correct place, and insert the specified timer */
	else
	{
		object_ptr = timers_list;
		/* Get timer tick */
		delta_ticks = timer_count;
		/* Find correct place for inserting the timer */
		while ((NULL != object_ptr) && (false == found_flag))
		{
			/* Get timer Count Difference */
			delta_ticks -= (int32_t) object_ptr->count;
			/* Check for delta ticks < 0 */
			if (delta_ticks <= 0)
			{
				/* Check If head item */
				if (NULL != object_ptr->prev)
				{
					/* If Insert to list */
					object_ptr->prev->next = &timers_table[tbl_index];
					timers_table[tbl_index].prev = object_ptr->prev;
					timers_table[tbl_index].next = object_ptr;
					object_ptr->prev = &timers_table[tbl_index];
				}
				else
				{
					/* Set Timer as first item */
					timers_table[tbl_index].next = timers_list;
					timers_list->prev = &timers_table[tbl_index];
					timers_list = &timers_table[tbl_index];
				}
				timers_table[tbl_index].count =
						timers_table[tbl_index].next->count
								+ (uint32_t) delta_ticks;
				timers_table[tbl_index].next->count -=
						timers_table[tbl_index].count;
				found_flag = true;
			}
			/* Check for last item in list */
			else
			{
				if ((delta_ticks > 0) && (NULL == object_ptr->next))
				{
					/* Yes, insert into */
					timers_table[tbl_index].prev = object_ptr;
					object_ptr->next = &timers_table[tbl_index];
					timers_table[tbl_index].count = (uint32_t) delta_ticks;
					found_flag = true;
				}
			}
			/* Get the next item in timer list */
			object_ptr = object_ptr->next;
		}
	}
}

static void SOFTTIMER_lRemoveTimerList(uint32_t tbl_index)
{
	SOFTTIMER_OBJECT_t *object_ptr;
	object_ptr = &timers_table[tbl_index];
	/* Check whether only one timer available */
	if ((NULL == object_ptr->prev) && (NULL == object_ptr->next))
	{
		/* set timer list as NULL */
		timers_list = NULL;
	}
	/* Check if the first item in timer list */
	else if (NULL == object_ptr->prev)
	{
		/* Remove timer from list, and reset timer list */
		timers_list = object_ptr->next;
		timers_list->prev = NULL;
		timers_list->count += object_ptr->count;
		object_ptr->next = NULL;
	}
	/* Check if the last item in timer list */
	else if (NULL == object_ptr->next)
	{
		/* Remove timer from list */
		object_ptr->prev->next = NULL;
		object_ptr->prev = NULL;
	}
	else
	{
		/* Remove timer from list */
		object_ptr->prev->next = object_ptr->next;
		object_ptr->next->prev = object_ptr->prev;
		object_ptr->next->count += object_ptr->count;
		object_ptr->next = NULL;
		object_ptr->prev = NULL;
	}
}

static void SOFTTIMER_lTimerHandler(void)
{
	SOFTTIMER_OBJECT_t *object_ptr;
	/* Get first item of timer list */
	object_ptr = timers_list;
	while ((NULL != object_ptr) && (0U == object_ptr->count))
	{
		if (true == object_ptr->delete_swtmr)
		{
			/* Yes, remove this timer from timer list */
			SOFTTIMER_lRemoveTimerList((uint32_t) object_ptr->id);
			/* Set timer status as SOFTTIMER_STATE_NOT_INITIALIZED */
			object_ptr->state = SOFTTIMER_STATE_NOT_INITIALIZED;
			/* Release resource which are hold by this timer */
			timers_tracker &= ~(1U << object_ptr->id);
		}
		/* Check whether timer is a one shot timer */
		else if (SOFTTIMER_MODE_ONE_SHOT == object_ptr->mode)
		{
			if (SOFTTIMER_STATE_RUNNING == object_ptr->state)
			{
				/* Yes, remove this timer from timer list */
				SOFTTIMER_lRemoveTimerList((uint32_t) object_ptr->id);
				/* Set timer status as SOFTTIMER_STATE_STOPPED */
				object_ptr->state = SOFTTIMER_STATE_STOPPED;
				/* Call timer callback function */
				(object_ptr->callback)(object_ptr->args);
			}
		}
		/* Check whether timer is periodic timer */
		else if (SOFTTIMER_MODE_PERIODIC == object_ptr->mode)
		{
			if (SOFTTIMER_STATE_RUNNING == object_ptr->state)
			{
				/* Yes, remove this timer from timer list */
				SOFTTIMER_lRemoveTimerList((uint32_t) object_ptr->id);
				/* Reset timer tick */
				object_ptr->count = object_ptr->reload;
				/* Insert timer into timer list */
				SOFTTIMER_lInsertTimerList((uint32_t) object_ptr->id);
				/* Call timer callback function */
				(object_ptr->callback)(object_ptr->args);
			}
		}
		else
		{
			break;
		}
		/* Get first item of timer list */
		object_ptr = timers_list;
	}
}

void HardTimerTick_Handler(void)
{
	SOFTTIMER_OBJECT_t *object_ptr;
	object_ptr = timers_list;
	tick_count++;

	if (NULL != object_ptr)
	{
		if (object_ptr->count > 1UL)
		{
			object_ptr->count--;
		}
		else
		{
			object_ptr->count = 0U;
			SOFTTIMER_lTimerHandler();
		}
	}
}

uint32_t SOFTTIMER_CreateTimer(uint32_t period, SOFTTIMER_MODE_t mode,
		SOFTTIMER_CALLBACK_t callback, void *args)
{
	uint32_t id = 0U;
	uint32_t count = 0U;
	uint32_t period_ratio = 0U;

	if (period < CFG_SOFTTIMER_TICK_PERIOD_US)
	{
		id = 0U;
	}
	else
	{
		for (count = 0U; count < CFG_SOFTTIMER_MAX_AMOUNT; count++)
		{
			/* Check for free timer ID */
			if (0U == (timers_tracker & (1U << count)))
			{
				/* If yes, assign ID to this timer */
				timers_tracker |= (1U << count);
				/* Initialize the timer as per input values */
				timers_table[count].id = count;
				timers_table[count].mode = mode;
				timers_table[count].state = SOFTTIMER_STATE_STOPPED;
				period_ratio =
						(uint32_t) (period / CFG_SOFTTIMER_TICK_PERIOD_US);
				timers_table[count].count = (period_ratio + ADDITIONAL_CNT_VAL);
				timers_table[count].reload = period_ratio;
				timers_table[count].callback = callback;
				timers_table[count].args = args;
				timers_table[count].prev = NULL;
				timers_table[count].next = NULL;
				id = count + 1U;
				break;
			}
		}

	}

	return (id);
}

SOFTTIMER_STATUS_t SOFTTIMER_StartTimer(uint32_t id)
{
	SOFTTIMER_STATUS_t status;
	status = SOFTTIMER_STATUS_FAILURE;

	/* Check if timer is running */
	if (SOFTTIMER_STATE_STOPPED == timers_table[id - 1U].state)
	{
		timers_table[id - 1U].count = (timers_table[id - 1U].reload
				+ ADDITIONAL_CNT_VAL);
		/* set timer status as SOFTTIMER_STATE_RUNNING */
		timers_table[id - 1U].state = SOFTTIMER_STATE_RUNNING;
		/* Insert this timer into timer list */
		SOFTTIMER_lInsertTimerList((id - 1U));
		status = SOFTTIMER_STATUS_SUCCESS;
	}

	return (status);
}

SOFTTIMER_STATUS_t SOFTTIMER_StopTimer(uint32_t id)
{
	SOFTTIMER_STATUS_t status;
	status = SOFTTIMER_STATUS_SUCCESS;

	if (SOFTTIMER_STATE_NOT_INITIALIZED == timers_table[id - 1U].state)
	{
		status = SOFTTIMER_STATUS_FAILURE;
	}
	else
	{
		/* Check whether Timer is in Stop state */
		if (SOFTTIMER_STATE_RUNNING == timers_table[id - 1U].state)
		{
			/* Set timer status as SOFTTIMER_STATE_STOPPED */
			timers_table[id - 1U].state = SOFTTIMER_STATE_STOPPED;

			/* remove Timer from node list */
			SOFTTIMER_lRemoveTimerList(id - 1U);

		}
	}

	return (status);
}

SOFTTIMER_STATUS_t SOFTTIMER_RestartTimer(uint32_t id, uint32_t microsec)
{
	uint32_t period_ratio = 0U;
	SOFTTIMER_STATUS_t status;
	status = SOFTTIMER_STATUS_SUCCESS;

	if (SOFTTIMER_STATE_NOT_INITIALIZED == timers_table[id - 1U].state)
	{
		status = SOFTTIMER_STATUS_FAILURE;
	}
	else
	{
		/* check whether timer is in run state */
		if (SOFTTIMER_STATE_STOPPED != timers_table[id - 1U].state)
		{
			/* Stop the timer */
			status = SOFTTIMER_StopTimer(id);
		}
		if (SOFTTIMER_STATUS_SUCCESS == status)
		{
			period_ratio = (uint32_t) (microsec / CFG_SOFTTIMER_TICK_PERIOD_US);
			timers_table[id - 1U].reload = period_ratio;
			/* Start the timer */
			status = SOFTTIMER_StartTimer(id);
		}
	}

	return (status);
}

SOFTTIMER_STATUS_t SOFTTIMER_DeleteTimer(uint32_t id)
{
	SOFTTIMER_STATUS_t status;
	status = SOFTTIMER_STATUS_SUCCESS;

	/* Check whether Timer is in delete state */
	if (SOFTTIMER_STATE_NOT_INITIALIZED == timers_table[id - 1U].state)
	{
		status = SOFTTIMER_STATUS_FAILURE;
	}
	else
	{
		if (SOFTTIMER_STATE_STOPPED == timers_table[id - 1U].state)
		{
			/* Set timer status as SOFTTIMER_STATE_NOT_INITIALIZED */
			timers_table[id - 1U].state = SOFTTIMER_STATE_NOT_INITIALIZED;
			/* Release resource which are hold by this timer */
			timers_tracker &= ~(1U << (id - 1U));
		}
		else
		{
			/* Yes, remove this timer from timer list during ISR execution */
			timers_table[id - 1U].delete_swtmr = true;
		}
	}

	return (status);
}

uint32_t SOFTTIMER_GetTime(void)
{
	return (tick_count * CFG_SOFTTIMER_TICK_PERIOD_US);
}

uint32_t SOFTTIMER_GetTickCount(void)
{
	return (tick_count);
}

SOFTTIMER_STATE_t SOFTTIMER_GetTimerState(uint32_t id)
{
	return (timers_table[id - 1U].state);
}

uint32_t SOFTTIMER_CreateTimerFromISR(uint32_t period, SOFTTIMER_MODE_t mode,
		SOFTTIMER_CALLBACK_t callback, void *args)
{
	uint32_t id;

	__disable_irq();

	id = SOFTTIMER_CreateTimer(period, mode, callback, args);

	__enable_irq();

	return (id);
}

SOFTTIMER_STATUS_t SOFTTIMER_StartTimerFromISR(uint32_t id)
{
	SOFTTIMER_STATUS_t status;

	__disable_irq();

	status = SOFTTIMER_StartTimer(id);

	__enable_irq();

	return (status);
}

SOFTTIMER_STATUS_t SOFTTIMER_StopTimerFromISR(uint32_t id)
{
	SOFTTIMER_STATUS_t status;

	__disable_irq();

	status = SOFTTIMER_StopTimer(id);

	__enable_irq();

	return (status);
}

SOFTTIMER_STATUS_t SOFTTIMER_RestartTimerFromISR(uint32_t id, uint32_t microsec)
{
	SOFTTIMER_STATUS_t status;

	__disable_irq();

	status = SOFTTIMER_RestartTimer(id, microsec);

	__enable_irq();

	return (status);
}

SOFTTIMER_STATUS_t SOFTTIMER_DeleteTimerFromISR(uint32_t id)
{
	SOFTTIMER_STATUS_t status;

	__disable_irq();

	status = SOFTTIMER_DeleteTimer(id);

	__enable_irq();

	return (status);
}
