/*
    FreeRTOS V9.0.0 - Copyright (C) 2016 Real Time Engineers Ltd.
    All rights reserved

    VISIT http://www.FreeRTOS.org TO ENSURE YOU ARE USING THE LATEST VERSION.

    This file is part of the FreeRTOS distribution.

    FreeRTOS is free software; you can redistribute it and/or modify it under
    the terms of the GNU General Public License (version 2) as published by the
    Free Software Foundation >>>> AND MODIFIED BY <<<< the FreeRTOS exception.

    ***************************************************************************
    >>!   NOTE: The modification to the GPL is included to allow you to     !<<
    >>!   distribute a combined work that includes FreeRTOS without being   !<<
    >>!   obliged to provide the source code for proprietary components     !<<
    >>!   outside of the FreeRTOS kernel.                                   !<<
    ***************************************************************************

    FreeRTOS is distributed in the hope that it will be useful, but WITHOUT ANY
    WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
    FOR A PARTICULAR PURPOSE.  Full license text is available on the following
    link: http://www.freertos.org/a00114.html

    ***************************************************************************
     *                                                                       *
     *    FreeRTOS provides completely free yet professionally developed,    *
     *    robust, strictly quality controlled, supported, and cross          *
     *    platform software that is more than just the market leader, it     *
     *    is the industry's de facto standard.                               *
     *                                                                       *
     *    Help yourself get started quickly while simultaneously helping     *
     *    to support the FreeRTOS project by purchasing a FreeRTOS           *
     *    tutorial book, reference manual, or both:                          *
     *    http://www.FreeRTOS.org/Documentation                              *
     *                                                                       *
    ***************************************************************************

    http://www.FreeRTOS.org/FAQHelp.html - Having a problem?  Start by reading
    the FAQ page "My application does not run, what could be wrong?".  Have you
    defined configASSERT()?

    http://www.FreeRTOS.org/support - In return for receiving this top quality
    embedded software for free we request you assist our global community by
    participating in the support forum.

    http://www.FreeRTOS.org/training - Investing in training allows your team to
    be as productive as possible as early as possible.  Now you can receive
    FreeRTOS training directly from Richard Barry, CEO of Real Time Engineers
    Ltd, and the world's leading authority on the world's leading RTOS.

    http://www.FreeRTOS.org/plus - A selection of FreeRTOS ecosystem products,
    including FreeRTOS+Trace - an indispensable productivity tool, a DOS
    compatible FAT file system, and our tiny thread aware UDP/IP stack.

    http://www.FreeRTOS.org/labs - Where new FreeRTOS products go to incubate.
    Come and try FreeRTOS+TCP, our new open source TCP/IP stack for FreeRTOS.

    http://www.OpenRTOS.com - Real Time Engineers ltd. license FreeRTOS to High
    Integrity Systems ltd. to sell under the OpenRTOS brand.  Low cost OpenRTOS
    licenses offer ticketed support, indemnification and commercial middleware.

    http://www.SafeRTOS.com - High Integrity Systems also provide a safety
    engineered and independently SIL3 certified version for use in safety and
    mission critical applications that require provable dependability.

    1 tab == 4 spaces!
*/

/******************************************************************************
 * This project provides two demo applications.  A simple blinky style project,
 * and a more comprehensive test and demo application.  The
 * mainCREATE_SIMPLE_BLINKY_DEMO_ONLY setting (defined in this file) is used to
 * select between the two.  The simply blinky demo is implemented and described
 * in main_blinky.c.  The more comprehensive test and demo application is
 * implemented and described in main_full.c.
 *
 * This file implements the code that is not demo specific, including the
 * hardware setup and FreeRTOS hook functions.
 *
 * ENSURE TO READ THE DOCUMENTATION PAGE FOR THIS PORT AND DEMO APPLICATION ON
 * THE http://www.FreeRTOS.org WEB SITE FOR FULL INFORMATION ON USING THIS DEMO
 * APPLICATION, AND ITS ASSOCIATE FreeRTOS ARCHITECTURE PORT!
 * http://www.FreeRTOS.org/RTOS_Intel_Quark_Galileo_GCC.html
 *
 */

/* Standard includes. */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <drivers/pci/pci.h>

/* Scheduler include files. */
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"


/* Added Galileo serial support. */
#include "galileo_support.h"
#include "GPIO_I2C.h"
#include "legacy_gpio.h"
#include "gpio.h"
#include "i2c.h"

/* Set to 1 to sit in a loop on start up, allowing a debugger to connect to the
application before main() executes. */
#define mainWAIT_FOR_DEBUG_CONNECTION 		1

/* Set mainCREATE_SIMPLE_BLINKY_DEMO_ONLY to one to run the simple blinky demo,
or 0 to run the more comprehensive test and demo application. */
#define mainCREATE_SIMPLE_BLINKY_DEMO_ONLY	1

/*-----------------------------------------------------------*/

/*
 * main_blinky() is used when mainCREATE_SIMPLE_BLINKY_DEMO_ONLY is set to 1.
 * main_full() is used when mainCREATE_SIMPLE_BLINKY_DEMO_ONLY is set to 0.
 */
#if( mainCREATE_SIMPLE_BLINKY_DEMO_ONLY == 1 )
	extern void main_blinky( void );
#else
	extern void main_full( void );
#endif /* mainCREATE_SIMPLE_BLINKY_DEMO_ONLY == 1 */

/* Prototypes for functions called from asm start up code. */
int main( void );
void CRT_Init( void );

/*
 * Prototypes for the standard FreeRTOS callback/hook functions implemented
 * within this file.
 */
void vApplicationMallocFailedHook( void );
void vApplicationIdleHook( void );
void vApplicationStackOverflowHook( TaskHandle_t pxTask, char *pcTaskName );
void vApplicationTickHook( void );

/*
 * Perform any hardware/peripheral related initialisation necessary to run the
 * demo.
 */
static void prvCalibrateLVTimer( void );

/*
 * If mainWAIT_FOR_DEBUG_CONNECTION is set to 1 then the following function will
 * sit in a loop on start up, allowing a debugger to connect to the application
 * before main() executes.  If mainWAIT_FOR_DEBUG_CONNECTION is not set to 1
 * then the following function does nothing.
 */
static void prvLoopToWaitForDebugConnection( void );

/*
 * Helper functions used when an assert is triggered.  The first periodically
 * displays an assert message, and the second clears the assert message when the
 * function called by the configASSERT() macro is exited.
 */
static void prvDisplayAssertion( const char * pcFile, unsigned long ulLine );
static void prvClearAssertionLine( void );

/*-----------------------------------------------------------*/

/* See http://www.FreeRTOS.org/RTOS_Intel_Quark_Galileo_GCC.html for usage
instructions. */

/* Priorities at which the tasks are created. */
#define mainQUEUE_RECEIVE_TASK_PRIORITY		( tskIDLE_PRIORITY + 2 )
#define	mainQUEUE_SEND_TASK_PRIORITY		( tskIDLE_PRIORITY + 1 )

/* The rate at which data is sent to the queue.  The 200ms value is converted
to ticks using the portTICK_PERIOD_MS constant. */
#define mainQUEUE_SEND_FREQUENCY_MS			( pdMS_TO_TICKS( 1000 ) )

/* The number of items the queue can hold.  This is 1 as the receive task
will remove items as they are added, meaning the send task should always find
the queue empty. */
#define mainQUEUE_LENGTH					( 1 )

/* The queue used by both tasks. */
static QueueHandle_t xQueue = NULL;
i2c_t i2c;
legacy_gpio_t legacy_gpio[6];
gpio_t gpio[10];

static uint32_t task_exec_counter[3] = {0, 0, 0};

static void prvQueueSendTask( void *pvParameters )
{
	TickType_t xNextWakeTime;

	/* Initialise xNextWakeTime - this only needs to be done once. */
	xNextWakeTime = xTaskGetTickCount();

	for( ;; )
	{
		/* Place this task in the blocked state until it is time to run again. */
		vTaskDelayUntil( &xNextWakeTime, mainQUEUE_SEND_FREQUENCY_MS );

		// read current output value
		uint32_t output_value = legacy_gpio_read(&legacy_gpio[5]);

		/* Send to the queue - causing the queue receive task to unblock and
		write to the COM port.  0 is used as the block time so the sending
		operation will not block - it shouldn't need to block as the queue
		should always be empty at this point in the code. */
		xQueueSend( xQueue, &output_value, 0U);

		task_exec_counter[0] += 1;
	}
}
/*-----------------------------------------------------------*/
static void prvQueueReceiveTask( void *pvParameters )
{
	for( ;; )
	{
		/* Wait until something arrives in the queue - this task will block
		indefinitely provided INCLUDE_vTaskSuspend is set to 1 in
		FreeRTOSConfig.h. */
		uint32_t output_value = 0;
		xQueueReceive( xQueue, &output_value, portMAX_DELAY );

		/* Print the LED status */
		g_printf_rcc( 6, 2, DEFAULT_SCREEN_COLOR, "LED State = %s\r\n", output_value ? "OFF" : "ON ");

		// write opposite value
		legacy_gpio_write(&legacy_gpio[5], !output_value);

		for (int i = 0; i < 6; i++)
		{
			legacy_gpio_write(&legacy_gpio[i], !output_value);
		}

		for (int i = 0; i < 10; i++)
		{
			gpio_write(&gpio[i], !output_value);
		}

		task_exec_counter[1] += 1;
	}
}
/*-----------------------------------------------------------*/

int main( void )
{
	/* Optionally wait for a debugger to connect. */
	prvLoopToWaitForDebugConnection();

	/* Initialise the serial port and GPIO. */
	vInitializeGalileoSerialPort( DEBUG_SERIAL_PORT );

	/* Print RTOS loaded message. */
	vPrintBanner();

	vGalileoRouteLEDPins();

	g_printf_rcc( 3, 2, DEFAULT_SCREEN_COLOR, "Running main_blinky()." );

	/* Create the queue. */
	xQueue = xQueueCreate( mainQUEUE_LENGTH, sizeof( uint32_t ) );

	// create load led instance
	for (int i = 0; i < 6; i++)
	{
		legacy_gpio_init(&legacy_gpio[i], i, 0, 0);
	}

	for (int i = 0; i < 10; i++)
	{
		gpio_init(&gpio[i], i, 1, 1);
	}

	vGalileoRouteLEDPins();

	if( xQueue != NULL )
	{
		/* Start the two tasks as described in the comments at the top of this
		file. */
		xTaskCreate( prvQueueReceiveTask,				/* The function that implements the task. */
		             "Rx", 								/* The text name assigned to the task - for debug only as it is not used by the kernel. */
		             configMINIMAL_STACK_SIZE * 2,		/* The size of the stack to allocate to the task. */
		             NULL, 								/* The parameter passed to the task - not used in this case. */
		             mainQUEUE_RECEIVE_TASK_PRIORITY, 	/* The priority assigned to the task. */
		             NULL );								/* The task handle is not required, so NULL is passed. */

		xTaskCreate( prvQueueSendTask, "TX", configMINIMAL_STACK_SIZE * 2, NULL, mainQUEUE_SEND_TASK_PRIORITY, NULL );

		/* Start the tasks and timer running. */
		vTaskStartScheduler();
	}

	/* If all is well, the scheduler will now be running, and the following
	line will never be reached.  If the following line does execute, then
	there was either insufficient FreeRTOS heap memory available for the idle
	and/or timer tasks to be created, or vTaskStartScheduler() was called from
	User mode.  See the memory management section on the FreeRTOS web site for
	more details on the FreeRTOS heap http://www.freertos.org/a00111.html.  The
	mode from which main() is called is set in the C start up code and must be
	a privileged mode (not user mode). */
	for( ;; );

	return 0;
}
/*-----------------------------------------------------------*/

void vApplicationMallocFailedHook( void )
{
	/* Called if a call to pvPortMalloc() fails because there is insufficient
	free memory available in the FreeRTOS heap.  pvPortMalloc() is called
	internally by FreeRTOS API functions that create tasks, queues, software
	timers, and semaphores.  The size of the FreeRTOS heap is set by the
	configTOTAL_HEAP_SIZE configuration constant in FreeRTOSConfig.h.

	Force an assert. */
	configASSERT( xTaskGetTickCount() == 0 );
	taskDISABLE_INTERRUPTS();
	for( ;; );
}
/*-----------------------------------------------------------*/

void vApplicationStackOverflowHook( TaskHandle_t pxTask, char *pcTaskName )
{
	( void ) pcTaskName;
	( void ) pxTask;

	/* Run time stack overflow checking is performed if
	configCHECK_FOR_STACK_OVERFLOW is defined to 1 or 2.  This hook
	function is called if a stack overflow is detected.

	Increase the size of the stack allocated to the offending task.

	Force an assert. */
	configASSERT( pxTask == NULL );
	taskDISABLE_INTERRUPTS();
	for( ;; );
}
/*-----------------------------------------------------------*/

void vApplicationIdleHook( void )
{
	task_exec_counter[2] += 1;
}
/*-----------------------------------------------------------*/

static void prvDisplayAssertion( const char * pcFile, unsigned long ulLine )
{
extern void vMilliSecondDelay( uint32_t DelayTime );
const uint32_t ul500ms = 500UL;

	/* Display assertion file and line. Don't use the gated g_printf just in
	the assert was triggered while the gating semaphore was taken.  Always print
	on line 23. */
	UngatedMoveToScreenPosition( 23, 2 );
	printf( ANSI_COLOR_RED );
	printf( "ASSERT: File = %s, Line = %lu\n\r", pcFile, ulLine );
	printf( ANSI_COLOR_RESET );
	printf( ANSI_SHOW_CURSOR );
	vMilliSecondDelay( ul500ms );
}
/*-----------------------------------------------------------*/

static void prvClearAssertionLine( void )
{
	UngatedMoveToScreenPosition( 23, 1 );
	printf( ANSI_COLOR_RESET );
	printf( ANSI_CLEAR_LINE );
	printf( ANSI_HIDE_CURSOR );
}
/*-----------------------------------------------------------*/

void vAssertCalled( const char * pcFile, unsigned long ulLine )
{
volatile uint32_t ul = 0;

	( void ) pcFile;
	( void ) ulLine;

	taskENTER_CRITICAL();
	{
		/* Set ul to a non-zero value or press a key to step out of this
		function in order to inspect the location of the assert(). */

		/* Clear any pending key presses. */
		while( ucGalileoGetchar() != 0 )
		{
			/* Nothing to do here - the key press is just discarded. */
		}

		do
		{
		   prvDisplayAssertion(pcFile, ulLine);
		} while ( ( ul == pdFALSE ) && ( ucGalileoGetchar() == 0 ) );

		prvClearAssertionLine();
	}
	taskEXIT_CRITICAL();
}
/*-----------------------------------------------------------*/

void vApplicationTickHook( void )
{

}
/*-----------------------------------------------------------*/

static void prvLoopToWaitForDebugConnection( void )
{
	/* Debug if define = 1. */
	#if( mainWAIT_FOR_DEBUG_CONNECTION == 1 )
	{
	/* When using the debugger, set this value to pdFALSE, and the application
	will sit in a loop at the top of main() to allow the debugger to attached
	before the application starts running.  Once attached, set
	ulExitResetSpinLoop to a non-zero value to leave the loop. */
	volatile uint32_t ulExitResetSpinLoop = pdFALSE;

		/* Must initialize UART before anything will print. */
		vInitializeGalileoSerialPort( DEBUG_SERIAL_PORT );

		/* RTOS loaded message. */
		vPrintBanner();

		/* Output instruction message. */
		MoveToScreenPosition( 3, 1 );
		g_printf( DEFAULT_SCREEN_COLOR );
		g_printf( " Waiting for JTAG connection.\n\n\r" );
		g_printf( ANSI_COLOR_RESET );
		g_printf( " Once connected, either set ulExitResetSpinLoop to a non-zero value,\n\r" );
		g_printf( " or you can [PRESS ANY KEY] to start the debug session.\n\n\r" );
		printf( ANSI_SHOW_CURSOR );

		/* Use the debugger to set the ulExitResetSpinLoop to a non-zero value
		or press a key to exit this loop, and step through the application.  In
		Eclipse, simple hover over the variable to see its value in a pop-over
		box, then edit the value in the pop-over box. */
		do
		{
			portNOP();

		} while( ( ulExitResetSpinLoop == pdFALSE ) && ( ucGalileoGetchar() == 0 ) );
	}
	#endif
}
/*-----------------------------------------------------------*/

void CRT_Init( void )
{
extern uint32_t *__bss_start;
extern uint32_t *__bss_end;
extern uint32_t *__data_vma;
extern uint32_t *__data_lma;
extern uint32_t *__data_start;
extern uint32_t *__data_end;
uint32_t x = 255;
size_t xSize;

	/* Zero out bss. */
	xSize = ( ( size_t ) __bss_end ) - ( ( size_t ) __bss_start );
	memset( ( void * ) __bss_start, 0x00, xSize );

	/* Copy initialised variables. */
	xSize = ( ( size_t ) __data_end ) - ( ( size_t ) __data_start );
	memcpy( ( void * ) __data_vma, __data_lma, xSize );

	/* Ensure no interrupts are pending. */
	do
	{
		portAPIC_EOI = 0;
		x--;
	} while( x > 0 );
}
/*-----------------------------------------------------------*/
