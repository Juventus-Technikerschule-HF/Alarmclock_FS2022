/*
 * main.c
 *
 * Created: 08.03.2022 21:35:00
 * Author : ...
 */ 

#include "avr_compiler.h"
#include "pmic_driver.h"
#include "TC_driver.h"
#include "clksys_driver.h"
#include "sleepConfig.h"
#include "port_driver.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "event_groups.h"
#include "stack_macros.h"

#include "mem_check.h"

#include "init.h"
#include "utils.h"
#include "errorHandler.h"
#include "NHD0420Driver.h"
#include "ButtonHandler.h"


void vLedBlink(void *pvParameters);
void vTime(void *pvParameters);
void vButtonTask(void *pvParameters);
void vInterfaceTask(void *pvParameters);

#define MODE_SHOWTIME		0x00
#define MODE_SETTIME		0x01
#define MODE_SETALARM		0x02
#define MODE_ALARMACTIVE	0x03

//Eventgroup and Defines for Alarmclock
#define ALARMCLOCK_COUNT1SECOND		0x01 << 0
#define ALARMCLOCK_TESTBUTTONPRESS	0x01 << 1
#define ALARMCLOCK_BUTTON1_SHORT	1 << 2
#define ALARMCLOCK_BUTTON1_LONG		1 << 3
#define ALARMCLOCK_BUTTON2_SHORT	1 << 4
#define ALARMCLOCK_BUTTON2_LONG		1 << 5
#define ALARMCLOCK_BUTTON3_SHORT	1 << 6
#define ALARMCLOCK_BUTTON3_LONG		1 << 7
#define ALARMCLOCK_BUTTON4_SHORT	1 << 8
#define ALARMCLOCK_BUTTON4_LONG		1 << 9

#define ALARMCLOCK_BUTTON_RESET		0x03FC

EventGroupHandle_t egAlarmClock;

//Time-Variables
uint8_t seconds = 0;
uint8_t minutes = 0;
uint8_t hours = 0;

uint8_t alarm_seconds = 0;
uint8_t alarm_minutes = 0;
uint8_t alarm_hours = 0;


int main(void)
{
    resetReason_t reason = getResetReason();

	vInitClock();
	vInitDisplay();
	
	//Eventgroup Initialisieren (Bevor irgendwelche Tasks gestartet werden)
	egAlarmClock = xEventGroupCreate();
	
	//Task erstellen
	xTaskCreate(vLedBlink, (const char *) "ledBlink", configMINIMAL_STACK_SIZE+10, NULL, 1, NULL);
	xTaskCreate(vTime, (const char *) "timeTask", configMINIMAL_STACK_SIZE, NULL, 2, NULL);
	xTaskCreate(vButtonTask, (const char *) "buttonTask", configMINIMAL_STACK_SIZE+50, NULL, 1, NULL);
	xTaskCreate(vInterfaceTask, (const char *) "uiTask", configMINIMAL_STACK_SIZE + 200, NULL, 1, NULL);
	
	vTaskStartScheduler();
	return 0;
}

#define TIMESELECT_HOUR		1
#define TIMESELECT_MINUTES	2
#define TIMESELECT_SECONDS	3

void vInterfaceTask(void *pvParameters) {
	int mode = MODE_SHOWTIME;
	int timeSelect = TIMESELECT_HOUR;
	for(;;) {
		switch(mode) {
			case MODE_SHOWTIME:
				vDisplayClear();
				vDisplayWriteStringAtPos(0,0,"Time:   %d:%d:%d", hours, minutes, seconds);
				vDisplayWriteStringAtPos(1,0,"Alarm:  %d:%d:%d", alarm_hours, alarm_minutes, alarm_seconds);
				vDisplayWriteStringAtPos(3,0,"Set-T Set-A EN-A --");
				if((xEventGroupGetBits(egAlarmClock) & ALARMCLOCK_BUTTON1_SHORT) > 0) {
					mode = MODE_SETTIME;
				}
				if((xEventGroupGetBits(egAlarmClock) & ALARMCLOCK_BUTTON2_SHORT) > 0) {
					mode = MODE_SETALARM;
				}
			break;
			case MODE_SETTIME:
				vDisplayClear();
				vDisplayWriteStringAtPos(0,0,"SetTime: %d:%d:%d", hours, minutes, seconds);
				vDisplayWriteStringAtPos(3,0,"Sel Up Dwn back");
				switch(timeSelect) {
					case TIMESELECT_HOUR:
						vDisplayWriteStringAtPos(2,0,"Hours");
					break;
					case TIMESELECT_MINUTES:
						vDisplayWriteStringAtPos(2,0,"Minutes");
					break;
					case TIMESELECT_SECONDS:
						vDisplayWriteStringAtPos(2,0,"Seconds");
					break;
				}
				if((xEventGroupGetBits(egAlarmClock) & ALARMCLOCK_BUTTON1_SHORT) > 0) {
					if(timeSelect < TIMESELECT_SECONDS) {
						timeSelect++;
					} else {
						timeSelect = TIMESELECT_HOUR;
					}
				}
				if((xEventGroupGetBits(egAlarmClock) & ALARMCLOCK_BUTTON2_SHORT) > 0) {
					if(timeSelect == TIMESELECT_HOUR) {
						hours++;
					} else if(timeSelect == TIMESELECT_MINUTES) {
						minutes++;
					} else if (timeSelect == TIMESELECT_SECONDS) {
						seconds++;
					}
				}
				if((xEventGroupGetBits(egAlarmClock) & ALARMCLOCK_BUTTON3_SHORT) > 0) {
					if(timeSelect == TIMESELECT_HOUR) {
						hours--;
						} else if(timeSelect == TIMESELECT_MINUTES) {
						minutes--;
						} else if (timeSelect == TIMESELECT_SECONDS) {
						seconds--;
					}
				}
				if((xEventGroupGetBits(egAlarmClock) & ALARMCLOCK_BUTTON4_SHORT) > 0) {
					mode = MODE_SHOWTIME;
				}
			break;
			case MODE_SETALARM:
				vDisplayClear();	
				vDisplayWriteStringAtPos(1,0,"SetAlarm: %d:%d:%d", alarm_hours, alarm_minutes, alarm_seconds);
				vDisplayWriteStringAtPos(3,0,"Sel Up Dwn back");
				if((xEventGroupGetBits(egAlarmClock) & ALARMCLOCK_BUTTON4_SHORT) > 0) {
					mode = MODE_SHOWTIME;
				}
			break;
			case MODE_ALARMACTIVE:

			break;
			default:

			break;
		}

		xEventGroupClearBits(egAlarmClock, ALARMCLOCK_BUTTON_RESET);
		vTaskDelay(200/portTICK_RATE_MS);
	}
}

void vButtonTask(void * pvParameters) {
	initButtons();
	///vTaskDelay(3000);
	for(;;) {
		updateButtons();
		if(getButtonPress(BUTTON1) == SHORT_PRESSED) {
			xEventGroupSetBits(egAlarmClock, ALARMCLOCK_BUTTON1_SHORT);
		}
		if(getButtonPress(BUTTON2) == SHORT_PRESSED) {
			xEventGroupSetBits(egAlarmClock, ALARMCLOCK_BUTTON2_SHORT);
		}
		if(getButtonPress(BUTTON3) == SHORT_PRESSED) {
			xEventGroupSetBits(egAlarmClock, ALARMCLOCK_BUTTON3_SHORT);
		}
		if(getButtonPress(BUTTON4) == SHORT_PRESSED) {
			xEventGroupSetBits(egAlarmClock, ALARMCLOCK_BUTTON4_SHORT);
		}
		if(getButtonPress(BUTTON1) == LONG_PRESSED) {
			xEventGroupSetBits(egAlarmClock, ALARMCLOCK_BUTTON1_LONG);
		}
		if(getButtonPress(BUTTON2) == LONG_PRESSED) {
			xEventGroupSetBits(egAlarmClock, ALARMCLOCK_BUTTON2_LONG);
		}
		if(getButtonPress(BUTTON3) == LONG_PRESSED) {
			xEventGroupSetBits(egAlarmClock, ALARMCLOCK_BUTTON3_LONG);
		}
		if(getButtonPress(BUTTON4) == LONG_PRESSED) {
			xEventGroupSetBits(egAlarmClock, ALARMCLOCK_BUTTON4_LONG);
		}
		vTaskDelay(10/portTICK_RATE_MS);
	}
}

void vInitTimer() {
	TCD0.CTRLA = TC_CLKSEL_DIV1024_gc ;
	TCD0.CTRLB = 0x00;
	TCD0.INTCTRLA = 0x03;
	TCD0.PER = 31250-1;
}

void vTime(void *pvParameters) {
	vInitTimer();
	for(;;) {
		xEventGroupWaitBits(egAlarmClock, ALARMCLOCK_COUNT1SECOND, true, false, portMAX_DELAY);
		seconds++;
		if(seconds == 60) {
			seconds = 0;
			minutes++;
		}
		if(minutes == 60) {
			minutes = 0;
			hours++;
		}
		if(hours == 24) {
			hours = 0;
		}
	}
}

ISR(TCD0_OVF_vect) 
{	
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	xEventGroupSetBitsFromISR(egAlarmClock, ALARMCLOCK_COUNT1SECOND,&xHigherPriorityTaskWoken );
}

void vLedBlink(void *pvParameters) {
	(void) pvParameters;
	PORTF.DIRSET = PIN0_bm; /*LED1*/
	PORTF.OUT = 0x01;
	for(;;) {
		PORTF.OUTTGL = 0x01;				
		vTaskDelay(100 / portTICK_RATE_MS);
	}
}
