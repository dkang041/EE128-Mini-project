/*
 * Copyright (c) 2015, Freescale Semiconductor, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * o Redistributions of source code must retain the above copyright notice, this list
 *   of conditions and the following disclaimer.
 *
 * o Redistributions in binary form must reproduce the above copyright notice, this
 *   list of conditions and the following disclaimer in the documentation and/or
 *   other materials provided with the distribution.
 *
 * o Neither the name of Freescale Semiconductor, Inc. nor the names of its
 *   contributors may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "fsl_device_registers.h"
#include <stdlib.h>

void software_delay(unsigned long delay) {
	while (delay > 0)
		delay--;
}

void sevenSegDis(unsigned int tempval);

volatile unsigned int nr_overflows = 0;

void FTM3_IRQHandler(void) {
	nr_overflows++;
	FTM3_SC &= 0x7F; // clear TOF
}

int main(void) {
	SIM_SCGC5 |= SIM_SCGC5_PORTB_MASK; /*Enable Port B Clock Gate Control*/
	PORTB_GPCHR = 0x001C0100;	//Configure Port B Pin 18-20 for GPIO;
	GPIOB_PDDR |= 0x001C0000;	//Configure Port B Pin 18-20 for Output
	PORTB_GPCLR = 0x0E0C0100; 	//Configure Port B Pin 2,3, 9-11 for GPIO;
	GPIOB_PDDR |= 0x00000800;//Configure Port B Pin 2, 3, 9, 10 for Input and 11 for Output;

	SIM_SCGC5 |= SIM_SCGC5_PORTC_MASK; /*Enable Port C Clock Gate Control*/
	PORTC_GPCLR = 0x01BF0100; 	//Configure Port C Pins 0-5 and 7-8 for GPIO;
	GPIOC_PDDR = 0x000001BF;	//Configure Port C Pins 0-5 and 7-8 for Output;

	SIM_SCGC5 |= SIM_SCGC5_PORTD_MASK; /*Enable Port D Clock Gate Control*/
	PORTD_GPCLR = 0x00FF0100; 	//Configure Port D Pins 0-7 for GPIO;
	GPIOD_PDDR = 0x000000FF; 	//Configure Port D Pins 0-7 for Output;

	SIM_SCGC5 |= SIM_SCGC5_PORTE_MASK; /*Enable Port E Clock Gate Control*/
	PORTE_GPCHR = 0x03000100; 	//Configure Port E Pins 24, 25 for GPIO;
	GPIOE_PDDR = 0x00000000; 	//Configure Port E Pins 24, 25 for Input;

	//timer initialization
	SIM_SCGC3 |= SIM_SCGC3_FTM3_MASK; // FTM3 clock enable
	FTM3_MODE = 0x5; // Enable FTM3
	FTM3_MOD = 0xFFFF;
	FTM3_SC = 0x0E; // System clock / 64
	NVIC_EnableIRQ(FTM3_IRQn); // Enable FTM3 interrupts
	FTM3_SC |= 0x40; // Enable TOF

	volatile int time = 0;
	volatile int finalscore1 = 0;
	volatile int finalscore2 = 99;
	volatile int currentscore = 0;
	volatile int moleLED[] = { 0, 0, 0, 0 };
	volatile int photo[] = { 0, 0, 0, 0 };
	volatile int moleflag = 0;
	volatile int tempval = 0;
	volatile int tempval2 = 0;
	volatile int molesleft = 0;
	volatile int LED = 0;

	for (;;) {
		unsigned long start = (~GPIOE_PDIR & 0x01000000);
		unsigned long gamemode = (~GPIOE_PDIR & 0x02000000);
		if (start != 0) {					//game started
			currentscore = 0;
			time = 3;
			while (time > 0) {				//countdown before game starts
				sevenSegDis(time);
				if (nr_overflows >= 5) {		//decreases time
					nr_overflows = 0;
					time--;
				}
			}
			if (gamemode == 0) {			//normal time-trial game mode
				time = 10;

				while (time > 0) {
					sevenSegDis(time);			//displays time

					//updates photoresistor input
					photo[0] = (GPIOB_PDIR & 0x04);
					photo[1] = (GPIOB_PDIR & 0x08);
					photo[2] = (GPIOB_PDIR & 0x0200);
					photo[3] = (GPIOB_PDIR & 0x0400);

					for (int i = 0; i < 4; i++) {	//check for active moles
						if (moleLED[i]) {
							moleflag = 1;
						}
					}

					if (moleflag == 0) {	//randomizes which mole to toggle
						tempval2 = tempval;
						tempval = rand() % 4;
						while (tempval == tempval2) {
							tempval = rand() % 4;
						}
						if (photo[tempval] <= 1) {
							moleLED[tempval] = 1;
							moleflag = 1;
						} else {//doesn't continue the game if any photoresistor is constantly covered
							while (photo[tempval] > 1) {
								photo[0] = (GPIOB_PDIR & 0x04);
								photo[1] = (GPIOB_PDIR & 0x08);
								photo[2] = (GPIOB_PDIR & 0x0200);
								photo[3] = (GPIOB_PDIR & 0x0400);
							}
							moleLED[tempval] = 1;
							moleflag = 1;
						}

						GPIOB_PDOR &= 0x00000000; /*Clear Port B*/

						//toggles LEDS on
						switch (tempval) {
						case 0:
							GPIOB_PDOR ^= 0x800;
							break;
						case 1:
							GPIOB_PDOR ^= 0x40000;
							break;
						case 2:
							GPIOB_PDOR ^= 0x80000;
							break;
						case 3:
							GPIOB_PDOR ^= 0x100000;
							break;
						}
						LED = 1;
					}
					//checks for covered photoresistor and toggles LED off
					if (photo[tempval] > 1 && moleLED[tempval]) {
						switch (tempval) {
						case 0:
							GPIOB_PDOR ^= 0x800;
							break;
						case 1:
							GPIOB_PDOR ^= 0x40000;
							break;
						case 2:
							GPIOB_PDOR ^= 0x80000;
							break;
						case 3:
							GPIOB_PDOR ^= 0x100000;
							break;
						}
						LED = 0;
						moleLED[tempval] = 0;
						moleflag = 0;
						currentscore++;
					}

					if (nr_overflows >= 5) {		//decreases time
						nr_overflows = 0;
						time--;
					}

				}
				if (LED = 1) {				//toggles led off if its still on
					switch (tempval) {
					case 0:
						GPIOB_PDOR ^= 0x800;
						break;
					case 1:
						GPIOB_PDOR ^= 0x40000;
						break;
					case 2:
						GPIOB_PDOR ^= 0x80000;
						break;
					case 3:
						GPIOB_PDOR ^= 0x100000;
						break;
					}
				}
				if (currentscore > finalscore1) {	//updates the highscore
					finalscore1 = currentscore;
				}
			} else {								//2nd game mode
				molesleft = 15;
				time = 0;

				while (molesleft > 0) {
					sevenSegDis(molesleft);			//displays time

					//updates photoresistor input
					photo[0] = (GPIOB_PDIR & 0x04);
					photo[1] = (GPIOB_PDIR & 0x08);
					photo[2] = (GPIOB_PDIR & 0x0200);
					photo[3] = (GPIOB_PDIR & 0x0400);

					for (int i = 0; i < 4; i++) {	//check for active moles
						if (moleLED[i]) {
							moleflag = 1;
						}
					}

					if (moleflag == 0) {	//randomizes which mole to toggle
						tempval2 = tempval;
						tempval = rand() % 4;
						while (tempval == tempval2) {
							tempval = rand() % 4;
						}
						if (photo[tempval] <= 1) {
							moleLED[tempval] = 1;
							moleflag = 1;
						} else {//doesn't continue the game if any photoresistor is constantly covered
							while (photo[tempval] > 1) {
								photo[0] = (GPIOB_PDIR & 0x04);
								photo[1] = (GPIOB_PDIR & 0x08);
								photo[2] = (GPIOB_PDIR & 0x0200);
								photo[3] = (GPIOB_PDIR & 0x0400);
							}
							moleLED[tempval] = 1;
							moleflag = 1;
						}

						GPIOB_PDOR &= 0x00000000; /*Clear Port B*/

						//toggles LEDS on
						switch (tempval) {
						case 0:
							GPIOB_PDOR ^= 0x800;
							break;
						case 1:
							GPIOB_PDOR ^= 0x40000;
							break;
						case 2:
							GPIOB_PDOR ^= 0x80000;
							break;
						case 3:
							GPIOB_PDOR ^= 0x100000;
							break;
						}
						LED = 1;
					}
					//checks for covered photoresistor and toggles LED off
					if (photo[tempval] > 1 && moleLED[tempval]) {
						switch (tempval) {
						case 0:
							GPIOB_PDOR ^= 0x800;
							break;
						case 1:
							GPIOB_PDOR ^= 0x40000;
							break;
						case 2:
							GPIOB_PDOR ^= 0x80000;
							break;
						case 3:
							GPIOB_PDOR ^= 0x100000;
							break;
						}
						LED = 0;
						moleLED[tempval] = 0;
						moleflag = 0;
						molesleft--;
					}

					if (nr_overflows >= 5) {		//decreases time
						nr_overflows = 0;
						time++;
						currentscore = time;
					}

				}
				if (LED = 1) {				//toggles led off if its still on
					switch (tempval) {
					case 0:
						GPIOB_PDOR ^= 0x800;
						break;
					case 1:
						GPIOB_PDOR ^= 0x40000;
						break;
					case 2:
						GPIOB_PDOR ^= 0x80000;
						break;
					case 3:
						GPIOB_PDOR ^= 0x100000;
						break;
					}
				}
				if (time < finalscore2) { 		//updates the highscore
					finalscore2 = time;
				}
			}
			moleflag = 0;
			time = 0;
			molesleft = 0;
			for (int i = 0; i < 4; i++) {//resets the moles and photoresistors
				photo[i] = 0;
				moleLED[i] = 0;
			}
			GPIOB_PDOR &= 0x00000000; /*Clear Port B*/
			time = 5;
			while (time > 0) {				//waits 5 seconds after game ends
				sevenSegDis(currentscore);
				if (nr_overflows >= 5) {		//decreases time
					nr_overflows = 0;
					time--;
				}
			}
			while (start != 0) {//restarts the game when DIP switch is toggled
				start = (~GPIOE_PDIR & 0x01000000);
				gamemode = (~GPIOE_PDIR & 0x02000000);
			}
		} else {//displays the highest score of the recently played game mode
			if (gamemode == 0) {
				sevenSegDis(finalscore1);
			} else {
				sevenSegDis(finalscore2);
			}
		}
	}
}

void sevenSegDis(unsigned int tempval) {
	//clear PORT D and PORT C
	GPIOC_PDOR &= 0x0000;
	GPIOD_PDOR &= 0x0000;

	/* PORT C and D to seven segments */
	unsigned int tempones = (tempval % 10);
	//Ones
	switch (tempones) {
	case 1:
		GPIOC_PDOR = 0x30;
		break;
	case 2:
		GPIOC_PDOR = 0xAD;
		break;
	case 3:
		GPIOC_PDOR = 0xB9;
		break;
	case 4:
		GPIOC_PDOR = 0x33;
		break;
	case 5:
		GPIOC_PDOR = 0x9B;
		break;
	case 6:
		GPIOC_PDOR = 0x9F;
		break;
	case 7:
		GPIOC_PDOR = 0xB0;
		break;
	case 8:
		GPIOC_PDOR = 0xBF;
		break;
	case 9:
		GPIOC_PDOR = 0xBB;
		break;
	default:
		GPIOC_PDOR = 0xBE;
		break;
	}

	unsigned int temptens = (tempval) / 10;
	//Tens
	switch (temptens) {
	case 1:
		GPIOD_PDOR = 0x30;
		break;
	case 2:
		GPIOD_PDOR = 0x6D;
		break;
	case 3:
		GPIOD_PDOR = 0x79;
		break;
	case 4:
		GPIOD_PDOR = 0x33;
		break;
	case 5:
		GPIOD_PDOR = 0x5B;
		break;
	case 6:
		GPIOD_PDOR = 0x5F;
		break;
	case 7:
		GPIOD_PDOR = 0x70;
		break;
	case 8:
		GPIOD_PDOR = 0x7F;
		break;
	case 9:
		GPIOD_PDOR = 0x7B;
		break;
	default:
		GPIOD_PDOR = 0x7E;
		break;
	}
}

////////////////////////////////////////////////////////////////////////////////
// EOF
////////////////////////////////////////////////////////////////////////////////
