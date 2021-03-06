#ifndef GPIO_H
#define GPIO_H
#include <libopencm3/stm32/rcc.h>
#include "gpio.h"
uint16_t weldExtiInt = 0;
uint8_t weldPinStatus = 0;
uint16_t exti_line_state;

void gpio_init(){

	/* Enable GPIO clock. */
	rcc_periph_clock_enable(RCC_GPIOC);
	rcc_periph_clock_enable(RCC_GPIOA);
	rcc_periph_clock_enable(RCC_GPIOB);

	/*Welding gun pin*/
	gpio_set_mode(WELD_GUN_PORT, GPIO_MODE_INPUT,
			GPIO_CNF_INPUT_FLOAT, WELD_GUN_PIN);
	/*Automatic\manual switch*/
	gpio_set_mode(SWITCH_PORT, GPIO_MODE_INPUT,
			GPIO_CNF_INPUT_FLOAT, SWITCH_PIN);
	/* Enable led as output */
	/*Green for BLuePill*/
	gpio_set_mode(GREEN_LED_PORT, GPIO_MODE_OUTPUT_50_MHZ,
			GPIO_CNF_OUTPUT_PUSHPULL, GREEN_LED);

	/*PWM output pin PA8*/
	gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_50_MHZ,
		      GPIO_CNF_OUTPUT_ALTFN_PUSHPULL,
		      GPIO_TIM1_CH1);
	/*Break output pin*/
	gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_50_MHZ,
		      GPIO_CNF_OUTPUT_ALTFN_PUSHPULL,
		      GPIO_TIM3_CH1);
	/*Frequency measurement input pin PA15*/
	gpio_primary_remap(AFIO_MAPR_SWJ_CFG_JTAG_OFF_SW_ON, AFIO_MAPR_TIM2_REMAP_PARTIAL_REMAP1);
	gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_50_MHZ,
		      GPIO_CNF_INPUT_FLOAT,
		      GPIO_TIM2_FR_CH1_ETR);
	/*Enable pins for gas, welding, motor breaking*/
	gpio_set_mode(GAS_PORT, GPIO_MODE_OUTPUT_50_MHZ,
			GPIO_CNF_OUTPUT_PUSHPULL, GAS_PIN);
	gpio_set_mode(WELD_PORT, GPIO_MODE_OUTPUT_50_MHZ,
			GPIO_CNF_OUTPUT_PUSHPULL, WELD_PIN);
	gpio_set_mode(BREAK_PORT, GPIO_MODE_OUTPUT_50_MHZ,
			GPIO_CNF_OUTPUT_PUSHPULL, BREAK_PIN);

	/*Encoder inputs*/
	gpio_set_mode(ENCODER_A_PORT, GPIO_MODE_INPUT,
			GPIO_CNF_INPUT_FLOAT, ENCODER_A_PIN);
	gpio_set_mode(ENCODER_B_PORT, GPIO_MODE_INPUT,
			GPIO_CNF_INPUT_FLOAT, ENCODER_B_PIN);

		/* Enable EXTI0 interrupt. */
	nvic_enable_irq(NVIC_EXTI9_5_IRQ);
		/* Configure the EXTI subsystem. */
	exti_select_source(EXTI9, WELD_GUN_PORT);
	exti_set_trigger(EXTI9, EXTI_TRIGGER_BOTH);
	exti_enable_request(EXTI9);
}

void exti9_5_isr(void)
{
	exti_line_state = GPIOB_IDR;

	if (exti_get_flag_status(EXTI9))
	{
		if (weldExtiInt == 0)
		{
	weldExtiInt = 50;
	weldPinStatus = gpio_get(WELD_GUN_PORT, WELD_GUN_PIN);
					//usart_send_string(USART1, "WPST: ", 6);
					//usart_send_byte(USART1, weldPinStatus);
					//usart_send_byte(USART1, '\n');

	/*usart_send_string(USART1, "Button interrupt \n", strlen("Button interrupt \n"));*/
		}
	exti_reset_request(EXTI9);
	}
}
#endif
