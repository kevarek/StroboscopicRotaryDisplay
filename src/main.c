/*
 * main.c
 *
 *  Created on: 23.12.2012
 *      Author: Standa
 */

#include "LPC11Uxx.h"
#include "crazyclock.h"
#include "rotdisp.h"

/* Main Program */
int main(void) {
	SystemCoreClockUpdate(); //initialize mcu clocks

	rd_init();

	while (1) {

		//__WFI();			//wait for interrupt


	}
}
