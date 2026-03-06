#include "board.h"
#include "fsl_uart.h"
#include "LCD_4bit.h"
#include "LEDs.h"
#include "delay.h"
#include "pin_mux.h"
#include "timers.h"
#include "clock_config.h"
#include "GPIO_defs.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>


#define MAIN_UART UART2
#define MAIN_UART_CLKSRC BUS_CLK
#define MAIN_UART_CLK_FREQ CLOCK_GetFreq(BUS_CLK)
#define MAIN_UART_IRQn UART2_IRQn
#define GPS_BAUDRATE 9600

#define MAIN_RING_BUFFER_SIZE 512
#define RX_BUFFER_SIZE 512
#define MAX_SENTENCE 128
#define MAX_PARSE 256
#define STOP "STOP"
const char ledTest[] = "1";
const char ledTest2[] = "0";
volatile bool stop = 0;
volatile bool resume = 0;



uint8_t mainRingBuffer[MAIN_RING_BUFFER_SIZE];
volatile uint16_t rxIndex = 0; // Write ISR
volatile uint16_t readIndex = 0; // Main loop
volatile char cmdBuffer[10];
volatile int cmdIndex = 0;
volatile bool sent = 0;
char lastParsedSentence[MAX_PARSE];
volatile uint8_t newSentenceReady = 0;


int main(void) {
	Init_RGB_LEDs();

	SIM->SCGC5 |= SIM_SCGC5_PORTD_MASK;
	PORTD->PCR[2] = PORT_PCR_MUX(3);
	PORTD->PCR[3] = PORT_PCR_MUX(3);

	// Start in red flash
	Control_RGB_LEDs(1, 0, 0);
	Delay(50);

	uart_config_t config;

	BOARD_InitPins();
	BOARD_BootClockRUN();
	Control_RGB_LEDs(0, 0, 1);
	//Init_RGB_LEDs();
	Init_PIT((5000 * 1000));

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
	Control_RGB_LEDs(0, 1, 0);
	config.baudRate_Bps = GPS_BAUDRATE;
	config.enableTx = true;
	config.enableRx = true;

	Control_RGB_LEDs(0, 0, 0);

	UART_Init(UART2, &config, MAIN_UART_CLK_FREQ);

	/* Enable RX interrupt. */
	UART_EnableInterrupts(UART2,
			kUART_RxDataRegFullInterruptEnable
					| kUART_RxOverrunInterruptEnable);
	EnableIRQ(UART2_IRQn);

	//Start_PIT();
	while (1) {

		while (!stop) {
			if (readIndex != rxIndex) {
				Control_RGB_LEDs(0,1,0);
				char data = mainRingBuffer[readIndex];
				readIndex = (readIndex + 1) % MAIN_RING_BUFFER_SIZE;
				cmdBuffer[cmdIndex++] = data;

				if (data == '\n') {   // end of command
					cmdBuffer[cmdIndex] = '\0';
					cmdIndex = 0;
				}

				// Here checking if a single char is equal to 4 - might be a bit sus
				if (data == '4') {
					// If received a 4 flash green
					Control_RGB_LEDs(0, 1, 0);
					Delay(50);
				} else if (data == '5') {
					//If received a 5 flash yellow
					Control_RGB_LEDs(1, 1, 0);
					Delay(50);
				}

			}
		}

		while (stop) {
			if (readIndex != rxIndex) {
				char data = mainRingBuffer[readIndex];
				readIndex = (readIndex + 1) % MAIN_RING_BUFFER_SIZE;
				if (data == '\n') {   // end of command
					cmdBuffer[cmdIndex] = '\0';

					if (strcmp(cmdBuffer, "RESUME") == 0) {
						Control_RGB_LEDs(0, 1, 1);
						stop = 0;
					}

					cmdIndex = 0;
				}

			}

		}
		//UART_WriteBlocking(UART2, (uint8_t*) ledTest2, sizeof(ledTest2) - 1);
	}
	//Control_RGB_LEDs(1, 0, 0);
	//Stop_PIT();

}

void PIT_IRQHandler() {
	//unsigned short out_data = 0;

	//clear pending IRQ
	NVIC_ClearPendingIRQ(PIT_IRQn);

	// check to see which channel triggered interrupt
	if (PIT->CHANNEL[0].TFLG & PIT_TFLG_TIF_MASK) { //Check against bit 31 which corresponds to timer interrupt flag
		// clear status flag for timer channel 0
		PIT->CHANNEL[0].TFLG &= PIT_TFLG_TIF_MASK; //Write 1 to clear the flag
		if (sent) {
			UART_WriteBlocking(UART2, (uint8_t*) ledTest2,
					sizeof(ledTest2) - 1);
			sent = 0;
		} else {
			UART_WriteBlocking(UART2, (uint8_t*) ledTest, sizeof(ledTest) - 1);
			sent = 1;
		}
		Control_RGB_LEDs(sent, 0, !sent);

	} else if (PIT->CHANNEL[1].TFLG & PIT_TFLG_TIF_MASK) {
		// clear status flag for timer channel 1
		PIT->CHANNEL[1].TFLG &= PIT_TFLG_TIF_MASK;
	}
}


void UART2_IRQHandler(void) {

	/* If new data arrived. */
	if (UART_GetStatusFlags(UART2) & kUART_RxDataRegFullFlag) {
		Control_RGB_LEDs(0,0,1);
		uint8_t data = UART_ReadByte(UART2);
		uint16_t next = ((rxIndex + 1) % MAIN_RING_BUFFER_SIZE);
		/* If ring buffer is not full, add data to ring buffer. */
		if (next != readIndex) {
			mainRingBuffer[rxIndex] = data;
			rxIndex = next;
		}
	}

	if (UART_GetStatusFlags(UART2) & kUART_RxOverrunFlag) {

		UART_ClearStatusFlags(UART2, kUART_RxOverrunFlag);

	}
}
