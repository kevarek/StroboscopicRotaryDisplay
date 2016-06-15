/*
 * StroboscopicDisplay.h
 *
 *  Created on: 19.9.2013
 *      Author: Standa
 */

#ifndef STROBOSCOPICDISPLAY_H_
#define STROBOSCOPICDISPLAY_H_


typedef struct {
	char Character;
	int AlphaStart;
	int AlphaEnd;
} CharStructTypedef;

typedef struct {
	int Beta;
	CharStructTypedef* DisplayedCharacter;
	void (*LedSet)(int newState);
} LEDStructTypedef;



typedef struct {
	LEDStructTypedef *LEDList;
	int LEDCount;
	CharStructTypedef *CharList;
	int CharCount;
	unsigned *MatchValBuffer;
} StroboscopicDispStructTypedef;

//private functions
void Led180SetValue(int val);
unsigned timer_ReadActual(void);
void timer_UpdateMatch(unsigned matchVal);
void sdisp_PrepMatchList(unsigned lastPeriod);
void sdisp_PrepNextRev(unsigned lastPeriod);
LEDStructTypedef* sdisp_GetActiveLED(int alsoAdvance);

//public definitions
extern void gpio_ISRHandler(void);
extern void timer_ISRHandler(void);
extern void sdisp_init(void);

extern StroboscopicDispStructTypedef StroboscopicDispStruct;

#endif /* STROBOSCOPICDISPLAY_H_ */
