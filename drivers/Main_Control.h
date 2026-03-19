/*
 * Main_Control.h
 *
 *  Created on: Mar. 12, 2026
 *      Author: kaiya
 */

#ifndef MAIN_CONTROL_H_
#define MAIN_CONTROL_H_

/*******************************************************************************
 * Definitions
 ******************************************************************************/
/* UART instance and clock */
#define MAIN_UART UART1
#define MAIN_UART_CLKSRC BUS_CLK
#define MAIN_UART_CLK_FREQ CLOCK_GetFreq(BUS_CLK)
#define MAIN_UART_IRQn UART1_IRQn

/*GPS Constants*/
#define GPS_BAUDRATE 9600

#define MAX_LCD 100

/*Magnetic Encoder Constants*/
#define LEFT_MAX 110
#define LS 118
#define STRAIGHT 126
#define RS 134
#define RIGHT_MAX 142
#define DEADZONE 2
/*Direction Constants*/
#define FORWARD 1
#define REVERSE -1
/*! @brief Ring buffer size (Unit: Byte). */
#define MAIN_RING_BUFFER_SIZE 512
#define RX_BUFFER_SIZE 512

/*Speed*/
#define LOW_SPEED 60

/*Line Sensors*/
#define HIGH (1)
#define LOW (0)

/* Ring buffer to save received data. */
uint8_t mainRingBuffer[MAIN_RING_BUFFER_SIZE];
volatile uint16_t rxIndex = 0; //Write ISR
volatile uint16_t readIndex = 0; // Main loop

/*LCD Print Buffers*/
char lastParsedSentence[MAX_PARSE];
volatile bool newSentenceReady = 0;

/*GPS Status*/
volatile char status = 'V';
volatile bool settingSpeed = 0;
volatile float speedMs = 0;
double angleCart = 0;
/*Print Check*/
volatile uint8_t printReady = 0;
volatile uint8_t lineDetectReady = 0;
char update[128];

uint8_t range_left = 124;
uint8_t range_right = 128;

volatile uint8_t limitLCD = MAX_LCD;

/*GPS RMC Configure*/
const char onlyRMC[] = "$PMTK314,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0*29\r\n";

#endif /* MAIN_CONTROL_H_ */
