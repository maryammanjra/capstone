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

#define STOP "STOP" //for later
#define RESUME "RES"

const char ledTest[] = "4";
const char ledTest2[] = "5";
const char stopCMD[] = "STOP\n";
const char resumeCMD[] = "RESUME\n";
volatile bool stop = 0;
volatile bool resume = 0;
volatile bool delay = 0;



uint8_t mainRingBuffer[MAIN_RING_BUFFER_SIZE];
volatile uint16_t rxIndex = 0;
volatile uint16_t readIndex = 0;
volatile bool sent = 0;
char lastParsedSentence[MAX_PARSE];
volatile uint8_t newSentenceReady = 0;

void INIT_PINS() {
	SIM->SCGC5 |= SIM_SCGC5_PORTD_MASK;

	/* UART2 RX = PTD2 */
	PORTD->PCR[2] = PORT_PCR_MUX(3);

	/* UART2 TX = PTD3 */
	PORTD->PCR[3] = PORT_PCR_MUX(3);

	PORTD->PCR[7] = PORT_PCR_MUX(1) |
	PORT_PCR_PS_MASK |
	PORT_PCR_PE_MASK |   // pull-UP
			PORT_PCR_IRQC(0x0B);

	// Mode switch
	PORTD->PCR[16] = PORT_PCR_MUX(1) |
	PORT_PCR_PS_MASK |
	PORT_PCR_PE_MASK |   // pull-UP
			PORT_PCR_IRQC(0x0B);

	PTD->PDDR &= ~(MASK(0) | MASK(7) | MASK(16)); //INPUT

	NVIC_SetPriority(PORTD_IRQn, 0); // 0, 64, 128 or 192

	NVIC_ClearPendingIRQ(PORTD_IRQn);

	NVIC_EnableIRQ(PORTD_IRQn);
}


int main(void) {
	Init_RGB_LEDs();
	Control_RGB_LEDs(1, 0, 0);

	uart_config_t config;

	BOARD_InitPins();
	BOARD_BootClockRUN();
	Control_RGB_LEDs(0, 0, 1);
	//Init_RGB_LEDs();
	Init_PIT((5000 * 1000));
	INIT_PINS();

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

	Start_PIT();
	while (1) {

	}
}

void PIT_IRQHandler() {

	//clear pending IRQ
	NVIC_ClearPendingIRQ(PIT_IRQn);

	// check to see which channel triggered interrupt
	if (PIT->CHANNEL[0].TFLG & PIT_TFLG_TIF_MASK) { //Check against bit 31 which corresponds to timer interrupt flag
		// clear status flag for timer channel 0
		PIT->CHANNEL[0].TFLG &= PIT_TFLG_TIF_MASK; //Write 1 to clear the flag

		// If the switch is pressed, reset the switch global variable
		if (delay) {
			UART_WriteBlocking(UART2, (uint8_t*) ledTest2,
						sizeof(ledTest2) - 1);
			sent = 0;
			//Flash blue - delay
			Control_RGB_LEDs(0,0,1);
			Delay(50);
			delay = 0;
		} else if (!delay){
			UART_WriteBlocking(UART2, (uint8_t*) ledTest, sizeof(ledTest) - 1);
			sent = 1;
			//Flash red when not sent
			Control_RGB_LEDs(1, 0, 0);
		}

	} else if (PIT->CHANNEL[1].TFLG & PIT_TFLG_TIF_MASK) {
		// clear status flag for timer channel 1
		PIT->CHANNEL[1].TFLG &= PIT_TFLG_TIF_MASK;
	}
}


// At the moment this doesn't do much yet
void UART2_IRQHandler(void) {

	/* If new data arrived. */
	if (UART_GetStatusFlags(UART2) & kUART_RxDataRegFullFlag) {
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

void PORTD_IRQHandler(void) {
	uint32_t flags = PORTD->ISFR;

	NVIC_ClearPendingIRQ(PORTD_IRQn);
	if ((flags & MASK(7))) {

		// Switch pressed - then do processing because of pull-up need to check with the external resistor
		if(PTD->PDIR & MASK(7)){
			Stop_PIT();

			// Flash yellow
			Control_RGB_LEDs(1,1,0);
			Delay(50);
			delay = 1;
			Set_Time((500 * 1000));
			Start_PIT();
		}

	}

	PORTD->ISFR = 0xffffffff;

}
