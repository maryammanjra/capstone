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
#include "Main_Control.h"
#include "Main_Initialize.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <stdbool.h>



/*Line Sensors, Motor Drive, Linear Actuator Drive, Inputs for  Motor and Actuator Pots and Input for Encoder*/
volatile uint8_t duty = 0;
volatile int8_t dir = 0;
volatile uint8_t drive = 0;
volatile int8_t control = 0;
volatile uint8_t adc_input = 0;
volatile uint8_t lin_input = 0;
volatile uint8_t encoder_input = 0;
volatile uint8_t tof_detected = 0;
volatile uint8_t ir_center1 = 0;
volatile uint8_t ir_center2 = 0;
volatile uint8_t ir_left = 0;
volatile uint8_t ir_right = 0;
/*Status Flags*/
volatile bool ready_start = 0;
volatile bool autonomous = 0;
volatile bool stop_cart = 0;
volatile bool mode_switch = 0;
volatile bool ir_ready = 0;

/*******************************************************************************
 * Code
 ******************************************************************************/


void set_pwm_value(uint8_t duty, int8_t dir) {
	if (dir == 1) {
		TPM2->CONTROLS[1].CnV = 0; //Pin 4
		TPM2->CONTROLS[0].CnV = (duty); //Pin 5

		TPM1->CONTROLS[0].CnV = 0; //Pin 12
		TPM1->CONTROLS[1].CnV = (duty); //Pin 13
	} else if (dir == -1) {
		TPM2->CONTROLS[0].CnV = 0; //Pin 4
		TPM2->CONTROLS[1].CnV = (duty); //Pin 5

		TPM1->CONTROLS[1].CnV = 0; //Pin 12
		TPM1->CONTROLS[0].CnV = (duty); //Pin 13
	} else {
		TPM2->CONTROLS[1].CnV = 0; //Pin 4
		TPM2->CONTROLS[0].CnV = 0; //Pin 5

		TPM1->CONTROLS[1].CnV = 0; //Pin 12
		TPM1->CONTROLS[0].CnV = 0; //Pin 13
	}
}
void set_lin_value(uint8_t drive, int8_t control) {
	if (control == 1) {          // Extend
		TPM0->CONTROLS[3].CnV = 0;
		TPM0->CONTROLS[2].CnV = drive;
	} else if (control == -1) {    // Retract
		TPM0->CONTROLS[2].CnV = 0;
		TPM0->CONTROLS[3].CnV = drive;
	} else {                      // Stop
		TPM0->CONTROLS[2].CnV = 0;
		TPM0->CONTROLS[3].CnV = 0;
	}
}



static uint8_t adc_read(void) {
	// Start conversion on specified channel
	ADC0->SC1[0] = ADC_SC1_ADCH(8);

	// Wait for conversion complete
	while (!(ADC0->SC1[0] & ADC_SC1_COCO_MASK))
		;

	// Return result
	return (uint8_t) ADC0->R[0];
}
static uint8_t lin_actuator_read(void) {
	// Start conversion on specified channel
	ADC0->SC1[0] = ADC_SC1_ADCH(9);

	// Wait for conversion complete
	while (!(ADC0->SC1[0] & ADC_SC1_COCO_MASK))
		;

	// Return result
	return (uint8_t) ADC0->R[0];
}
static uint8_t encoder_read(void) {
	// Start conversion on specified channel
	ADC0->SC1[0] = ADC_SC1_ADCH(12);

	// Wait for conversion complete
	while (!(ADC0->SC1[0] & ADC_SC1_COCO_MASK))
		;

	// Return result
	return (uint8_t) ADC0->R[0];
}

/*
 * Ensures GPRMC sentence before parsing info
 * Parses Speed, Latitude, Longitude, Status
 * Sets a newSentenceReady flag for new sentence to update LCD
 */
void parse_RMC(char *sentence) {
	if (strncmp(sentence, "$GPRMC", 6) != 0) {
		return;
	}

	char *token;
	int field = 0;
	double latRaw = 0;
	double longRaw = 0;
	char ns = 'N', ew = 'E';
	double speedKnots = 0;
	status = 'V';
	char buf[128];
	strncpy(buf, sentence, sizeof(buf));
	buf[sizeof(buf) - 1] = '\0';
	token = strtok(buf, ",");
	while (token != NULL) {
		switch (field) {
		case 2:
			status = token[0];
			break;
		case 3:
			latRaw = atof(token);

			break;
		case 4:
			ns = token[0];
			break;
		case 5:
			longRaw = atof(token);

			break;
		case 6:
			ew = token[0];
			break;
		case 7:
			speedKnots = atof(token);
			break;
		case 8:
			angleCart = atof(token);
			break;
		}
		field++;
		token = strtok(NULL, ",");

	}

	double latDeg = ((int) (latRaw / 100))
			+ (latRaw - ((int) (latRaw / 100)) * 100) / 60.0;
	double lonDeg = ((int) (longRaw / 100))
			+ (longRaw - ((int) (longRaw / 100)) * 100) / 60.0;
	if (ns == 'S')
		latDeg = -latDeg;
	if (ew == 'W')
		lonDeg = -lonDeg;
	speedMs = speedKnots * 0.5144;
	double latMeters = 0;
	double lonMeters = 0;
	double currDist = 0;
	int closestBound = 0;
	double angle = 0;
	calcClosestBoundary(latDeg, lonDeg, &currDist, &closestBound, &angle);

	if (status == 'A') {
		snprintf(lastParsedSentence, sizeof(lastParsedSentence),
				"Status: %c | Lat: %2.4f | Lon: %2.4f | Speed: %2.2f | Bearing: %2.2f | Distance to Target: %2.2f",
				status, latDeg, lonDeg, speedMs, angle, currDist);
		//snprintf(lastParsedSentence, lastParsedSentenceSize, "%c", sentence);
	} else {
		snprintf(lastParsedSentence, sizeof(lastParsedSentence),
				"GPS Signal Invalid (Status: %c)", status);
	}
	newSentenceReady = 1;

}
/**
 * Parses a full NMEA sentence then forwards to the data parser
 */
void parse_RX_NMEA(char c) {
	static char line[MAX_PARSE];
	static uint8_t idx = 0;
	if (c == '$') {
		idx = 0;
	}
	if (idx < MAX_PARSE - 1) {
		line[idx++] = c;
	} else {
		idx = 0;
		return;
	}
	if (c == '\n') {
		line[idx - 1] = '\0';
		parse_RMC(line);
		idx = 0;
	}
}

uint8_t map_encoder(uint8_t encoder_input) {
	uint8_t new_encoder = 0;
	if (encoder_input <= STRAIGHT) {
		new_encoder = (encoder_input - LEFT_MAX) * 90 / 16 + 270;
	} else {
		new_encoder = (encoder_input - STRAIGHT) * 90 / 16;
	}
	return new_encoder;

}

void set_straight(uint8_t encoder_input) {

	if (encoder_input < 126) {
		while (encoder_input < 126) {
			set_lin_value(255, 1);
			Delay(15);
			encoder_input = encoder_read();
		}
	} else if (encoder_input > 126) {
		while (encoder_input > 126) {
			set_lin_value(255, (-1));
			Delay(15);
			encoder_input = encoder_read();
		}

	}
	set_lin_value(0, 0);
	Control_RGB_LEDs(1, 0, 1);

}
/*GPS Speed Update Motor*/
void adjust_speed() {
	if (speedMs > 1.5f) {
		if (!autonomous) {
			adc_input = adc_read();

			if (adc_input <= 100 && adc_input >= 0) {
				duty = (100 - adc_input) * 255 / 200;
				dir = 1;
				Control_RGB_LEDs(0, 1, 0);
			} else if (adc_input >= 155 && adc_input <= 255) {
				duty = (adc_input - 155) * 255 / 200;
				dir = -1;
				Control_RGB_LEDs(0, 0, 1);
			} else {
				duty = 0;
				dir = 0;
				Control_RGB_LEDs(1, 0, 0);
			}

		} else if (autonomous) {
			if (duty >= 0 && duty <= 100) {
				duty = 50; //Placeholder for now
			} else if (duty >= 0 && duty >= 100) {
				duty = LOW_SPEED;
			}

		}

		set_pwm_value(duty, dir);
	}

}
void deaccelerate_duty(uint8_t duty) {
	uint8_t step_down = 5;
	//NEEDS CHECK OBJ EVENTUALLY
	while (duty > 0) {
		if ((duty - step_down) >= 0) {
			duty -= step_down;
		} else {
			duty = 0;
		}
		set_pwm_value(duty, dir);
		Delay(10);
	}

}
void accelerate_duty() {
	uint8_t step_up = 5;
	if (duty < LOW_SPEED && duty >= 0) {
		while (duty < LOW_SPEED) {
			if ((duty + step_up) <= LOW_SPEED) {
				duty += step_up;
			} else {
				duty = LOW_SPEED;
			}
			set_pwm_value(duty, dir);
		}
	}

}

void check_uart1_buffer() {
	if (readIndex != rxIndex) {
		uint8_t ch = mainRingBuffer[readIndex];
		readIndex = (readIndex + 1) % MAIN_RING_BUFFER_SIZE;
		parse_RX_NMEA(ch);
		//Control_RGB_LEDs(0, 0, 1);
//		if (limitLCD == 0) {
//			if (lineDetectReady) {
//				Clear_LCD();
//				Data_Output(update);
//				lineDetectReady = 0;
//			} else if (printReady) {
//				Clear_LCD();
//				Data_Output(lastParsedSentence);
//				printReady = 0;
//			}
//			limitLCD = MAX_LCD;
//		}

	}
}
void initial_sequence() {

	set_pwm_value(0, 0);
	encoder_input = encoder_read();
	set_straight(encoder_input);
	autonomous = PTD->PDIR & MASK(16);
	if (status == 'V') {
		ready_start = 1;
		Start_PIT();	//Start timer interrupt for position update
	}

}
void process_adc_input(uint8_t adc_input) {
	if (adc_input <= 100 && adc_input >= 0) {
		duty = (100 - adc_input) * 255 / 200;
		dir = 1;
		Control_RGB_LEDs(0, 1, 0);
	} else if (adc_input >= 155 && adc_input <= 255) {
		duty = (adc_input - 155) * 255 / 200;
		dir = -1;
		Control_RGB_LEDs(0, 0, 1);
	} else {
		duty = 0;
		dir = 0;
		Control_RGB_LEDs(1, 0, 0);
	}
}
void process_lin_input(uint8_t lin_input) {
	if (lin_input <= 100 && lin_input >= 0) {
		drive = (100 - lin_input) * 255 / 100;
		control = 1;
		Control_RGB_LEDs(0, 1, 0);

	} else if (lin_input >= 155 && lin_input <= 255) {
		drive = (lin_input - 155) * 255 / 100;
		control = -1;
		Control_RGB_LEDs(0, 0, 1);
	} else {
		drive = 0;
		control = 0;
		Control_RGB_LEDs(1, 0, 0);
	}
}
/*!
 * @brief Main function
 */
int main(void) {
	uart_config_t config;

	BOARD_InitPins();
	BOARD_BootClockRUN();
	Init_RGB_LEDs();
	Init_PIT((10000 * 1000));
	Init_PWM();
	INIT_PINS();

	Control_RGB_LEDs(0, 1, 0);

	/*
	 * Initialize motor pot, linear actuator, encoder input and LCD
	 */
	init_pot();
	init_encoder();
	init_lin_pot();

	/*
	 * config.baudRate_Bps = 9600;
	 * config.parityMode = kUART_ParityDisabled;
	 * config.stopBitCount = kUART_OneStopBit;
	 * config.txFifoWatermark = 0;
	 * config.rxFifoWatermark = 1;
	 * config.enableTx = false;
	 * config.enableRx = false;
	 */
	UART_GetDefaultConfig(&config);
	config.baudRate_Bps = GPS_BAUDRATE;
	config.enableTx = true;
	config.enableRx = true;
	UART_Init(UART1, &config, MAIN_UART_CLK_FREQ);
	UART_WriteBlocking(UART1, (uint8_t*) onlyRMC, sizeof(onlyRMC) - 1);

	/* Enable RX interrupt. */
	UART_EnableInterrupts(UART1,
			kUART_RxDataRegFullInterruptEnable
					| kUART_RxOverrunInterruptEnable);
	EnableIRQ(UART1_IRQn);
	__enable_irq();

	//__disable_irq(); //TESTING

	/*
	 * Main While Loop for control statement prints and data handling
	 */

	Control_RGB_LEDs(0, 0, 0);

	initial_sequence();

	if (ready_start & autonomous) {
		//set_pwm_value(LOW_SPEED, -1);
		duty = LOW_SPEED;
		Control_RGB_LEDs(0, 1, 1);
	} else if (ready_start & !autonomous) {
		set_pwm_value(0, 0);
		Control_RGB_LEDs(1, 1, 0);
	}
	while (!ready_start) {

	}

	while (1) {
		check_uart1_buffer();
		while (tof_detected || stop_cart) {
			set_pwm_value(0, 0);
			duty = 0;
			dir = 0;
			Control_RGB_LEDs(0, 1, 1);
		}
		//Control_RGB_LEDs(1,0,0);
		if (autonomous && !tof_detected && !stop_cart) {
			if (mode_switch == 1) {
				//deaccelerate_duty(duty);
				mode_switch = 0;
			}

			Control_RGB_LEDs(0, 0, 1);
			if (autonomous && !tof_detected && !stop_cart){
				set_pwm_value(LOW_SPEED, -1);
			}

			if (ir_ready) {
				if(center1 && center2) {
					// go straight
					duty = speed;
					dir = -1;
					encoder_input = encoder_read();
					set_straight(encoder_input);
					//Control_RGB_LEDs(1,0,0);
				}
				else if(left && center1) {
					// go left
				    duty = speed;
					dir = -1;
					// turn lin actuator left
					drive = (lin_input - 155) * 255 / 100;
					control = -1;
					set_lin_value(drive, control);
					//Control_RGB_LEDs(1,1,0);
				}
				else if(right && center2) {
					// go right
					duty = speed;
					dir = -1;
					// turn lin actuator right
					drive = (100-lin_input) * 255 / 100;
					control = 1;
					set_lin_value(drive, control);
					//Control_RGB_LEDs(0,1,0);
				}
//				else if (at GPS coord) { // This needs to be updated when we have the exact coord for the corral
//					// Stop the cart because we reached the docking point
//					duty = 0;
//					dir = 0;
//					// Set wheels straight
//					encoder_input = encoder_read();
//					set_straight(encoder_input);
//				}
				else {
					// lost line
					duty = 0;
					dir = 0;
					// Set wheels straight
					encoder_input = encoder_read();
					set_straight(encoder_input);
					//Control_RGB_LEDs(1,1,1);
				}
			}


		} else if (!autonomous && !tof_detected && !stop_cart) {
			//Control_RGB_LEDs(1, 1, 1);
			/*
			 * Perform a check for mode change before each task
			 * */
			if (mode_switch == 1) {
				deaccelerate_duty(duty);
				mode_switch = 0;
			}

			if (!autonomous && !tof_detected && !stop_cart) {
				set_pwm_value(duty, dir);
				set_lin_value(drive, control);
			}

			adc_input = adc_read();

			lin_input = lin_actuator_read();

			encoder_input = encoder_read();

			process_adc_input(adc_input);

			process_lin_input(lin_input);

		}

	}
}
void PIT_IRQHandler() {

	//clear pending IRQ
	NVIC_ClearPendingIRQ(PIT_IRQn);

	// check to see which channel triggered interrupt
	if (PIT->CHANNEL[0].TFLG & PIT_TFLG_TIF_MASK) { //Check against bit 31 which corresponds to timer interrupt flag
		// clear status flag for timer channel 0
		PIT->CHANNEL[0].TFLG &= PIT_TFLG_TIF_MASK; //Write 1 to clear the flag
		if (newSentenceReady) {
			printReady = 1;
			newSentenceReady = 0;
		}
		if (limitLCD > 0) {
			limitLCD--;
		}

		//duty=0;

	} else if (PIT->CHANNEL[1].TFLG & PIT_TFLG_TIF_MASK) {
		// clear status flag for timer channel 1
		PIT->CHANNEL[1].TFLG &= PIT_TFLG_TIF_MASK;
	}
}
void UART1_IRQHandler(void) {

	/* If new data arrived. */
	if (UART_GetStatusFlags(UART1) & kUART_RxDataRegFullFlag) {
		uint8_t data = UART_ReadByte(UART1);
		uint16_t next = ((rxIndex + 1) % MAIN_RING_BUFFER_SIZE);
		/* If ring buffer is not full, add data to ring buffer. */
		if (next != readIndex) {
			mainRingBuffer[rxIndex] = data;
			rxIndex = next;
		}
	}
	if (UART_GetStatusFlags(UART1) & kUART_RxOverrunFlag) {

		UART_ClearStatusFlags(UART1, kUART_RxOverrunFlag);

	}
}
void PORTA_IRQHandler(void) {
	NVIC_ClearPendingIRQ(PORTA_IRQn);
	if (PORTA->ISFR & MASK(16)) {
		PORTA->ISFR = MASK(16);
		if (PTA->PDIR & MASK(16)) {
			tof_detected = 0;
			Control_RGB_LEDs(1, 1, 0);
		} else {
			tof_detected = 1;
			set_pwm_value(0, 0);
			duty = 0;
			dir = 0;
			Control_RGB_LEDs(0, 1, 1);
		}
	}
}
void PORTD_IRQHandler(void) {
	uint32_t flags = PORTD->ISFR;

	int Center1 = (flags & MASK(0)) ? HIGH : LOW;
	int Center2 = (flags & MASK(1)) ? HIGH : LOW;
	int Right = (flags & MASK(2)) ? HIGH : LOW;
	int Left = (flags & MASK(3)) ? HIGH : LOW;


	//NVIC_ClearPendingIRQ(PORTD_IRQn);
	if ((flags & MASK(7))) {
		Control_RGB_LEDs(0, 1, 1);
		//deaccelerate_duty();
		stop_cart = 1;
		set_pwm_value(0, 0);

	} else if ((flags & MASK(5))) {
		//Control_RGB_LEDs(0, 0, 0);
		if (ready_start) {
			if (!(PTD->PDIR & MASK(5))) {
				Control_RGB_LEDs(1, 1, 0);
				autonomous = 1;
				mode_switch = 1;
				//accelerate_duty();

			} else {
				Control_RGB_LEDs(1, 0, 1);
				autonomous = 0;
				mode_switch = 1;
				//deaccelerate_duty();
			}
		}

	}
	// clear status flags
	PORTD->ISFR = 0xffffffff;

}
