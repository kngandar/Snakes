#include <RTL.h>
#include <stdlib.h>
#include <stdio.h>
#include "LPC17xx.H"                    /* LPC17xx definitions*/
#include "GLCD.h"
#include "LED.h"
#include "KBD.h"
#include "ADC.h"

#define __SF				0			/* Score Font 6 x 8 */
#define __FI        		1           /* Font index 16x24 */

#define midX				160			/* Screen Midpoint x-direction */
#define midY				120			/* Screen Midpoint y-direction */
#define SSIZE_MAX			10			/* Maximum Snake Size */

// Different Status for LCDStatus or GameStatus
#define IDLE 				0
#define MAIN				1
#define RUNNING				2
#define GAME_END			3

#define PLAYING				4
#define PAUSE				5
#define OVER				6

// Snake movement direction
#define RIGHT				1
#define LEFT				2
#define UP					3
#define DOWN				4

// Pause menu direction
#define RESUME				1
#define QUIT				2

#define B					Black
#define W					White
#define SH					Cyan		// Snake Head
#define SB					DarkCyan	// Snake Body
#define F					Green		// Food
#define SP					Red			// Spike

OS_TID t_led;                           /* assigned task id of task: led */
OS_TID t_kbd;                           /* assigned task id of task: keyread */
OS_TID t_jst;                        	/* assigned task id of task: joystick */
OS_TID t_clock;                         /* assigned task id of task: clock   */
OS_TID t_lcd;                           /* assigned task id of task: lcd     */
OS_TID t_game;

OS_MUT mut_GLCD;                        /* Mutex to control GLCD access     */

 unsigned int ADCStat = 0;
 unsigned int ADCValue = 0;
  
 
 
/* ============================ */
/*        DATA STRUCTURE        */
/* ============================ */

 typedef struct {
	 unsigned int xpos;
	 unsigned int ypos;
 } snakeBlock_t;
 
 snakeBlock_t snake[SSIZE_MAX];
 
 
 
 /* ============================== */
 /*        GLOBAL VARIABLES        */
 /* ============================== */
 
 unsigned int gameSpeed_i[5] = {25,18,13,7,5};
 
/* Only one food on screen at a time, 
    Index: 0 xPosition, 1 yPosition */
 unsigned int food_i[2];
 
 /* Used to generate random positions, 
    Index: 0 xPosition, 1 yPosition */
 unsigned int rand_pos[2];
 
/* Two spikes on screen 
	Index: 0 xPosition, 1 yPosition */
 unsigned int spike1_i[2];
 unsigned int spike2_i[2];
	 
 unsigned int LCDStatusLast = IDLE;
 //unsigned int GameStatusLast = OVER;
 
 unsigned int moveDir;
 unsigned int moveDirLast;
 unsigned int snakeSize;
 
 unsigned int randTime = 0;
 char printBuffer [20];
 
 
 
 /* ================================ */
 /*        VOLATILE VARIABLES        */
 /* ================================ */
 
 volatile unsigned int LCDStatus = IDLE;
 volatile unsigned int GameStatus = OVER;
 volatile unsigned int decision = 0;
 volatile unsigned int select = RESUME;
 volatile unsigned int score = 0;
 volatile unsigned int gameSpeed;
 volatile unsigned int win = 0;	// Did user win game?
 
 
 
 /* =========================== */
 /*        GAME GRAPHICS        */
 /* =========================== */
 
	// 20 x 20 Block
	unsigned short snakeHead[] = {
			SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,
			SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,
			SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,
		  SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,
			SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,
			SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,
			SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,
			SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,
			SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,
		  SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,
			SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,
			SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,
			SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,
			SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,
			SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,
			SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,
			SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,
			SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,
			SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,
			SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH,SH
	};
	
	// 20 x 20 Block
	unsigned short snakeBody[] = {
			SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,
			SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,
			SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,
			SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,
			SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,
			SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,
			SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,
			SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,
			SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,
			SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,
			SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,
			SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,
			SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,
			SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,
			SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,
			SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,
			SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,
			SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,
			SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,
			SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,SB,		
	};
	
	// 20 x 20 Food
	unsigned short food[] = {
			F,F,F,F,F,F,F,F,F,F,F,F,F,F,F,F,F,F,F,F,
			F,F,F,F,F,F,F,F,F,F,F,F,F,F,F,F,F,F,F,F,
			F,F,F,F,F,F,F,F,F,F,F,F,F,F,F,F,F,F,F,F,
			F,F,F,F,F,F,F,F,F,F,F,F,F,F,F,F,F,F,F,F,
			F,F,F,F,B,B,B,B,B,B,B,B,B,B,B,B,F,F,F,F,
			F,F,F,F,B,B,B,B,B,B,B,B,B,B,B,B,F,F,F,F,
			F,F,F,F,B,B,B,B,B,B,B,B,B,B,B,B,F,F,F,F,
			F,F,F,F,B,B,B,B,B,B,B,B,B,B,B,B,F,F,F,F,
			F,F,F,F,B,B,B,B,B,B,B,B,B,B,B,B,F,F,F,F,
			F,F,F,F,B,B,B,B,B,B,B,B,B,B,B,B,F,F,F,F,
			F,F,F,F,B,B,B,B,B,B,B,B,B,B,B,B,F,F,F,F,
			F,F,F,F,B,B,B,B,B,B,B,B,B,B,B,B,F,F,F,F,
			F,F,F,F,B,B,B,B,B,B,B,B,B,B,B,B,F,F,F,F,
			F,F,F,F,B,B,B,B,B,B,B,B,B,B,B,B,F,F,F,F,
			F,F,F,F,B,B,B,B,B,B,B,B,B,B,B,B,F,F,F,F,
			F,F,F,F,B,B,B,B,B,B,B,B,B,B,B,B,F,F,F,F,
			F,F,F,F,F,F,F,F,F,F,F,F,F,F,F,F,F,F,F,F,
			F,F,F,F,F,F,F,F,F,F,F,F,F,F,F,F,F,F,F,F,
			F,F,F,F,F,F,F,F,F,F,F,F,F,F,F,F,F,F,F,F,
			F,F,F,F,F,F,F,F,F,F,F,F,F,F,F,F,F,F,F,F,
	};
	
	// 20 x 20 Spike
	unsigned short spike[] = {
			B,B,B,B,B,B,B,B,SP,SP,SP,SP,B,B,B,B,B,B,B,B,
			B,B,B,B,B,B,B,B,SP,SP,SP,SP,B,B,B,B,B,B,B,B,
			B,B,B,B,B,B,B,B,SP,SP,SP,SP,B,B,B,B,B,B,B,B,
			B,B,B,B,B,B,B,B,SP,SP,SP,SP,B,B,B,B,B,B,B,B,
			B,B,B,B,B,B,SP,SP,SP,SP,SP,SP,SP,SP,B,B,B,B,B,B,
			B,B,B,B,B,B,SP,SP,SP,SP,SP,SP,SP,SP,B,B,B,B,B,B,
			B,B,B,B,SP,SP,SP,SP,SP,SP,SP,SP,SP,SP,SP,SP,B,B,B,B,
			B,B,B,B,SP,SP,SP,SP,SP,SP,SP,SP,SP,SP,SP,SP,B,B,B,B,
			SP,SP,SP,SP,SP,SP,SP,SP,SP,SP,SP,SP,SP,SP,SP,SP,SP,SP,SP,SP,
			SP,SP,SP,SP,SP,SP,SP,SP,SP,SP,SP,SP,SP,SP,SP,SP,SP,SP,SP,SP,
		  SP,SP,SP,SP,SP,SP,SP,SP,SP,SP,SP,SP,SP,SP,SP,SP,SP,SP,SP,SP,
			SP,SP,SP,SP,SP,SP,SP,SP,SP,SP,SP,SP,SP,SP,SP,SP,SP,SP,SP,SP,
			B,B,B,B,SP,SP,SP,SP,SP,SP,SP,SP,SP,SP,SP,SP,B,B,B,B,
			B,B,B,B,SP,SP,SP,SP,SP,SP,SP,SP,SP,SP,SP,SP,B,B,B,B,
			B,B,B,B,B,B,SP,SP,SP,SP,SP,SP,SP,SP,B,B,B,B,B,B,
			B,B,B,B,B,B,SP,SP,SP,SP,SP,SP,SP,SP,B,B,B,B,B,B,
			B,B,B,B,B,B,B,B,SP,SP,SP,SP,B,B,B,B,B,B,B,B,
			B,B,B,B,B,B,B,B,SP,SP,SP,SP,B,B,B,B,B,B,B,B,
			B,B,B,B,B,B,B,B,SP,SP,SP,SP,B,B,B,B,B,B,B,B,
			B,B,B,B,B,B,B,B,SP,SP,SP,SP,B,B,B,B,B,B,B,B,
	};
	
	// 20 x 20 Block
	unsigned short clear[] = {
			B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,
			B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,
			B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,
		  B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,
			B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,
			B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,
			B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,
			B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,
			B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,
		  B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,
			B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,
			B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,
			B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,
			B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,
			B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,
			B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,
			B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,
			B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,
			B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,
			B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,
	};

	
/*----------------------------------------------------------------------------
  switch LED on
 *---------------------------------------------------------------------------*/
void LED_on  (unsigned char led) {
  LED_On (led); //turn on the physical LED
}


/*----------------------------------------------------------------------------
  switch LED off
 *---------------------------------------------------------------------------*/
void LED_off (unsigned char led) {
  LED_Off(led); //turn off the physical LED
}


/*----------------------------------------------------------------------------
  get OS time for random generator
 *---------------------------------------------------------------------------*/
void randomizer() {
	/* Randomly generates numbers for x and y position provided that that space
	is not already taken by spikes, food, or snake blocks
	*/
	unsigned int i = 0;
	unsigned int exists = 1;
		
	srand((unsigned int)os_time_get());
	
	while (exists) {
		exists = 0;
		
		rand_pos[0] = rand()%16*20;
		rand_pos[1] = rand()%12*20;
		
		// Make sure random block isn't on a snake block
		for (i=0; i<snakeSize; i++) {
			if (snake[i].xpos == rand_pos[0] && snake[i].ypos == rand_pos[1]) {
				exists = 1;
				break;
			}
		}
		
		// Make sure random block isn't on a spike block or food block
		if (spike1_i[0] == rand_pos[0] && spike1_i[1] == rand_pos[1]) {
			exists = 1;
			continue;
		}
		
		else if (spike2_i[0] == rand_pos[0] && spike2_i[1] == rand_pos[1]) {
			exists = 1;
			continue;
		}
		
		// Make sure random block isn't on a food block
		else if (food_i[0] == rand_pos[0] && food_i[1] == rand_pos[1]) {
			exists = 1;
			continue;
		}
	}
}


/*----------------------------------------------------------------------------
  Task 1 'LEDs': Controls game time and speed
 *---------------------------------------------------------------------------*/
__task void led (void) {
	int on = 7;
	int off = 0;
	int timeCounter = 0;
	int gameSpeedCounter = 0;

	// Game initialization
	LED_on (on);
	on--;	
	gameSpeed = gameSpeed_i[gameSpeedCounter];
	
		// Counts every 30 seconds (approx. 2600 clock ticks)
		while (on > 1 && GameStatus != OVER) {
			
			// Counting approx. every second
			while (timeCounter < 26) {
				os_dly_wait(100);
				timeCounter++;
				
				if(GameStatus == OVER) {
					break;
				}
				
				while (GameStatus == PAUSE) {
					// Do nothing when game is paused
				}
			}
			
			if(GameStatus == OVER) {
					break;
				}
			
			// Lights one LED up and increase game speed every 30 seconds
			LED_on (on);
			on--;
			timeCounter = 0;
			
			gameSpeedCounter++;
			gameSpeed = gameSpeed_i[gameSpeedCounter];			
		}	
		
		// Win Event: Game over when player survives for 3 minutes
		if (on < 2) {
			win = 1;
		}
	
		LCDStatus = GAME_END;
		GameStatus = OVER;
	
	// Turns off LEDs when after game over
	for (off = 0; off < 8; off++) {
		LED_off (off);
	}
	
	os_tsk_delete_self();
}


/*----------------------------------------------------------------------------
  Task 2 'keyread': process key stroke from int0 push button
 *---------------------------------------------------------------------------*/
__task void keyread (void) {
	uint32_t INT0_Press = 0;
	
  while (1) { 
		// If on MAIN page, go to RUNNING/start game
		if(LCDStatusLast == MAIN) {
			if(INT0_Get() == 0) {
				LCDStatus = RUNNING;
			}
		}
		
		// If on RUNNING page, game can be either PLAYING or PAUSED
			while (GameStatus!= OVER) {
				while (GameStatus == PLAYING) {
					if (INT0_Get() == 0) {
						INT0_Press = 1;
					}
					
					if(INT0_Press == 1) {  			
						GameStatus = PAUSE;
					}
				}
				
				// Changing this seems to affect how long pause menu will let me pick options before it hangs?
				os_dly_wait(50);
				
				INT0_Press = 0;
				
				// Waits for user response to resume or quit game after pausing game
				while(GameStatus == PAUSE) {
					if (INT0_Get() == 0) {
						INT0_Press = 1;
					}
					// When pressed decision is made
					if(INT0_Press == 1) {
						decision = 1;
						os_dly_wait(50);
					}
				}
				
					INT0_Press = 0;
				}
		}			
	}


/*----------------------------------------------------------------------------
  Task 3 'joystick': process an input from the joystick
 *---------------------------------------------------------------------------*/
 
/*NOTE: You need to complete this function*/
__task void joystick (void) {
	uint32_t jstickPress;
	
  while (GameStatus != OVER) {
			/* PLAYING MODE */
			// Joystick motion detects when game is playing	
			if (GameStatus == PLAYING) {				
				jstickPress = KBD_Get();
				if (moveDirLast == RIGHT || moveDirLast == LEFT) {	
					if(jstickPress == 113){
						moveDir = UP;
					} 
					if(jstickPress == 89){
						moveDir = DOWN;
					}
				}	
				if(moveDirLast == UP || moveDirLast == DOWN) {	
					if (jstickPress == 57){
						moveDir = LEFT;
					}
					if(jstickPress == 105){
						moveDir = RIGHT;
					}				
				}
			}
			
			/* PAUSE MODE */
			else if(GameStatus == PAUSE){
				// Pick between  two options only RESUME/QUIT				
				jstickPress = KBD_Get();
				if(jstickPress == 113){
					select = RESUME;
				} 
				if(jstickPress == 89){
					select = QUIT;
				}
		}
	
	}
	os_tsk_delete_self();
}


/*----------------------------------------------------------------------------
  Task 3 'game': Game animation task
 *---------------------------------------------------------------------------*/
__task void game(void){
	// Snake init
	unsigned int i;
	unsigned int grow = 0;
	int xpos; 
	int ypos; 
	int xposPrev;
	int yposPrev;
	
	// Food & Spike init
	unsigned int foodExist = 1;
	unsigned int stepCount = 0;
	
	// Death Flag init
	unsigned int hitSpike = 0;
	unsigned int closedLoop = 0;
	
	/* GAME INITIALIZATION */
	unsigned int last_select = QUIT;
	os_mut_wait(mut_GLCD, 0xffff);	
	GLCD_SetBackColor(B);
	GLCD_SetTextColor(W);
	
	// Snake & Movement reset
	moveDir = RIGHT;
	moveDirLast = RIGHT;
	snakeSize = 1;
	snake[0].xpos = midX;
	snake[0].ypos = midY; 
	GLCD_Bitmap (midX, midY, 20, 20, (unsigned char*)snakeHead);
	
	// Food reset
	randomizer();
	food_i[0] = rand_pos[0];
	food_i[1] = rand_pos[1];
	GLCD_Bitmap (food_i[0], food_i[1], 20, 20, (unsigned char*)food);

	// Spikes reset
	randomizer();
	spike1_i[0] = rand_pos[0];
	spike1_i[1] = rand_pos[1];
	GLCD_Bitmap (spike1_i[0], spike1_i[1], 20, 20, (unsigned char*)spike);
	
	randomizer();
	spike2_i[0] = rand_pos[0];
	spike2_i[1] = rand_pos[1];
	GLCD_Bitmap (spike2_i[0], spike2_i[1], 20, 20, (unsigned char*)spike);
	
	// Flag Reset
	decision = 0;
	win = 0;
	select = RESUME;
	
	// Score Reset
	score = 0;
	GLCD_DisplayString(0,0, __SF, "SCORE:");
	sprintf(printBuffer, "%d", score);
	GLCD_DisplayString(1,0, __SF, printBuffer);
	
	
	while (GameStatus != OVER) {
		while (GameStatus == PLAYING){
			
			/* SNAKE ANIMATION CONTROL */
			xpos = snake[0].xpos;
			ypos = snake[0].ypos;
			xposPrev = xpos;
			yposPrev = ypos;
				
				/* CAUSE */
				// Moves snake head according to joystick press
				if (moveDir == RIGHT){
					xpos = (xpos + 20)%320;
				}					
				else if (moveDir == LEFT) {
					xpos = (xpos - 20)%320;
					if(xpos < 0){
						xpos = 320 - (xpos)*(-1);
					}
				}					
				else if (moveDir == DOWN) {
					ypos = (ypos + 20)%240;
				}					
				else if (moveDir == UP) {	
					ypos = (ypos - 20)%240;
					if(ypos < 0){
						ypos = 240 - (ypos)*(-1);
					}
				}

				snake[0].xpos = xpos;
				snake[0].ypos = ypos;
				
				GLCD_Bitmap (xpos, ypos, 20, 20, (unsigned char*)snakeHead);
				if (snakeSize == 1) { //Clear previous location only if there's no body
					GLCD_Bitmap (xposPrev, yposPrev, 20, 20, (unsigned char*)clear);
				}
				
				moveDirLast = moveDir;
				
				/* SNAKE BODY FOLLOWING ANIMATION*/
				// If snake has body have sprites follow head sprites
				if (snakeSize > 1 && GameStatus == PLAYING) {
					for(i = 1; i < snakeSize; i++) {
						xpos = xposPrev;
						ypos = yposPrev;
						xposPrev = snake[i].xpos;
						yposPrev = snake[i].ypos;
						
						GLCD_Bitmap (xpos, ypos, 20, 20, (unsigned char*)snakeBody);
						GLCD_Bitmap (xposPrev, yposPrev, 20, 20, (unsigned char*)clear);
						
						// Redraw position of snake head for special glitch when head disappears near tail
						if (i == snakeSize-1 && xposPrev == snake[0].xpos && yposPrev == snake[0].ypos) {
							GLCD_Bitmap (xpos, ypos, 20, 20, (unsigned char*)snakeHead);
						}
						
						// Write over new location
						snake[i].xpos = xpos;
						snake[i].ypos = ypos;		
									
					}
				}
				
				/* EVENT CHECKER */
				// Checks if snake eats food; snake head position == food position
				if ((snake[0].xpos == food_i[0]) && (snake[0].ypos == food_i[1])) {
					foodExist = 0;
					grow = 1;		
					score = score + 20;
				}
				
				// If snake head == spike1 or spike2, then snake died
				if ((snake[0].xpos == spike1_i[0]  && snake[0].ypos == spike1_i[1]) || (snake[0].xpos == spike2_i[0]  && snake[0].ypos == spike2_i[1])) {
					hitSpike = 1;
				}
				
				// If snake ever eats itself, then snake died
				if (snakeSize >= 5){
					for (i = 4; i < snakeSize; i++) {
						if (snake[0].xpos == snake[i].xpos && snake[0].ypos == snake[i].ypos) {
							closedLoop = 1;
						}
					}
				}	
								
				// Lose Event
				if (hitSpike == 1 || closedLoop == 1) {				
					GameStatus = OVER;
					LCDStatus = GAME_END;
				}
			
				/* EFFECT */
				
				/* GROW SNAKE UPDATE*/
				if (GameStatus == PLAYING) {
					// If food is eaten, snake should grow
					if (grow == 1 && snakeSize < SSIZE_MAX) { 
						snake[snakeSize].xpos = xposPrev;
						snake[snakeSize].ypos = yposPrev;
						snakeSize++;
						GLCD_Bitmap (xposPrev, yposPrev, 20, 20, (unsigned char*)snakeBody);	
						grow = 0;
					}
					
					/* FOOD GENERATOR */		
					// Generates food every time food is not on screen
					if (foodExist == 0) {
						
						// Randomly generate a food block
						randomizer();
						food_i[0] = rand_pos[0];
						food_i[1] = rand_pos[1];
						
						GLCD_Bitmap (food_i[0], food_i[1], 20, 20, (unsigned char*)food);
						
						foodExist = 1;
					}
					
					/* SPIKE GENERATOR */
					// Generates spikes every 80 moves 
					if (stepCount == 80) {
						GLCD_Bitmap (spike1_i[0], spike1_i[1], 20, 20, (unsigned char*)clear);
						GLCD_Bitmap (spike2_i[0], spike2_i[1], 20, 20, (unsigned char*)clear);
						
	 					randomizer();
						spike1_i[0] = rand_pos[0];
						spike1_i[1] = rand_pos[1];
						randomizer();
						spike2_i[0] = rand_pos[0];
						spike2_i[1] = rand_pos[1];

						
						GLCD_Bitmap (spike1_i[0], spike1_i[1], 20, 20, (unsigned char*)spike);
						GLCD_Bitmap (spike2_i[0], spike2_i[1], 20, 20, (unsigned char*)spike);
						stepCount = 0;
					}		
				
				}
				
				stepCount++;
				GLCD_DisplayString(0,1, __SF, "SCORE:");
				sprintf(printBuffer, "%d", score); 
				GLCD_DisplayString(1,1, __SF, printBuffer);
				
				os_dly_wait(gameSpeed);
			}
			
			while (GameStatus == PAUSE) {
				
				GLCD_DisplayString(2, 0, __FI, "    P A U S E D     ");
				GLCD_DisplayString(3, 0, __FI, "                   ");
				GLCD_DisplayString(4, 0, __FI, "      Resume       ");
				GLCD_DisplayString(5, 0, __FI, "     Quit Game     ");

				last_select = QUIT;
				/* TOGGLING BETWEEN RESUME OR QUIT GAME */
				while (decision == 0) {
					// It's done with a last_resume variable so that LCD is only written to when option is changed
					// This makes sure it doesn't hang by running out of memory and repeatedly writing to GLCD
					// Just toggle between options (resume or quit) based on joystick input while decision not made
					
					if (last_select != select) {
						if (select == RESUME){
							GLCD_DisplayString(5, 2, __FI, "  ");
							GLCD_DisplayString(4, 2, __FI, "->");
							last_select = RESUME;
						}
						else if (select == QUIT) {
							GLCD_DisplayString(4, 2, __FI, "  ");
							GLCD_DisplayString(5, 2, __FI, "->");
							last_select = QUIT;
						}
					}                          
				}
				
				/* AFTER DECISION */
				/* RESUMES GAME */
				if (select == RESUME) {
					// Redraw what happened last
					
					// Clear Resume Screen
					GLCD_DisplayString(2, 0, __FI, "                   ");
					GLCD_DisplayString(3, 0, __FI, "                   ");
					GLCD_DisplayString(4, 0, __FI, "                   ");
					GLCD_DisplayString(5, 0, __FI, "                   ");
					
					// Snake				
					GLCD_Bitmap (snake[0].xpos, snake[0].ypos, 20, 20, (unsigned char*)snakeHead);
					for(i = 1; i < snakeSize; i++) {
						GLCD_Bitmap (snake[i].xpos, snake[i].ypos, 20, 20, (unsigned char*)snakeBody);
					}
					
					// Food & Spikes
					GLCD_Bitmap (food_i[0], food_i[1], 20, 20, (unsigned char*)food);
					GLCD_Bitmap (spike1_i[0], spike1_i[1], 20, 20, (unsigned char*)spike);
					GLCD_Bitmap (spike2_i[0], spike2_i[1], 20, 20, (unsigned char*)spike);
					
					select = RESUME;
					GameStatus = PLAYING;
					decision = 0;
				}
				
				/* QUIT GAME */
				else {
					GameStatus = OVER;
					LCDStatus = GAME_END;
				}
			}	
		}	
		
	// Done with LCD only after game is over
	os_mut_release(mut_GLCD);
	os_tsk_delete_self();		
}
	
	
/*----------------------------------------------------------------------------
  Task 4 'lcd': LCD Control task
 *---------------------------------------------------------------------------*/
__task void lcd (void) {	
	
		for (;;) {	
			
			// If idle, do nothing
			if (LCDStatus == IDLE) {
				// Do nothing
			}
			
			// If interrupted, change screens
			else {
				GLCD_Clear(B);
				
				if (LCDStatus == MAIN) {
					os_mut_wait(mut_GLCD, 0xffff);
					GLCD_SetBackColor(B);
					GLCD_SetTextColor(W);
					
					GLCD_DisplayString(2, 0, __FI, "     Welcome To      ");
					GLCD_DisplayString(4, 0, __FI, "    S N A K E S    ");
					GLCD_DisplayString(8, 0, __FI, "   Push to start   ");
					LCDStatus = IDLE;
					LCDStatusLast = MAIN;
					os_mut_release(mut_GLCD);
				}
			
				else if(LCDStatus == RUNNING){					
					LCDStatus = IDLE;
					LCDStatusLast = RUNNING;
					GameStatus = PLAYING;
					
					
					t_led 	 = os_tsk_create (led, 0);		 		/* start the led task */
					t_jst		 = os_tsk_create (joystick, 0);     /* start the joystick task */
					t_game = os_tsk_create (game, 0);
				}
				
				else if(LCDStatus == GAME_END){
					os_mut_wait(mut_GLCD, 0xffff);
					GLCD_SetBackColor(B);
					GLCD_SetTextColor(W);
					GLCD_Clear(B);
					
					if(!win){
						GLCD_DisplayString(2, 0, __FI, "  Y O U  L O S E   ");
						GLCD_DisplayString(4, 0, __FI, "       Score:      ");
						sprintf(printBuffer, "%d", score);
						GLCD_DisplayString(5, 8, __FI, printBuffer);
					}
					
					else if(win){
						GLCD_DisplayString(2, 0, __FI, "  Y O U  W O N !   ");
						GLCD_DisplayString(4, 0, __FI, "       Score:      ");
						sprintf(printBuffer, "%d", score);
						GLCD_DisplayString(5, 8, __FI, printBuffer);
					}
					
					// Leave display for approx. 5 secs and refreshes game
					os_dly_wait (500);
					LCDStatus = MAIN;
					LCDStatusLast = MAIN;
					os_mut_release(mut_GLCD);
				}			
			}
			os_dly_wait (5);
		}
}


/*----------------------------------------------------------------------------
  Task 6 'init': Initialize game
 *---------------------------------------------------------------------------*/
/* NOTE: Add additional initialization calls for your tasks here */
__task void init (void) {

  os_mut_init(mut_GLCD);

	LCDStatus = MAIN;
	LCDStatusLast = MAIN;

	t_kbd		 = os_tsk_create (keyread, 0);		/* start the kbd task */
	t_lcd    = os_tsk_create (lcd, 0);     		 	/* start task lcd */
	
  os_tsk_delete_self ();
}


/*----------------------------------------------------------------------------
  Main: Initialize and start RTX Kernel
 *---------------------------------------------------------------------------*/
int main (void) {
		
	NVIC_EnableIRQ( ADC_IRQn ); 	/* Enable ADC interrupt handler  */
	
	LED_Init ();                	/* Initialize the LEDs           */
	GLCD_Init();                	/* Initialize the GLCD           */
	KBD_Init ();                	/* initialize Push Button        */
	ADC_Init ();					/* initialize the ADC            */	
	GLCD_Clear(B);              	/* Clear the GLCD                */
		
	os_sys_init(init);          	/* Initialize RTX and start init */
}