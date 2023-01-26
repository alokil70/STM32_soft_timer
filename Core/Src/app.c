#include "app.h"

#define ONE_MS_PERIOD_US				( 1000UL )					// [us]
#define BOOT_DELAY_PERIOD_US			( 100 * ONE_MS_PERIOD_US )	// [us]
#define LED_BLINK_PERIOD_US				( 500 * ONE_MS_PERIOD_US )	// [us]

#define SOFTTIMER_TICK_PERIOD_US		( 100U )

OUTPUT_OBJECT_t LED1 =
{ .Port = GPIOC, .Pin = GPIO_PIN_13, .Mode = GPIO_MODE_OUTPUT_PP, };

uint32_t BootDelayTimerId = 0U;

uint32_t LED_TimerId = 0U;

int Task_Init(void)
{
	// Do required MCU initialization such a clock and periphery...

	// Configure the hardware timer 1
	TIMER1_Config(SOFTTIMER_TICK_PERIOD_US);

	__enable_irq();

	HAL_TIM_Base_Start_IT(&htim3);

	// Create a boot delay timer
	BootDelayTimerId = (uint32_t) SOFTTIMER_CreateTimer(
	BOOT_DELAY_PERIOD_US, SOFTTIMER_MODE_ONE_SHOT, (void*) StartApp,
	NULL);
	if (BootDelayTimerId != 0U)
	{
		// Timer is created successfully, now start/run software timer
		SOFTTIMER_STATUS_t status = SOFTTIMER_StartTimer(BootDelayTimerId);

		if (SOFTTIMER_STATUS_SUCCESS != status)
		{
			Error_Handler_(); // Error during software timer start operation
		}
	}
	else
	{
		Error_Handler_(); // timer ID Can not be zero
	}

	// Creating a timer for flashing the LED indicator
	LED_TimerId = (uint32_t) SOFTTIMER_CreateTimer(
	LED_BLINK_PERIOD_US, SOFTTIMER_MODE_PERIODIC, (void*) ToggleDigitalOutput,
			(void*) &LED1);

	if (LED_TimerId == 0U)
	{
		Error_Handler_(); // timer ID Can not be zero
	}

	// Waiting for the boot delay to elapse...
	while (SOFTTIMER_STATE_RUNNING == SOFTTIMER_GetTimerState(BootDelayTimerId))
	{
		HAL_GPIO_WritePin(LED1.Port, LED1.Pin, 0);
	}

	HAL_GPIO_WritePin(LED1.Port, LED1.Pin, 1);

	while (1)
	{
		// Do some app actions finally...
	}
}

void TIMER1_OVF_ISR_Handler(void)
{
	HardTimerTick_Handler();
}

void TIMER1_Config(uint32_t period)
{
	// Do clocking initialization of the hardware timer 1...

	TIM_ClockConfigTypeDef sClockSourceConfig =
	{ 0 };
	TIM_MasterConfigTypeDef sMasterConfig =
	{ 0 };

	htim3.Instance = TIM3;
	htim3.Init.Prescaler = 7;
	htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
	htim3.Init.Period = period - 1;
	htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
	htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;

	if (HAL_TIM_Base_Init(&htim3) != HAL_OK)
	{
		Error_Handler_();
	}
	sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;

	if (HAL_TIM_ConfigClockSource(&htim3, &sClockSourceConfig) != HAL_OK)
	{
		Error_Handler_();
	}
	sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
	sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;

	if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK)
	{
		Error_Handler_();
	}
}

void StartApp(void)
{
	SOFTTIMER_STATUS_t status;

	// Deletу a boot delay timer
	status = SOFTTIMER_DeleteTimerFromISR(BootDelayTimerId);
	if (SOFTTIMER_STATUS_SUCCESS != status)
	{
		// Do some error handling actions...
	}

	//start/run software timer
	status = SOFTTIMER_StartTimerFromISR(LED_TimerId);
	if (SOFTTIMER_STATUS_SUCCESS != status)
	{
		// Do some error handling actions...
	}

}

void ToggleDigitalOutput(OUTPUT_OBJECT_t *pOutput)
{
	HAL_GPIO_TogglePin(pOutput->Port, pOutput->Pin);
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
	if (htim->Instance == TIM3)
	{
		TIMER1_OVF_ISR_Handler();
	}
}

void Error_Handler_(void)
{
	__disable_irq();
	HAL_GPIO_WritePin(LED1.Port, LED1.Pin, 0);
	while (1)
	{
	}
}
