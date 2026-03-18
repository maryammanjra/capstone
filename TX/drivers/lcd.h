#ifndef LCD_H
#define LCD_H

#include "MKL25Z4.h"
#include <stdint.h>

#define LCD_COLUMNS   8u
#define LCD_ROWS      2u

/* 4-bit interface on GPIO */
#define PIN_DATA_PORT    PORTC
#define PIN_DATA_PT      PTC

/* LCD data lines:
 * D4 -> PTC3
 * D5 -> PTC4
 * D6 -> PTC5
 * D7 -> PTC6
 */
#define PIN_D4_SHIFT     (3u)
#define PIN_D5_SHIFT     (4u)
#define PIN_D6_SHIFT     (5u)
#define PIN_D7_SHIFT     (6u)

#define PIN_D4           (1u << PIN_D4_SHIFT)
#define PIN_D5           (1u << PIN_D5_SHIFT)
#define PIN_D6           (1u << PIN_D6_SHIFT)
#define PIN_D7           (1u << PIN_D7_SHIFT)

#define PINS_DATA        (PIN_D4 | PIN_D5 | PIN_D6 | PIN_D7)

#define PIN_E_PORT       PORTC
#define PIN_E_PT         PTC
#define PIN_E_SHIFT      (7u)
#define PIN_E            (1u << PIN_E_SHIFT)

#define PIN_RW_PORT      PORTC
#define PIN_RW_PT        PTC
#define PIN_RW_SHIFT     (8u)
#define PIN_RW           (1u << PIN_RW_SHIFT)

#define PIN_RS_PORT      PORTC
#define PIN_RS_PT        PTC
#define PIN_RS_SHIFT     (9u)
#define PIN_RS           (1u << PIN_RS_SHIFT)

#define ENABLE_LCD_PORT_CLOCKS \
    (SIM->SCGC5 |= SIM_SCGC5_PORTC_MASK | SIM_SCGC5_PORTD_MASK | SIM_SCGC5_PORTA_MASK)

#define SET_LCD_E(x)     do { if (x) { PIN_E_PT->PSOR = PIN_E; } else { PIN_E_PT->PCOR = PIN_E; } } while (0)
#define SET_LCD_RW(x)    do { if (x) { PIN_RW_PT->PSOR = PIN_RW; } else { PIN_RW_PT->PCOR = PIN_RW; } } while (0)
#define SET_LCD_RS(x)    do { if (x) { PIN_RS_PT->PSOR = PIN_RS; } else { PIN_RS_PT->PCOR = PIN_RS; } } while (0)

#define SET_LCD_DATA_OUT(x)                                                   \
    do {                                                                      \
        PIN_DATA_PT->PCOR = PINS_DATA;                                        \
        if ((x) & 0x01u) PIN_DATA_PT->PSOR = PIN_D4;                          \
        if ((x) & 0x02u) PIN_DATA_PT->PSOR = PIN_D5;                          \
        if ((x) & 0x04u) PIN_DATA_PT->PSOR = PIN_D6;                          \
        if ((x) & 0x08u) PIN_DATA_PT->PSOR = PIN_D7;                          \
    } while (0)

void Init_LCD(void);
void Set_Cursor(uint8_t column, uint8_t row);
void Clear_LCD(void);
void Print_LCD(char *string);
void lcd_putchar(char c);

#endif /* LCD_H */
