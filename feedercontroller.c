#include <stdint.h>
#include <string.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/f1/nvic.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/stm32/adc.h>
#include <libopencm3/stm32/usart.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/i2c.h>
#include "sources/gpio.h"
#include "sources/rcc.h"
#include "sources/i2c.h"
#include "sources/usart.h"
#include "sources/timers.h"
#include "sources/adc.h"
#include "sources/defines.h"
#include "sources/display.h"

Data_t *d;
volatile uint32_t system_millis = 0;
uint16_t motorFreq; 
uint8_t sequence = 0;
uint8_t task = 0;
uint8_t channel_array[16];

extern uint16_t weldExtiInt;
extern uint8_t weldPinStatus;
extern struct iic i2c;
void milli_sleep(uint32_t delay);
 
void process_task(Tasks_t *task);
void initiate_start_sequence(Tasks_t *task);
void initiate_stop_sequence(Tasks_t *task);
void task_set_delay(Tasks_t *task, uint16_t delay);
void gas_set(uint8_t val);
void break_set(uint8_t val);
void welding_set(uint8_t val);
void break_motor(void);
void calculate_speed(Encoder_t *enc);
int init_varables(void);

int main(void)
{
	rcc_init();
	gpio_init();
	gas_set(false);
	welding_set(false);
	break_set(false);

	usart_init(USART1, 115200, false);
	/*display_set_speed(I2C1, "HELLO");*/


	init_varables();
	tim1_init();
	tim2_init();
	tim3_init();
	tim4_init();
	adc_init();
	/* Select the channel we want to convert. 16=temperature_sensor. */
	channel_array[0] = 16;
	/* Set the injected sequence here, with number of channels */
	adc_set_regular_sequence(ADC1, 1, channel_array);

	channel_array[0] = 8;
	/* Set the injected sequence here, with number of channels */
	adc_set_regular_sequence(ADC2, 1, channel_array);
	systick_setup();

	i2c2_setup();
	display_init(I2C2);
	usart_printf(USART1, "Welding controller started \n");
	int i;
	while (1) {
		for (i = 0; i < 800000; i++)	/* Wait a bit. */
			__asm__("nop");
	}
	return 0;

}
void sys_tick_handler(void){
uint16_t adcVal;
float div;
	system_millis++;

	/*PID regulation here?*/
	if(d->tasks->sequence != STOPPED_SEQUENCE)
	{
		d->tasks->timeout--;
		if (d->tasks->timeout == 0)
		{
			process_task(d->tasks);
		}
	}
	if (d->encoder->systics-- == 0)
	{	
		d->encoder->systics = ENCODER_SYSTICS;
		d->encoder->previous_cnt = d->encoder->current_cnt;
		d->encoder->current_cnt = timer_get_counter(TIM4);
		calculate_speed(d->encoder);
		/*usart_printf(USART1, "SPEED: %f mm/sec\n", d->encoder->speed_mm);*/
		/*usart_printf(USART1, "CNT: %d mm/sec\n", timer_get_counter(TIM4));*/

	}
	if(!gpio_get(SWITCH_PORT, SWITCH_PIN))
	{
		adcVal = adc_get();
		div = adcVal / 4095.0;
		div = div * 100;
		adcVal = (uint16_t)div;
		tim1_set_pwm(adcVal);

		if (weldExtiInt != 0)
		{
			/*usart_send_string(USART1, "Exti != 0\n", 10);*/
			if (weldExtiInt-- == 1)
			{
				if (!gpio_get(WELD_GUN_PORT, WELD_GUN_PIN))
				{
					initiate_start_sequence(d->tasks);
				}else{
					initiate_stop_sequence(d->tasks);
				}
			}
		}
	}
}
/*Gas, break and welding are low-active circuits*/
void gas_set(uint8_t val)
{
	if (val == true)
		gpio_clear(GAS_PORT, GAS_PIN);
	else 
		gpio_set(GAS_PORT, GAS_PIN);
}

void welding_set(uint8_t val)
{
	if (val == true)
		gpio_clear(WELD_PORT, WELD_PIN);
	else
		gpio_set(WELD_PORT, WELD_PIN);
}

void break_set(uint8_t val)
{
	if (val == true)
		gpio_clear(BREAK_PORT, BREAK_PIN);
	else
		gpio_set(BREAK_PORT, BREAK_PIN);
}
void break_motor(void)
{
	timer_disable_break_main_output(TIM1);
	break_set(true);
	tim1_enable(false);
	tim3_enable(true);
}

void initiate_start_sequence(Tasks_t *task)
{
	gas_set(true);
	task->sequence = START_SEQUENCE;
	usart_send_string(USART1, "START GAS_TASK\n", strlen("START GAS_TASK\n"));
	task->subtask = WELD_TASK;
	/*tim4_set_pwm(500);*/
	task_set_delay(task, 500);
	/*tim4_enable(true);*/
}

void initiate_stop_sequence(Tasks_t *task)
{
	break_motor();
	task->sequence = STOP_SEQUENCE;
	usart_send_string(USART1, "STOP MOTOR_TASK\n", strlen("STOP MOTOR_TASK\n"));
	task->subtask = WELD_TASK;
	/*tim4_set_pwm(100);*/
	task_set_delay(task, 100);
	/*tim4_enable(true);*/
}

void process_task(Tasks_t *task)
{
	if (task->sequence == START_SEQUENCE)
	{
		switch (task->subtask)
		{
			case (WELD_TASK):
				usart_send_string(USART1, "START WELD_TASK\n", strlen("START WELD_TASK\n"));
				welding_set(true);
				task->subtask = MOTOR_TASK;
				/*tim4_set_pwm(100);*/
				task_set_delay(task, 100);
				/*tim4_enable(true);*/
				break;
			case (MOTOR_TASK):
				usart_send_string(USART1, "START MOTOR_TASK\n", strlen("START MOTOR_TASK\n"));
				break_set(false);
				tim1_enable(true);
				timer_enable_break_main_output(TIM1);
				break;
		}
	}else if (task->sequence == STOP_SEQUENCE){
		switch (task->subtask)
		{
			case (WELD_TASK):
				usart_send_string(USART1, "STOP WELD_TASK\n", strlen("STOP WELD_TASK\n"));
				welding_set(false);
				task->subtask = GAS_TASK;
				/*tim4_set_pwm(500);*/
				task_set_delay(task, 500);
				/*tim4_enable(true);*/
				break;
			case (GAS_TASK):
				usart_send_string(USART1, "STOP GAS_TASK\n", strlen("STOP GAS_TASK\n"));
				gas_set(false);
				task->sequence = STOPPED_SEQUENCE;
				break;
		}
	}
}

int init_varables(void)
{
	d = calloc(1, sizeof(Data_t));
	d->encoder = calloc(1, sizeof(Encoder_t));
	d->tasks = calloc(1, sizeof(Tasks_t));
	d->tasks->sequence = 0;
	d->tasks->subtask = 0;
	d->encoder->systics = ENCODER_SYSTICS;

	i2c.init_seq = 0;
	i2c.init_cmds[0] = (0x3 << 4) | COMMAND_D;
	usart_printf(USART1, "0 %d \n", i2c.init_cmds[0]);
	i2c.init_cmds[1] = (0x2 << 4) | COMMAND_D;
	i2c.init_cmds[2] = (0x2 << 4)  | COMMAND_D;
	i2c.init_cmds[3] = (0x8 << 4)  | COMMAND_D;
	i2c.init_cmds[4] = (0x0 << 4)  | COMMAND_D;
	i2c.init_cmds[5] = (0x8 << 4)  | COMMAND_D;
	i2c.init_cmds[6] = (0x00 << 4)  | COMMAND_D;
	i2c.init_cmds[7] = (0x01 << 4)  | COMMAND_D;
	i2c.init_cmds[8] = (0x00 << 4)  | COMMAND_D;
	i2c.init_cmds[9] = (0x06 << 4)  | COMMAND_D;
	i2c.init_cmds[10] = (0x00 << 4)  | COMMAND_D;
	i2c.init_cmds[11] = (0x0C << 4)  | COMMAND_D;
	/*
	 *i2c.init_cmds[12] = (0x5 << 4)  | DATA_D;
	 *i2c.init_cmds[13] = (0x3 << 4)  | DATA_D;
	 */
	return 0;
}

void task_set_delay(Tasks_t *task, uint16_t delay)
{
	task->timeout = delay;
}

void calculate_speed(Encoder_t *enc)
{
	float speed;
	speed = enc->current_cnt - enc->previous_cnt;
	/*
	 *enc->speed_mm = (speed * ENCODER_MM_PER_TIC * 1000/ENCODER_SYSTICS); 
	 */
	enc->speed_mm = (speed * ENCODER_MM_PER_TIC); 
}

/* simple sleep for delay milliseconds */
void milli_sleep(uint32_t delay)
{
	uint32_t wake = system_millis + delay;
	while (wake > system_millis) {
		continue;
	}
}

/* Getter function for the current time */
uint32_t mtime(void)
{
	return system_millis;
}
