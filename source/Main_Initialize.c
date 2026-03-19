/*
 * Main_Initialize.c
 *
 *  Created on: Mar. 12, 2026
 *      Author: kaiya
 */
#include "board.h"
#include "fsl_uart.h"
#include "LCD_4bit.h"
#include "LEDs.h"
#include "delay.h"
#include "pin_mux.h"
#include "GPS_helper.h"
#include "clock_config.h"
#include "GPIO_defs.h"
#include "user_defs.h"
#include "timers.h"
#include "Main_Initialize.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <stdbool.h>

void INIT_PINS() {
	SIM->SCGC5 |= SIM_SCGC5_PORTD_MASK | SIM_SCGC5_PORTB_MASK
			| SIM_SCGC5_PORTA_MASK;
	// TOF
	PORTA->PCR[16] = PORT_PCR_MUX(1) |
	PORT_PCR_PS_MASK |
	PORT_PCR_PE_MASK |   // pull-UP
			PORT_PCR_IRQC(0x0B); //falling edge trigger
	//Object
	PORTD->PCR[0] = PORT_PCR_MUX(1) |
	PORT_PCR_PS_MASK |
	PORT_PCR_PE_MASK |   // pull-UP
			PORT_PCR_IRQC(0x0B);
	//Line
	PORTD->PCR[2] = PORT_PCR_MUX(1) |
	PORT_PCR_PS_MASK |
	PORT_PCR_PE_MASK |   // pull-UP
			PORT_PCR_IRQC(0x0B);
	PORTD->PCR[7] = PORT_PCR_MUX(1) |
	PORT_PCR_PS_MASK |
	PORT_PCR_PE_MASK |   // pull-UP
			PORT_PCR_IRQC(0x0a);
	//Mode SWITCH
	PORTD->PCR[5] = PORT_PCR_MUX(1) |
	PORT_PCR_PS_MASK |
	PORT_PCR_PE_MASK |   // pull-UP
			PORT_PCR_IRQC(0x0B);
	PTD->PDDR &= ~(MASK(0) | MASK(2) | MASK(5) | MASK(7)); //INPUT

	NVIC_SetPriority(PORTA_IRQn, 0);
	NVIC_ClearPendingIRQ(PORTA_IRQn);
	NVIC_EnableIRQ(PORTA_IRQn);
	NVIC_SetPriority(PORTD_IRQn, 0); // 0, 64, 128 or 192
	NVIC_ClearPendingIRQ(PORTD_IRQn);
	NVIC_EnableIRQ(PORTD_IRQn);
}

void Init_PWM() {
	// System Clock Gating Control Register 5 enable to port A
	SIM->SCGC5 |= SIM_SCGC5_PORTA_MASK | SIM_SCGC5_PORTE_MASK
			| SIM_SCGC5_PORTB_MASK;
	// System Clock Gating Control Register 6 enable to TPM0
	SIM->SCGC6 |= SIM_SCGC6_TPM0_MASK | SIM_SCGC6_TPM1_MASK
			| SIM_SCGC6_TPM2_MASK;

	// Clear TPM clock source bits
	SIM->SOPT2 &= ~SIM_SOPT2_TPMSRC_MASK;
	// Set TPM clock source.
	SIM->SOPT2 |= SIM_SOPT2_TPMSRC(1);

	// Configure multiplexing on PCR5 on port A.
	// Alternate function 3 --> connects to  TMP0 channel 2
	PORTA->PCR[13] = PORT_PCR_MUX(3); //LeftMotor PWM Reverse TPM1 Ch.1
	PORTA->PCR[12] = PORT_PCR_MUX(3); //LeftMotor PWM Forward TPM1 Ch.0

	PORTA->PCR[1] = PORT_PCR_MUX(3); //RightMotor PWM Reverse TPM0 Ch.1
	PORTA->PCR[2] = PORT_PCR_MUX(3); //RightMotor PWM Forward TPM0 Ch.2

	PORTE->PCR[29] = PORT_PCR_MUX(3);
	PORTE->PCR[30] = PORT_PCR_MUX(3);

	// Set maximum timer count
	TPM2->MOD = 255;

	// Enable and configure timer.
	// Set timer count to clock mode
	// set clock source division 2^3. 6,000,000 Hz
	TPM2->SC = TPM_SC_CMOD(1) | TPM_SC_PS(3);

	TPM1->MOD = 255;
	TPM1->SC = TPM_SC_CMOD(1) | TPM_SC_PS(3);

	TPM0->MOD = 255;
	TPM0->SC = TPM_SC_CMOD(1) | TPM_SC_PS(3);

	// Set edge aligned PWM mode.
	// Set output high when counter is less than CnV
	// Left motor
	TPM2->CONTROLS[1].CnSC = TPM_CnSC_MSB_MASK | TPM_CnSC_ELSB_MASK;
	TPM2->CONTROLS[0].CnSC = TPM_CnSC_MSB_MASK | TPM_CnSC_ELSB_MASK;

	// Right motor
	TPM1->CONTROLS[0].CnSC = TPM_CnSC_MSB_MASK | TPM_CnSC_ELSB_MASK;
	TPM1->CONTROLS[1].CnSC = TPM_CnSC_MSB_MASK | TPM_CnSC_ELSB_MASK;

	// Linear Actuator
	TPM0->CONTROLS[2].CnSC = TPM_CnSC_MSB_MASK | TPM_CnSC_ELSB_MASK;
	TPM0->CONTROLS[3].CnSC = TPM_CnSC_MSB_MASK | TPM_CnSC_ELSB_MASK;

	// Set output duty cycle to 0
	TPM2->CONTROLS[0].CnV = 0;
	TPM2->CONTROLS[1].CnV = 0;

	TPM1->CONTROLS[0].CnV = 0;
	TPM1->CONTROLS[1].CnV = 0;

	TPM0->CONTROLS[2].CnV = 0;
	TPM0->CONTROLS[3].CnV = 0;
}

void init_pot(void) {
	SIM->SCGC6 |= SIM_SCGC6_ADC0_MASK;
	// Initialize pot GPIO
	PORTB->PCR[0] &= ~PORT_PCR_MUX_MASK;
	PORTB->PCR[0] |= PORT_PCR_MUX(0);  // Analog mode

	// Configure ADC0
	// Use default clock (bus clock), 8-bit resolution, single-ended mode
	ADC0->CFG1 = ADC_CFG1_MODE(0) |     	// 8-bit mode
			ADC_CFG1_ADICLK(0);    	// Bus clock

	ADC0->SC1[0] = ADC_SC1_ADCH(31); // Disable module initially. 31 disables conversion.
}
void init_encoder(void) {
	SIM->SCGC6 |= SIM_SCGC6_ADC0_MASK;
	// Initialize pot GPIO
	PORTB->PCR[2] &= ~PORT_PCR_MUX_MASK;
	PORTB->PCR[2] |= PORT_PCR_MUX(0);  // Analog mode

	// Configure ADC0
	// Use default clock (bus clock), 8-bit resolution, single-ended mode
	ADC0->CFG1 = ADC_CFG1_MODE(0) |     	// 8-bit mode
			ADC_CFG1_ADICLK(0);    	// Bus clock

	ADC0->SC1[0] = ADC_SC1_ADCH(31); // Disable module initially. 31 disables conversion.
}
void init_lin_pot(void) {
	SIM->SCGC6 |= SIM_SCGC6_ADC0_MASK;
	// Initialize pot GPIO
	PORTB->PCR[1] &= ~PORT_PCR_MUX_MASK;
	PORTB->PCR[1] |= PORT_PCR_MUX(0);  // Analog mode

	// Configure ADC0
	// Use default clock (bus clock), 8-bit resolution, single-ended mode
	ADC0->CFG1 = ADC_CFG1_MODE(0) |     	// 8-bit mode
			ADC_CFG1_ADICLK(0);    	// Bus clock

	ADC0->SC1[0] = ADC_SC1_ADCH(31); // Disable module initially. 31 disables conversion.
}
