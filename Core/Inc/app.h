#ifndef INC_APP_H_
#define INC_APP_H_

#include <soft_timer.h>
#include <stdint.h>

extern TIM_HandleTypeDef htim3;

typedef struct OUTPUT_OBJECT
{
	GPIO_TypeDef *Port;

	uint16_t Pin;

	uint32_t Mode;

} OUTPUT_OBJECT_t;

void TIMER1_Config(uint32_t period);
void StartApp(void);
void ToggleDigitalOutput(OUTPUT_OBJECT_t *pOutput);
void Error_Handler_(void);

int Task_Init(void);

#endif /* INC_APP_H_ */
