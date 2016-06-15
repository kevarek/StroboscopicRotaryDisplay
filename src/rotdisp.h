/*
 * rotdisp.h
 *
 *  Created on: 14.9.2013
 *      Author: Standa
 */

#ifndef ROTDISP_H_
#define ROTDISP_H_

typedef struct {
	char Character;
	int AlphaStart;
	int AlphaEnd;
} CharStructTypedef;

typedef struct {
	int Beta;
	void (*LedSet)(int status);
	CharStructTypedef* DisplayedCharacter;
} LEDStructTypedef;



typedef struct {
	int LEDCount;
	LEDStructTypedef *LEDList;
	int CharCount;
	CharStructTypedef *CharList;
	unsigned *MatchValList;
} RotDispStructTypedef;


extern void rd_init(void);
extern unsigned rd_GetNextMatchValFromList(int restart);
#endif /* ROTDISP_H_ */
