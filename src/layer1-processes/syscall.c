#include "syscall.h"
#include "processes.h"
#include "asm.h"
#include "layer0.h"
#include "rpi.h"
#include "util.h"
#include "timer/systimer.h"
#include "gic.h"
#include "vmm.h"
#include "apu_completion.h"
#include "malloc/malloc.h"
#include "kernel_state.h"
#include "sched_stubs.h"
#include "ipc_stubs.h"
#include "messaging.h"

#define READY 0

extern void EXIT(void);
extern void setActiveInterrupt(uint32_t interruptid);
extern void INTERRUPT_CLEAR_ACTIVE_REGS(uint32_t interruptid);
extern uint32_t readInterruptId(void);
extern unsigned char uart_getc_modified(size_t line);
extern uint32_t get_CTS(size_t line);
extern void clear_GICC_EOIR(uint16_t interrupt_id);
extern uint32_t checkActiveInterrupt(uint32_t interrupt_id);
extern void handlerExceptionHelper(uint64_t esr_el1);

static struct process_container PROCESS_CONTAINERS[NPROCESSES];
static uint8_t PROCESS_SHARED_MEM[NPROCESSES][SHARED_MEM_PER_PROCESS];

uint8_t NO_PARAMS = 0;

static void apu_init_process_containers(void)
{
	for (int i = 0; i < NPROCESSES; i++)
	{
		PROCESS_CONTAINERS[i].pid = i;
		PROCESS_CONTAINERS[i].shared_mem_base = &PROCESS_SHARED_MEM[i][0];
		PROCESS_CONTAINERS[i].shared_mem_size = SHARED_MEM_PER_PROCESS;
	}
}

int KernelCreateInProcess(uint8_t priority, void (*function)(), int parent, int process_id)
{
	if (process_id < 0 || process_id >= NPROCESSES)
		return -3;
	for (int i = 0; i < NUMPROCS; i++)
	{
		if (PROCS[i].pcpointer != NULL)
			continue;
		PROCS[i].pcpointer = function;
		PROCS[i].stackpointer = ((uint8_t *)STACKSTART) + (STACK_SIZE_PER_THREAD * (i + 1));
		PROCS[i].parentpid = parent;
		PROCS[i].process_id = process_id;
		PROCS[i].priority = priority;
		PROCS[i].pid = i + 1;
		PROCS[i].pstate = 0;
		PROCS[i].waiting_reply = 0;
		PROCS[i].waiting_send = 0;
		PROCS[i].waiting_recieve_head = 0;
		PROCS[i].waiting_recieve_tail = 0;
		PROCS[i].queuesize = 0;
		PROCS[i].totaltime = 0;
		scrSchedule(i + 1, PROCS[i].priority);
		return i + 1;
	}
	return -1;
}

void InitSys(void *reg)
{
	sched_init(reg);
	vm_init();
	apu_init_process_containers();
	malloc_init_default();
}

void HandleASYNC(void *sp)
{
	int p = PID - 1;
	updateRunTimer();
	uint64_t ASYNC = Save(sp, &PROCS[p].registervalues[0], &PROCS[p].pcpointer, &PROCS[p].stackpointer, &PROCS[p].pstate);
	ExceptionASYNC(ASYNC);
	Schedule();
#if DEBUG_EXIT >= 1
	uart_printf(CONSOLE, "All Tasks Complete, Press Any Key to Exit\n\r");
	uart_getc(1);
	EXIT();
#endif
}

int unblock_return(uint32_t interruptid, uint64_t ret)
{
	for (int i = 0; i < AWAIT_INTERRUPT[interruptid].len; i++)
	{
		struct state freed_state = AWAIT_INTERRUPT[interruptid].pid_ls[i];
		int p_free = freed_state.pid - 1;
		freed_state.time = READY;
		unblock(freed_state);
		PROCS[p_free].registervalues[0] = ret;
	}
	ret = AWAIT_INTERRUPT[interruptid].len;
	AWAIT_INTERRUPT[interruptid].len = 0;
	return (int)ret;
}

void ExceptionASYNC(uint64_t esr_el1)
{
	(void)esr_el1;
	int p = PID - 1;
	uint32_t interruptid = readInterruptId();
	setActiveInterrupt(interruptid);
	scrSchedule((int)PID, PROCS[p].priority);

	switch (interruptid)
	{
	case CLOCKINTID:
		gentimer_arm_ms(10);
		unblock_return(CLOCKINTID, 1);
		break;
	case UARTINTER:
	{
		char return_val[8];
		volatile uint32_t *RIS_CONSOLE = get_RIS(CONSOLE);
		volatile uint32_t *ICR_CONSOLE = get_ICR(CONSOLE);
		volatile uint32_t *RIS_MARKLIN = get_RIS(MARKLIN);
		volatile uint32_t *ICR_MARKLIN = get_ICR(MARKLIN);
		if ((*RIS_CONSOLE) & (0x01 << CTSMIM))
		{
			return_val[0] = CTSMIM;
			return_val[1] = MARKLIN;
			return_val[2] = (get_CTS(MARKLIN) == 1) ? 1 : 0;
			*ICR_CONSOLE = (0x01 << CTSMIM);
		}
		else if ((*RIS_CONSOLE) & ((0x01 << RXIC) | (0x01 << 6)))
		{
			return_val[0] = RXIC;
			return_val[1] = CONSOLE;
			return_val[2] = uart_getc_modified(CONSOLE);
			*ICR_CONSOLE = ((0x01 << RXIC) | (0x01 << 6));
			while (uart_getc_queue(CONSOLE))
			{
				char extra[8] = {0};
				extra[0] = RXIC;
				extra[1] = CONSOLE;
				extra[2] = uart_getc_modified(CONSOLE);
				AWAIT_INTERRUPT[interruptid].event_q[AWAIT_INTERRUPT[interruptid].eventq_tail] = *(uint64_t *)extra;
				AWAIT_INTERRUPT[interruptid].eventq_tail++;
				AWAIT_INTERRUPT[interruptid].eventq_tail %= NUMPROCS;
				AWAIT_INTERRUPT[interruptid].eventq_len++;
			}
		}
		else if ((*RIS_MARKLIN) & (0x01 << TXIC))
		{
			return_val[0] = TXIC;
			return_val[1] = MARKLIN;
			return_val[2] = -1;
			*ICR_MARKLIN = (0x01 << TXIC);
		}
		else if ((*RIS_MARKLIN) & (0x01 << RXIC))
		{
			return_val[0] = RXIC;
			return_val[1] = MARKLIN;
			return_val[2] = uart_getc_modified(MARKLIN);
			*ICR_MARKLIN = (0x01 << RXIC);
		}
		else if ((*RIS_MARKLIN) & (0x01 << CTSMIM))
		{
			return_val[0] = CTSMIM;
			return_val[1] = MARKLIN;
			return_val[2] = get_CTS(MARKLIN);
			*ICR_MARKLIN = (0x01 << CTSMIM);
		}
		if (!unblock_return(interruptid, *(uint64_t *)return_val))
		{
			AWAIT_INTERRUPT[interruptid].event_q[AWAIT_INTERRUPT[interruptid].eventq_tail] = *(uint64_t *)return_val;
			AWAIT_INTERRUPT[interruptid].eventq_tail++;
			AWAIT_INTERRUPT[interruptid].eventq_tail %= NUMPROCS;
			AWAIT_INTERRUPT[interruptid].eventq_len++;
		}
		break;
	}
	case APU1_DONE_EVENT:
	case APU2_DONE_EVENT:
	case APU3_DONE_EVENT:
		if (!unblock_return(interruptid, (uint64_t)interruptid)) {
			AWAIT_INTERRUPT[interruptid].event_q[AWAIT_INTERRUPT[interruptid].eventq_tail] = (uint64_t)interruptid;
			AWAIT_INTERRUPT[interruptid].eventq_tail++;
			AWAIT_INTERRUPT[interruptid].eventq_tail %= NUMPROCS;
			AWAIT_INTERRUPT[interruptid].eventq_len++;
		}
		break;
	default:
		break;
	}
	INTERRUPT_CLEAR_ACTIVE_REGS(interruptid);
	clear_GICC_EOIR(interruptid);
}

void Handle(void *sp)
{
	int p = PID - 1;
	updateRunTimer();
	uint64_t esr_el1 = Save(sp, &PROCS[p].registervalues[0], &PROCS[p].pcpointer, &PROCS[p].stackpointer, &PROCS[p].pstate);
	handlerExceptionHelper(esr_el1);
	Schedule();
#if DEBUG_EXIT >= 1
	uart_printf(CONSOLE, "All Tasks Complete, Press Any Key to Exit\n\r");
	uart_getc(1);
	EXIT();
#endif
}

void handlerExceptionHelper(uint64_t esr_el1)
{
	int p = PID - 1;
	if ((esr_el1 >> 24) != 86)
	{
		scrSchedule((int)PID, PROCS[p].priority);
		return;
	}
	if (esr_el1 >> 24 == 86)
	{
		int i = (int)(esr_el1 % 0x1000000);
		switch (i)
		{
		case 0:
			Kill(p);
			break;
		case 1:
			scrSchedule((int)PID, PROCS[p].priority);
			break;
		case 2:
			scrSchedule((int)PID, PROCS[p].priority);
			PROCS[p].registervalues[0] = (uint64_t)KernelCreate((uint8_t)PROCS[p].registervalues[0], (void (*)(void))PROCS[p].registervalues[1], (int)PID);
			if ((int)PROCS[p].registervalues[0] > 0)
			{
				int child = (int)PROCS[p].registervalues[0] - 1;
				PROCS[child].process_id = (PID > 0) ? PROCS[p].process_id : 0;
			}
			break;
		case 3:
			scrSchedule((int)PID, PROCS[p].priority);
			PROCS[p].registervalues[0] = (uint64_t)PROCS[p].pid;
			break;
		case 4:
			scrSchedule((int)PID, PROCS[p].priority);
			PROCS[p].registervalues[0] = (uint64_t)PROCS[p].parentpid;
			break;
		case 5:
			ipc_handle_send(p);
			break;
		case 6:
			ipc_handle_receive(p);
			break;
		case 7:
			ipc_handle_reply(p);
			break;
		case 8:
			scrSchedule((int)PID, PROCS[p].priority);
			PROCS[p].registervalues[0] = PROCS[p].priority;
			break;
		case 9:
		{
			scrSchedule((int)PID, PROCS[p].priority);
			uint64_t retd = (uint64_t)KernelCreate((uint8_t)PROCS[p].registervalues[0], (void (*)(void))PROCS[p].registervalues[1], (int)PID);
			if (PROCS[p].registervalues[2] > 0)
			{
				for (int j = 0; j < 8; j++)
					PROCS[retd - 1].registervalues[j] = ((int64_t *)PROCS[p].registervalues[3])[j];
				if (PROCS[p].registervalues[2] > 8)
				{
					int64_t *newsp = (int64_t *)PROCS[retd - 1].stackpointer;
					uint8_t stack_offset = (uint8_t)(PROCS[p].registervalues[2] - 8);
					newsp = newsp - (PROCS[p].registervalues[2] - 8);
					if (stack_offset > 0)
					{
						for (int j = 0; j < stack_offset; j++)
							newsp[j] = ((int64_t *)PROCS[p].registervalues[3])[j + 8];
						PROCS[retd - 1].stackpointer = (void *)newsp;
					}
				}
			}
			if ((int)retd > 0)
				PROCS[retd - 1].process_id = (PID > 0) ? PROCS[p].process_id : 0;
			PROCS[p].registervalues[0] = retd;
			break;
		}
		case 10:
		{
			uint64_t eventType = PROCS[p].registervalues[0];
			PROCS[p].registervalues[0] = (uint64_t)-1;
			if (checkActiveInterrupt(eventType))
			{
				if (AWAIT_INTERRUPT[eventType].eventq_len)
				{
					scrSchedule((int)PID, PROCS[p].priority);
					PROCS[p].registervalues[0] = AWAIT_INTERRUPT[eventType].event_q[AWAIT_INTERRUPT[eventType].eventq_head];
					AWAIT_INTERRUPT[eventType].eventq_head++;
					AWAIT_INTERRUPT[eventType].eventq_head %= NUMPROCS;
					AWAIT_INTERRUPT[eventType].eventq_len--;
				}
				else
				{
					block((int)PID, PROCS[p].priority);
					struct state currItem = {(int)PID, PROCS[p].priority, SCHED_BLOCKED};
					AWAIT_INTERRUPT[eventType].pid_ls[AWAIT_INTERRUPT[eventType].len] = currItem;
					AWAIT_INTERRUPT[eventType].len++;
				}
			}
			else
				scrSchedule((int)PID, PROCS[p].priority);
			break;
		}
		case 11:
			scrSchedule((int)PID, PROCS[p].priority);
			PROCS[p].registervalues[0] = PROCS[p].totaltime;
			break;
		case 12:
			scrSchedule((int)PID, PROCS[p].priority);
			PROCS[p].registervalues[0] = get_timerLO() - kernelStartTime;
			break;
		case 13:
			scrSchedule((int)PID, PROCS[p].priority);
			PROCS[p].registervalues[0] = (uint64_t)PROCS[p].process_id;
			break;
		case 14:
			scrSchedule((int)PID, PROCS[p].priority);
		{
			int pcid = PROCS[p].process_id;
			if (pcid >= 0 && pcid < NPROCESSES)
				PROCS[p].registervalues[0] = (uint64_t)(uintptr_t)PROCESS_CONTAINERS[pcid].shared_mem_base;
			else
				PROCS[p].registervalues[0] = 0;
			break;
		}
		case 20:
			scrSchedule((int)PID, PROCS[p].priority);
			PROCS[p].registervalues[0] = (uint64_t)KernelCreateInProcess(
			    (uint8_t)PROCS[p].registervalues[0],
			    (void (*)(void))PROCS[p].registervalues[1],
			    (int)PID,
			    (int)PROCS[p].registervalues[2]);
			break;
		default:
			scrSchedule((int)PID, PROCS[p].priority);
			break;
		}
	}
}

void Deregister(void) {}

void Exit(void)
{
	Deregister();
	asm_svc_0();
}

void Yield(void)
{
	asm_svc_1();
}

int MyTid(void) { return asm_svc_3(); }
int MyParentTid(void) { return asm_svc_4(); }
int MyPriority(void) { return asm_svc_8(); }
int Create(uint8_t priority, void (*function)(void)) { return asm_svc_2(priority, (void (*)(void))function); }
int CreateArgs(uint8_t priority, void (*function)(), uint64_t argsno, uint64_t *args)
{
	(void)priority; (void)function; (void)argsno; (void)args;
	return asm_svc_9();
}
int AwaitEvent(int eventType) { return asm_svc_10(eventType); }
int GetRuntime(void) { return asm_svc_11(); }
int GetKernelRuntime(void) { return asm_svc_12(); }
int MyProcessId(void) { return asm_svc_13(); }
void *GetProcessSharedMem(void) { return asm_svc_14(); }
int CreateInProcess(uint8_t priority, void (*function)(void), int process_id)
{
	return asm_svc_20(priority, function, process_id);
}
