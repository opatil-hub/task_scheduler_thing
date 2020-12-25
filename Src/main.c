/**
 ******************************************************************************
 * @file           : main.c
 * @author         : Oankar Patil
 * @brief          : Main program body
 ******************************************************************************
 * @attention
 *
 * <h2><center>&copy; Copyright (c) 2020 STMicroelectronics.
 * All rights reserved.</center></h2>
 *
 * This software component is licensed by ST under BSD 3-Clause license,
 * the "License"; You may not use this file except in compliance with the
 * License. You may obtain a copy of the License at:
 *                        opensource.org/licenses/BSD-3-Clause
 *
 ******************************************************************************
 */

//shut up b, it's my code^^ /s jk lul

#include <stdint.h>
#include <stdio.h>
#include "main.h"

#if !defined(__SOFT_FP__) && defined(__ARM_FP)
  #warning "FPU is not initialized, but the project is compiling for an FPU. Please initialize the FPU before use."
#endif

void task1_handler(void);
void task2_handler(void);
void task3_handler(void);
void task4_handler(void);
void init_sys_tick_timer(uint32_t tick_hz);
void SysTick_Handler(void);
__attribute__((naked)) void init_stack_scheduler(uint32_t sched_start);
void init_tasks(void);
void enable_faults(void);
__attribute__ ((naked)) void switch_sp_psp(void);
uint32_t get_psp(void);
void save_psp_value(uint32_t current_psp_value);
void update_next_task(void);
void t_delay(uint32_t tick_count);
void idle_task(void);
void schedulePendSV(void);

uint8_t current_task = 0;
uint32_t global_tick_count = 1;

typedef struct {
	uint32_t psp_val;
	uint32_t block_count;
	uint8_t current_state;
	void (*task_handler)(void);
}task_control_block_t;

task_control_block_t user_tasks[MAX_TASKS];

int main(void)
{
	enable_faults();

	init_stack_scheduler(SCHEDULER_START);

	init_tasks();

	init_sys_tick_timer(TICK_HZ);

	switch_sp_psp();

	task1_handler();
    /* Loop forever */
	for(;;);
}

void init_tasks(void) {

	user_tasks[0].current_state = READY_STATE;
	user_tasks[1].current_state = READY_STATE;
	user_tasks[2].current_state = READY_STATE;
	user_tasks[3].current_state = READY_STATE;
	user_tasks[4].current_state = READY_STATE;

	user_tasks[0].psp_val = IDLE_START;
	user_tasks[1].psp_val = TASK1_START;
	user_tasks[2].psp_val = TASK2_START;
	user_tasks[3].psp_val = TASK3_START;
	user_tasks[4].psp_val = TASK4_START;

	user_tasks[0].task_handler = idle_task;
	user_tasks[1].task_handler = task1_handler;
	user_tasks[2].task_handler = task2_handler;
	user_tasks[3].task_handler = task3_handler;
	user_tasks[4].task_handler = task4_handler;

	uint32_t *p_PSP;
	for (int i = 0; i < MAX_TASKS; i++) {
		p_PSP = (uint32_t*) user_tasks[i].psp_val;
		p_PSP--;

		//xPSR
		*p_PSP = DUMMY_XPSR;

		//PC
		p_PSP--;
		*p_PSP = (uint32_t) user_tasks[i].task_handler;

		//LR
		p_PSP--;
		*p_PSP = 0xFFFFFFFD;

		for (int j = 0; j < 13; j++) {
			p_PSP--;
			*p_PSP = 0;
		}

		user_tasks[i].psp_val = (uint32_t) p_PSP;
	}
}

__attribute__((naked)) void init_stack_scheduler(uint32_t sched_start) {
	__asm__ volatile ("MSR MSP, R0");
	__asm__ volatile ("BX LR");
}

void init_sys_tick_timer(uint32_t tick_hz) {
	uint32_t *p_Syst_CSR = (uint32_t*) 0xE000E010;
	uint32_t *p_Syst_RVR = (uint32_t*) 0xE000E014;
	uint32_t count_value = ((SYSTICK_HZ / tick_hz) - 1);

	//clear any previous reload value
	*p_Syst_RVR &= ~(0x00FFFFFF);

	/*set reload value (documentation says n-1 amount of desired
	clock cycles)*/
	*p_Syst_RVR |= count_value;

	/*enable SYST_CSR, TICKINT Counting down and using
	processor clock*/
	*p_Syst_CSR |= (1 << 1); //counting down
	*p_Syst_CSR |= (1 << 2); //using processor clock
	*p_Syst_CSR |= (1 << 0); //enable

}
uint32_t get_psp(void) {
	return user_tasks[current_task].psp_val;
}

void save_psp_value(uint32_t current_psp_value) {
	user_tasks[current_task].psp_val = current_psp_value;
}

void update_next_task(void)
{
	int32_t state = BLOCKED_STATE;
	for (int i = 0; i < (MAX_TASKS); i++) {
		current_task++;
		current_task %= MAX_TASKS;
		state = user_tasks[current_task].current_state;
		if((state == READY_STATE) && (current_task != 0)) {
			break;
		}
	}
	if (state != READY_STATE) {
		current_task = 0;
	}
}

__attribute__ ((naked)) void switch_sp_psp(void) {
	__asm__ volatile("PUSH {LR}");
	__asm__ volatile("BL get_psp");
	__asm__ volatile("MSR PSP, R0");
	__asm__ volatile("POP {LR}");

	__asm__ volatile("MOV R0, 0x02");
	__asm__ volatile("MSR CONTROL,R0");
	__asm__ volatile("BX LR");
}

void t_delay(uint32_t tick_count) {

	INTERRUPT_D();

	if (current_task) {
		user_tasks[current_task].block_count = global_tick_count + tick_count;
		user_tasks[current_task].current_state = BLOCKED_STATE;
		schedulePendSV();
	}

	INTERRUPT_E();

}

void schedulePendSV(void) {
	uint32_t *p_ICSR = (uint32_t*) 0xE000ED04;
	*p_ICSR |= (1 << 28); //pending pendSV
}
//  >w< ahhh i'm naked asm-chan

__attribute__ ((naked)) void PendSV_Handler(void) {
	__asm__ volatile ("MRS R0, PSP");
	__asm__ volatile ("STMDB R0!, {R4-R11}");
	__asm__ volatile("PUSH {LR}");
	__asm__ volatile ("BL save_psp_value");

	__asm__ volatile ("BL update_next_task");
	__asm__ volatile ("BL get_psp");
	__asm__ volatile ("LDMIA R0!, {R4-R11}");
	__asm__ volatile ("MSR PSP, R0");
	__asm__ volatile("POP {LR}");
	__asm__ volatile("BX LR");
}

void update_global_tick_count(void) {
	global_tick_count++;
}

void unblock_tasks(void) {
	for (int i = 1; i < MAX_TASKS; i++) {
		if (user_tasks[i].current_state != READY_STATE) {
			if (user_tasks[i].block_count == global_tick_count) {
				user_tasks[i].current_state = READY_STATE;
			}
		}
	}
}

void SysTick_Handler(void) {
	uint32_t *p_ICSR = (uint32_t*) 0xE000ED04;

	update_global_tick_count();

	unblock_tasks();

	*p_ICSR |= (1 << 28); //pending pendSV
}

void idle_task(void) {
	while(1);
}

void task1_handler(void) {
	while(1) {
		t_delay(1000);
		printf("Task 1\n");
		t_delay(1000);
	}
}

void task2_handler(void) {
	while(1) {
		t_delay(500);
		printf("Task 2\n");
		t_delay(500);
	}
}

void task3_handler(void) {
	while(1) {
		t_delay(250);
		printf("Task 3\n");
		t_delay(250);
	}
}

void task4_handler(void) {
	while(1) {
		t_delay(125);
		printf("Task 4\n");
		t_delay(125);
	}
}

/*Enabling faults and implementing fault handlers*/
void enable_faults(void) {
	uint32_t *p_SHCSR = (uint32_t*) 0xE000ED24;

	*p_SHCSR |= (1 << 16);
	*p_SHCSR |= (1 << 17);
	*p_SHCSR |= (1 << 18);
}

void HardFault_Handler(void) {
	printf("Hard Fault\n");
	while(1);
}

void MemManage_Handler(void) {
	printf("Memory Manage Fault\n");
	while(1);
}

void BusFault_Handler(void) {
	printf("Bus Fault\n");
	while(1);
}
