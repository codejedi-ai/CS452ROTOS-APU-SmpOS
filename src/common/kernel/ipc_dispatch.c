#include "ipc_stubs.h"
#include "kernel_api.h"
#include "kernel_state.h"
#include "sched_stubs.h"
#include "syscall.h"

void ipc_handle_send(int p)
{
	int dest_tid = (int)PROCS[p].registervalues[0];
	int dest_p = dest_tid - 1;
	if (dead((int8_t)dest_p))
	{
		sched_enqueue((int)PID, PROCS[p].priority);
		PROCS[p].registervalues[0] = (uint64_t)-1;
	}
	else if (PROCS[dest_p].queuesize >= QUEUESIZE)
	{
		sched_enqueue((int)PID, PROCS[p].priority);
		PROCS[p].registervalues[0] = (uint64_t)-2;
	}
	else
	{
		sched_block((int)PID, PROCS[p].priority);
		ipc_kernel_send();
	}
}

void ipc_handle_receive(int p)
{
	(void)p;
	sched_block((int)PID, PROCS[p].priority);
	ipc_kernel_receive((int)PID);
}

void ipc_handle_reply(int p)
{
	sched_enqueue((int)PID, PROCS[p].priority);
	int dest_tid = (int)PROCS[p].registervalues[0];
	if (dead((int8_t)(dest_tid - 1)))
		PROCS[p].registervalues[0] = (uint64_t)-1;
	else if (PROCS[dest_tid - 1].waiting_reply != 1)
		PROCS[p].registervalues[0] = (uint64_t)-2 - PROCS[dest_tid - 1].waiting_reply;
	else
		ipc_kernel_reply();
}
