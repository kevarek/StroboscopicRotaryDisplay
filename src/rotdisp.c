#include "rotdisp.h"
#include "mytimer.h"
#include "mygpio.h"




unsigned MatchValList[2];

#define LEFTBORDERSHIFT	1
#define RIGHTBORDERSHIFT 0
CharStructTypedef RotDispChars[] = {
	{':', 45 - LEFTBORDERSHIFT, 45 + RIGHTBORDERSHIFT},
	{'0', 68 - LEFTBORDERSHIFT, 68 + RIGHTBORDERSHIFT},
	{'1', 90 - LEFTBORDERSHIFT, 90 + RIGHTBORDERSHIFT},
	{'2', 113 - LEFTBORDERSHIFT, 113 + RIGHTBORDERSHIFT},
	{'3', 135 - LEFTBORDERSHIFT, 135 + RIGHTBORDERSHIFT},
	{'4', 158 - LEFTBORDERSHIFT, 158 + RIGHTBORDERSHIFT},
	{'5', 180 - LEFTBORDERSHIFT, 180 + RIGHTBORDERSHIFT},
	{'6', 203 - LEFTBORDERSHIFT, 203 + RIGHTBORDERSHIFT},
	{'7', 225 - LEFTBORDERSHIFT, 225 + RIGHTBORDERSHIFT},
	{'8', 248 - LEFTBORDERSHIFT, 248 + RIGHTBORDERSHIFT},
	{'9', 270 - LEFTBORDERSHIFT, 270 + RIGHTBORDERSHIFT},
};

LEDStructTypedef RotDispLEDs[] = {
		{ 180, &(RotDispChars[1]) },
};



RotDispStructTypedef RotDispStruct = {
		1, RotDispLEDs, 11, RotDispChars, MatchValList,
};



void timer_init(void) {
	// Init timer for counting upwards with compare interrupt
	// You have to make sure that counter wont overflow during one clock revolution
	LPC_SYSCON->SYSAHBCLKCTRL |= (1 << 9);		//enable peripheral clock
	LPC_CT32B0->TCR = 0x02;						//reset timer
	LPC_CT32B0->PR = 0x00;						//set prescaler to zero
	LPC_CT32B0->MCR = 0x01;						//interrupt on match in match register 0
	NVIC_EnableIRQ(TIMER_32_0_IRQn);			//enable timer interrupts
	LPC_CT32B0->IR = 0xff;						//reset all  t0 interrrupts
}

void gpio_init(void){
	// Init selected GPIO pins as:
	// 		Input pin for revolutions counting - input, set interrupt on both edges
	// 		Output pins for each led of display - just outputs
	GPIOInit();									// Init GPIO
	GPIOSetDir( 1, 15, 0 );						// set P1_15 as input for rev counting
	GPIOSetFlexInterrupt(0, 1, 15, 0, 0);		// channel 0, port 1 pin 15, edge triggered interrupt
	GPIOFlexIntEnable(0, 0);
	NVIC_EnableIRQ(FLEX_INT0_IRQn);				// enable interrupts for GPIO

	/*GPIOSetDir( 0, 7, 1 );						//set led at p0_7 as output
	GPIOSetBitValue( 0, 7, 1 );*/

	GPIOSetDir( 1, 14, 1 );						// set led at p1_14 as output
	GPIOSetBitValue( 1, 14, 0 );				// and clear output
}


unsigned mytimer0_ReadActual(void){

	return LPC_CT32B0->TC;
}



void mytimer0_RestartWithMatch(int matchVal){
	LPC_CT32B0->TCR = 0x02; /* reset timer */
	LPC_CT32B0->MR0 = matchVal;
	LPC_CT32B0->TCR = 0x01; /* start timer */
}

void mytimer0_UpdateMatch(int matchVal){
	LPC_CT32B0->MR0 = matchVal;
}



// timer interrupt service routine handler
void TIMER32_0_IRQHandler(void){
	static int i = 1;
	GPIOSetBitValue( 1, 14, i );
	i ^= 1;
	mytimer0_UpdateMatch(rd_GetNextMatchValFromList(0));
	LPC_CT32B0->IR  = 0xff;		/* reset all t0 interrupts */
}



void Led1Set(int status){
	GPIOSetBitValue( 1, 14, status );
}



/////////////////////////////////////////////////////////////////
/////////HARDWARE INDEPENDENT PART///////////////////////////////
/////////////////////////////////////////////////////////////////



// initialize timer, gpio and restart timer with maximum value loaded in match register
void rd_init(){
	timer_init();
	gpio_init();
	mytimer0_RestartWithMatch(0xFFFFFFFF);
}


// Prepare match values for next revolution
#define SHIFT (int)7
void rd_PrepMatchList(unsigned lastPeriod, LEDStructTypedef* led){
	int i, index = 0;
	float tmp;

	tmp = led->Beta - led->DisplayedCharacter->AlphaEnd + SHIFT;		// calculate match value for led on timing
	if ( tmp < 0) {
		tmp += 360;
	}
	tmp /= 360.0;
	RotDispStruct.MatchValList[index++] = tmp * lastPeriod;				// and save

	tmp = led->Beta - led->DisplayedCharacter->AlphaStart + SHIFT;		// calculate match value for led off timing
	if ( tmp < 0) {
		tmp += 360;
	}
	tmp /= 360.0;
	RotDispStruct.MatchValList[index++] = tmp * lastPeriod;				// and save
}


// get next match value with restart option
unsigned rd_GetNextMatchValFromList(int restart){
	static int i = 0;

	if(restart){
		i = 0;
	}
	if( i >= ( 2 * RotDispStruct.LEDCount ) ) return 0xFFFFFFFF;

	return RotDispStruct.MatchValList[i++];
}


// prepares all for next revolution
// called from revolution counter input ISR
void rd_PrepNextRev(unsigned lastPeriod){
	if (lastPeriod == 0) return;									// return if wheel is not spinning or first revolution
	rd_PrepMatchList(lastPeriod);									// prepare match list for timer
	mytimer0_RestartWithMatch( rd_GetNextMatchValFromList(1) );		// load first value from match list, load it to match register and restart counter
}
