/*
 * main.h
 *
 *  Created on: Dec 24, 2020
 *      Author: oankarpatil
 */

#ifndef MAIN_H_
#define MAIN_H_

#define MAX_TASKS         5
#define DUMMY_XPSR        0x01000000

#define SRAM_START        0x20000000
#define SRAM_SIZE         ((128) * (1024))
#define SRAM_END          ((SRAM_START) + (SRAM_SIZE))

#define TASK_SIZE_STACK   (1024U)
#define TASK1_START       SRAM_END
#define TASK2_START       ((SRAM_END) - (1 * (TASK_SIZE_STACK)))
#define TASK3_START       ((SRAM_END) - (2 * (TASK_SIZE_STACK)))
#define TASK4_START       ((SRAM_END) - (3 * (TASK_SIZE_STACK)))
#define IDLE_START        ((SRAM_END) - (4 * (TASK_SIZE_STACK)))
#define SCHEDULER_START   ((SRAM_END) - (5 * (TASK_SIZE_STACK)))

#define TICK_HZ           1000U
#define SYSTICK_HZ        16000000U

#define READY_STATE       0x00
#define BLOCKED_STATE     0xFF

#define INTERRUPT_D()     do{__asm__ volatile ("MOV R0, #0x2"); __asm__ volatile ("MSR PRIMASK, R0"); } while(0)
#define INTERRUPT_E()     do{__asm__ volatile ("MOV R0, #0x0"); __asm__ volatile ("MSR PRIMASK, R0"); } while(0)

#endif /* MAIN_H_ */
