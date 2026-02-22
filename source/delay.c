#include <MKL25Z4.H>

void delay (uint32_t dly) {
  volatile uint32_t t;

	for (t=dly*10000; t>0; t--)
		;
}
