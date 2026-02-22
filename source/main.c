// Standard library headers
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <stdbool.h>

// Capstone internal headers - all present in drivers directory
#include "board.h"
#include "fsl_uart.h"
#include "lcd_4_bit.h"
#include "leds.h"
#include "delay.h"
#include "pin_mux.h"
#include "clock_config.h"
#include "gpio_defs.h"
#include "user_defs.h"



#define MAIN_UART UART1
#define MAIN_UART_CLKSRC BUS_CLK
#define MAIN_UART_CLK_FREQ CLOCK_GetFreq(BUS_CLK)
#define MAIN_UART_IRQn UART1_IRQn
#define GPS_BAUDRATE 9600
#define LAT_CONSTANT 111000
#define HIGH (1)
#define LOW (0)
#define MAIN_RING_BUFFER_SIZE 512
#define RX_BUFFER_SIZE 512

const char ONLY_RMC[] = "$PMTK314,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0*29\r\n";

int threshold[NUM_RANGE_STEPS] = { 500, 700, 800, 1000, 200, 0 };

const int Speeds[NUM_RANGE_STEPS] = { 100, 80, 60, 40, 20, 0 };
volatile uint8_t duty = 0;
volatile int8_t dir = 0;
volatile uint8_t drive = 0;
volatile int8_t control = 0;
volatile uint8_t adc_input = 0;
volatile uint8_t lin_input = 0;
volatile uint8_t encoder_input =0;


void init_pins() {
	SIM->SCGC5 |= SIM_SCGC5_PORTD_MASK|SIM_SCGC5_PORTB_MASK;
	
	// object
	PORTD->PCR[0] = PORT_PCR_MUX(1) |
	PORT_PCR_PS_MASK |
	PORT_PCR_PE_MASK |   // pull-up
			PORT_PCR_IRQC(0x0B);

	// line
	PORTD->PCR[2] = PORT_PCR_MUX(1) |
	PORT_PCR_PS_MASK |
	PORT_PCR_PE_MASK |   // pull-up
			PORT_PCR_IRQC(0x0B);

	PTD->PDDR &= ~(MASK(0) | MASK(2)); // input
}

void init_pwm() {
	// System Clock Gating Control Register 5 enable to port A
	SIM->SCGC5 |= SIM_SCGC5_PORTA_MASK | SIM_SCGC5_PORTE_MASK | SIM_SCGC5_PORTB_MASK;
	// System Clock Gating Control Register 6 enable to TPM0
	SIM->SCGC6 |= SIM_SCGC6_TPM0_MASK | SIM_SCGC6_TPM1_MASK|SIM_SCGC6_TPM2_MASK;

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
	TPM1->SC  = TPM_SC_CMOD(1) | TPM_SC_PS(3);

	TPM0->MOD = 255;
	TPM0->SC  = TPM_SC_CMOD(1) | TPM_SC_PS(3);

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

void Set_PWM_Value(uint8_t duty, int8_t dir) {
	if(dir == 1){
		TPM2->CONTROLS[1].CnV = 0; //Pin 4
		TPM2->CONTROLS[0].CnV = (duty); //Pin 5

		TPM1->CONTROLS[0].CnV = 0; //Pin 12
		TPM1->CONTROLS[1].CnV = (duty); //Pin 13
	}
	else if(dir == -1){
		TPM2->CONTROLS[0].CnV = 0; //Pin 4
		TPM2->CONTROLS[1].CnV = (duty); //Pin 5

		TPM1->CONTROLS[1].CnV = 0; //Pin 12
		TPM1->CONTROLS[0].CnV = (duty); //Pin 13
	}
	else{
		TPM2->CONTROLS[1].CnV = 0; //Pin 4
		TPM2->CONTROLS[0].CnV = 0; //Pin 5

		TPM1->CONTROLS[1].CnV = 0; //Pin 12
		TPM1->CONTROLS[0].CnV = 0; //Pin 13
	}
}

void set_lin_value(uint8_t drive, int8_t control){
	if(control == 1){          // Extend
	        TPM0->CONTROLS[3].CnV = 0;
	        TPM0->CONTROLS[2].CnV = drive;
	    }
	    else if(control == -1){    // Retract
	        TPM0->CONTROLS[2].CnV = 0;
	        TPM0->CONTROLS[3].CnV = drive;
	    }
	    else{                      // Stop
	        TPM0->CONTROLS[2].CnV = 0;
	        TPM0->CONTROLS[3].CnV = 0;
	    }
}

void init_pot(void){
	SIM->SCGC6 |= SIM_SCGC6_ADC0_MASK;
	// Initialize pot GPIO
	PORTB->PCR[0] &= ~PORT_PCR_MUX_MASK;
	PORTB->PCR[0] |= PORT_PCR_MUX(0);  // Analog mode

	// Configure ADC0
	// Use default clock (bus clock), 8-bit resolution, single-ended mode
	ADC0->CFG1 = ADC_CFG1_MODE(0) |     	// 8-bit mode
				 ADC_CFG1_ADICLK(0);    	// Bus clock

	ADC0->SC1[0] = ADC_SC1_ADCH(31);    	// Disable module initially. 31 disables conversion.
}

void init_encoder(void){
	SIM->SCGC6 |= SIM_SCGC6_ADC0_MASK;
		// Initialize pot GPIO
		PORTB->PCR[2] &= ~PORT_PCR_MUX_MASK;
		PORTB->PCR[2] |= PORT_PCR_MUX(0);  // Analog mode

		// Configure ADC0
		// Use default clock (bus clock), 8-bit resolution, single-ended mode
		ADC0->CFG1 = ADC_CFG1_MODE(0) |     	// 8-bit mode
					 ADC_CFG1_ADICLK(0);    	// Bus clock

		ADC0->SC1[0] = ADC_SC1_ADCH(31);    	// Disable module initially. 31 disables conversion.
}

void init_lin_pot(void){

	SIM->SCGC6 |= SIM_SCGC6_ADC0_MASK;

	// initialize pot GPIO
	PORTB->PCR[1] &= ~PORT_PCR_MUX_MASK;
	PORTB->PCR[1] |= PORT_PCR_MUX(0);  // analog mode

	// configure ADC0
	// use default clock (bus clock), 8-bit resolution, single-ended mode
	ADC0->CFG1 = ADC_CFG1_MODE(0) |     	// 8-bit mode
					ADC_CFG1_ADICLK(0);    	// bus clock

	ADC0->SC1[0] = ADC_SC1_ADCH(31);    	// disable module initially, 31 disables conversion.
}

static uint8_t ADC_Read(void){
    
    ADC0->SC1[0] = ADC_SC1_ADCH(8);  // start conversion on specified channel
    while (!(ADC0->SC1[0] & ADC_SC1_COCO_MASK));    // wait for conversion complete

    return (uint8_t)ADC0->R[0];   // return result
}

static uint8_t lin_actuator_read(void){

    ADC0->SC1[0] = ADC_SC1_ADCH(9);
    while (!(ADC0->SC1[0] & ADC_SC1_COCO_MASK));

    return (uint8_t)ADC0->R[0];
}

static uint8_t encoder_read(void){

    ADC0->SC1[0] = ADC_SC1_ADCH(12);
    while (!(ADC0->SC1[0] & ADC_SC1_COCO_MASK)); 

    return (uint8_t)ADC0->R[0]; 
}

void data_output(char *string) {

	int i = 0;
	int j = 0;
	set_cursor(0, 0);

	while (*string && i < 16 && j < 2) {

		if (j == 0 && i >= 7 && (*string == ' ')) {
			i = 0;
			j = 1;
			set_cursor(i, j);
		}

		if (j == 1 && i >= 7 && (*string == ' ')) {
			i = 0;
			j = 0;
			set_cursor(i, j);
			clear_lcd();
		}

		lcd_putchar(*string++);
		i++;

		if (i == 16) {
			j++;
			i = 0;
			if (j == 2 && (*string != '\0')) {
				j = 0;
				clear_lcd();
			}
		}

		set_cursor(i, j);

	}
}


/*!
 * @brief Main function
 */
int main(void) {


	BOARD_InitPins();
	BOARD_BootClockRUN();

	Init_RGB_LEDs();
	//Init_PIT((6000 * 1000));
	init_pwm();
	init_pins();

	Control_RGB_LEDs(0, 1, 0);

	//Start_PIT();	//Start timer interrupt for position update


	init_pot();
	init_encoder();
	init_lin_pot();

	while (1) {

		Set_PWM_Value(duty, dir);
		set_lin_value(drive,control);

		adc_input = ADC_Read();
		lin_input = lin_actuator_read();
		encoder_input = encoder_read();

		char buf[128];

		if (adc_input <= 100 && adc_input >= 0){
			duty = (100 - adc_input) * 255 / 200;
			dir = 1;
			Control_RGB_LEDs(0,1,0);
		} else if(adc_input >= 155 && adc_input <= 255){
			duty = (adc_input - 155) * 255 / 200;
			dir = -1;
			Control_RGB_LEDs(0,0,1);
		} else {
			duty = 0;
			dir = 0;
			Control_RGB_LEDs(1,0,0);
		}

		// snprintf(buf,sizeof(buf),"Encoder Read: %d", encoder_input);
		if (lin_input <= 100 && lin_input >= 0){
			drive = (100 - lin_input) * 255 / 100;
			control = 1;
			Control_RGB_LEDs(0,1,0);
		} else if(lin_input >= 155 && lin_input <= 255){
			drive = (lin_input - 155) * 255 / 100;
			control = -1;
			Control_RGB_LEDs(0,0,1);
		} else {
			drive = 0;
			control = 0;
			Control_RGB_LEDs(1,0,0);
		}
		// data_output(buf);

	}
}

void PIT_IRQHandler() {


	NVIC_ClearPendingIRQ(PIT_IRQn); // clear pending IRQ

	// check to see which channel triggered interrupt
	if (PIT->CHANNEL[0].TFLG & PIT_TFLG_TIF_MASK) { // check against bit 31 which corresponds to timer interrupt flag
		// clear status flag for timer channel 0
		PIT->CHANNEL[0].TFLG &= PIT_TFLG_TIF_MASK; // write 1 to clear the flag
		// duty = 0;	
	} else if (PIT->CHANNEL[1].TFLG & PIT_TFLG_TIF_MASK) {
		// clear status flag for timer channel 1
		PIT->CHANNEL[1].TFLG &= PIT_TFLG_TIF_MASK;
	}
}
