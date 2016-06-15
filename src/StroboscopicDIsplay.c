/*
 * StroboscopicDIsplay.c
 *
 *  Created on: 19.9.2013
 *      Author: Standa
 */

#include "StroboscopicDisplay.h"
#include "LPC11Uxx.h"
#include "gpio.h"
#include "timer32.h"

//function declarations



//////////////////////////////////////////////////////////
///////////DISPLAY DEFINITIONS////////////////////////////
//////////////////////////////////////////////////////////


unsigned MatchValBuffer[2];

#define LEFTBORDERSHIFT	1
#define RIGHTBORDERSHIFT 0
CharStructTypedef StroboscopicDispChars[] = {
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

LEDStructTypedef StroboscopicDispLEDs[] = {
		{ 180, &(StroboscopicDispChars[0]), &Led180SetValue },
};


#define CHARSINLIST	sizeof(StroboscopicDispChars)/sizeof(StroboscopicDispChars[0])
#define LEDSINLIST	sizeof(StroboscopicDispLEDs)/sizeof(StroboscopicDispLEDs[0])
StroboscopicDispStructTypedef StroboscopicDispStruct = {
		StroboscopicDispLEDs, LEDSINLIST, StroboscopicDispChars, CHARSINLIST, MatchValBuffer,
};



/////////////////////////////////////////////////////////////////
/////////HARDWARE ABSTRACTION LAYER//////////////////////////////
/////////////////////////////////////////////////////////////////

void timer_Init(void) {
	// Init timer for counting upwards with compare interrupt
	// You have to make sure that counter wont overflow during one clock revolution
	LPC_SYSCON->SYSAHBCLKCTRL |= (1 << 9);		//enable peripheral clock
	LPC_CT32B0->TCR = 0x02;						//reset timer
	LPC_CT32B0->PR = 0x00;						//set prescaler to zero
	LPC_CT32B0->MCR = 0x01;						//interrupt on match in match register 0
	NVIC_EnableIRQ(TIMER_32_0_IRQn);			//enable timer interrupts
	LPC_CT32B0->IR = 0xff;						//reset all  t0 interrrupts
}


int LedStateAfterNextTimerInterrupt;
void timer_ISRHandler(void){
	sdisp_GetActiveLED(0)->LedSet(LedStateAfterNextTimerInterrupt);	//in led definition there is pointer to function which sets led state
	timer_UpdateMatch(StroboscopicDispStruct.MatchValBuffer[1]);	//load time to switch off led into timer
	LedStateAfterNextTimerInterrupt = 0;
	LPC_CT32B0->IR  = 0xff;		/* reset all t0 interrupts */
}


unsigned timer_ReadActual(void){
	return LPC_CT32B0->TC;						//read actual value in timer counter
}


void timer_RestartWithMatch(int matchVal){
	LPC_CT32B0->TCR = 0x02; 					//reset timer
	LPC_CT32B0->MR0 = matchVal;					//set match value
	LPC_CT32B0->TCR = 0x01;						//start counting
}

void timer_UpdateMatch(unsigned matchVal){
	LPC_CT32B0->MR0 = matchVal;
}



#define DIRINPUT	0
#define DIROUTPUT	1
#define PORT		1
#define REVCNTPIN	15
#define LED180PIN	14
void gpio_Init(void){
	//Init selected GPIO pins as:
	// 		Input pin for revolutions counting - input, set interrupt on both edges
	//		Output pins for each led of display - just set output and low level for each pin
	GPIOInit();											//Init GPIO
	GPIOSetDir( PORT, REVCNTPIN, DIRINPUT );			//set P1_15 as input for rev counting
	GPIOSetFlexInterrupt(0, PORT, REVCNTPIN, 0, 0);		//channel 0, port 1 pin 15, edge triggered interrupt
	GPIOFlexIntEnable(0, 0);
	NVIC_EnableIRQ(FLEX_INT0_IRQn);						//enable interrupts for GPIO


	GPIOSetDir( PORT, LED180PIN, DIROUTPUT );			//set led at p1_14 as output
	GPIOSetBitValue( PORT, LED180PIN, 0 );				//and clear output
}

void gpio_ISRHandler(void){
	static int i = 0, index = 0;
	if(GPIOGetPinValue(PORT, REVCNTPIN) == 0){			//check pin state - > reacting only on falling edge
		sdisp_PrepNextRev(timer_ReadActual());

		if(i > 60) {
			i = 0;
			//dodelat sekvenci
			StroboscopicDispStruct.LEDList[0].DisplayedCharacter = &(StroboscopicDispStruct.CharList[index++]);
			if(index>10) index = 0;
		}

		i++;

	}

	LPC_GPIO_PIN_INT->IST = 0x1<<0;
  return;

}

void Led180SetValue(int val){
	GPIOSetBitValue( PORT, LED180PIN, val );
}





/////////////////////////////////////////////////////////////////
/////////HARDWARE INDEPENDENT PART///////////////////////////////
/////////////////////////////////////////////////////////////////



//initialize timer, gpio and restart timer with maximum value loaded in match register
void sdisp_init(void){
	timer_Init();
	gpio_Init();
	timer_RestartWithMatch(0xFFFFFFFF);
}

//prepares all for next revolution
//called from revolution counter input ISR
void sdisp_PrepNextRev(unsigned lastPeriod){
	if (lastPeriod == 0) return;											//return if wheel is not spinning or first revolution
	sdisp_PrepMatchList(lastPeriod);										//prepare match list for timer
	timer_RestartWithMatch( StroboscopicDispStruct.MatchValBuffer[0] );		//load first value from match list, load it to match register and restart counter
	LedStateAfterNextTimerInterrupt = 1;									//first interrupt will turn on led
}


//prepare match values for next revolution
#define SHIFT (int)7
void sdisp_PrepMatchList(unsigned lastPeriod){
	float tmp;
	LEDStructTypedef* led = sdisp_GetActiveLED(1);						//get actual led and advance
	tmp = led->Beta - led->DisplayedCharacter->AlphaEnd + SHIFT;		// calculate match value for led on timing
	if ( tmp < 0) {
		tmp += 360;
	}
	tmp /= 360.0;
	StroboscopicDispStruct.MatchValBuffer[0] = tmp * lastPeriod;					// and save

	tmp = led->Beta - led->DisplayedCharacter->AlphaStart + SHIFT;		// calculate match value for led off timing
	if ( tmp < 0) {
		tmp += 360;
	}
	tmp /= 360.0;
	StroboscopicDispStruct.MatchValBuffer[1] = tmp * lastPeriod;					// and save
}

LEDStructTypedef* sdisp_GetActiveLED(int alsoAdvance){
	static int i = 0;
	if (i >= StroboscopicDispStruct.LEDCount) i = 0;
	if(alsoAdvance){
		return &(StroboscopicDispStruct.LEDList[i++]);
	}
	else {
		return &(StroboscopicDispStruct.LEDList[i]);
	}

}
