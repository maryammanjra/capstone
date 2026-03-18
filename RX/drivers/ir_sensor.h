/*
 * Hardware mapping (FRDM-KL25Z):
 *   Index   Pin     Port/GPIO   Active-low (0 = obstacle, 1 = clear)
 *     0     PTE3    PORTE bit3
 *     1     PTE2    PORTE bit2
 *     2     PTB11   PORTB bit11
 *     3     PTB10   PORTB bit10
 *     4     PTB9    PORTB bit9
 *     5     PTB8    PORTB bit8
 */

#ifndef IR_SENSOR_H
#define IR_SENSOR_H

#include "MKL25Z4.h"
#include "pin_config.h"     /* SIM clock gates, PORT_PCR_MUX macros */
#include "sensor_status.h"  /* IR_COUNT = 6 */

void ir_sensor_init(void);
void ir_sensor_read(uint8_t obs[IR_COUNT]);

#endif /* IR_SENSOR_H */
