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
#include "rtos_buttonhandler.h"


void vLedBlink(void *pvParameters);
void vTime(void *pvParameters);
void vButtonTask(void *pvParameters);
void vInterfaceTask(void *pvParameters);


//Eventgroup and Defines for Alarmclock
#define ALARMCLOCK_COUNT1SECOND		0x01 << 0
#define ALARMCLOCK_TESTBUTTONPRESS	0x01 << 1
EventGroupHandle_t egAlarmClock;

//Time-Variables
uint8_t seconds = 0;
uint8_t minutes = 0;
uint8_t hours = 0;


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

void vInterfaceTask(void *pvParameters) {
	
	for(;;) {
		if((xEventGroupGetBits(egAlarmClock) & ALARMCLOCK_TESTBUTTONPRESS)  == ALARMCLOCK_TESTBUTTONPRESS) {
			//Ausgabe der Zeit wenn die Taste 1 gedrückt wird. Dies ist nur ein Beispiel für die Anwendung der EventGroup
			vDisplayClear();
			vDisplayWriteStringAtPos(0,0,"Time: %d:%d:%d", hours, minutes, seconds);
			xEventGroupClearBits(egAlarmClock, ALARMCLOCK_TESTBUTTONPRESS);
		}
	}
}

void vButtonTask(void * pvParameters) {
	initButtonHandler();
	setupButton(BUTTON1, &PORTF, 4, 1);
	setupButton(BUTTON2, &PORTF, 5, 1);
	setupButton(BUTTON3, &PORTF, 6, 1);
	setupButton(BUTTON4, &PORTF, 7, 1);
	for(;;) {
		if(getButtonState(BUTTON1, false) == buttonState_Short) {
			//Beispiel-Anwendung der Eventgroup um einen Tastendruck zu übermitteln.
			xEventGroupSetBits(egAlarmClock, ALARMCLOCK_TESTBUTTONPRESS);
		}
		if(getButtonState(BUTTON2, false) == buttonState_Short) {
			
		}
		if(getButtonState(BUTTON3, false) == buttonState_Short) {
			
		}
		if(getButtonState(BUTTON4, false) == buttonState_Short) {
			
		}
		if(getButtonState(BUTTON1, true) == buttonState_Long) {
			
		}
		if(getButtonState(BUTTON2, true) == buttonState_Long) {
			
		}
		if(getButtonState(BUTTON3, true) == buttonState_Long) {
			
		}
		if(getButtonState(BUTTON4, true) == buttonState_Long) {
			
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
