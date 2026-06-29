#include <stddef.h>
#include <stdint.h>
#include "sched_stubs.h"
#include "kernel_state.h"
#include "asm.h"

extern void kernel_init_extras(void *reg);

extern uint32_t get_timerHI(void);
extern uint32_t get_timerLO(void);

static struct MinHeapState READY_HEAP;

static uint8_t compare_state(struct state a, struct state b)
{
	if (a.priority < b.priority)
		return 1;
	else if (a.priority > b.priority)
		return 2;
	else
	{
		if (a.time == (uint64_t)-1)
			return 2;
		else if (b.time == (uint64_t)-1)
			return 1;
		else if (a.time < b.time)
			return 1;
		else if (a.time > b.time)
			return 2;
		else
			return 0;
	}
}

static void swap_state(struct state *a, struct state *b)
{
	struct state temp;
	temp.pid = a->pid;
	temp.priority = a->priority;
	temp.time = a->time;
	a->pid = b->pid;
	a->priority = b->priority;
	a->time = b->time;
	b->pid = temp.pid;
	b->priority = temp.priority;
	b->time = temp.time;
}

static void _bubbleUp_state_heap(struct MinHeapState *h, int i)
{
	int parent = (i - 1) / 2;
	if (1 == compare_state(h->harr[i], h->harr[parent]))
	{
		swap_state(&h->harr[i], &h->harr[parent]);
		_bubbleUp_state_heap(h, parent);
	}
}

static void bubbleDown_state_heap(struct MinHeapState *h, int i)
{
	unsigned int left = 2 * i + 1;
	unsigned int right = 2 * i + 2;
	int smallest = i;
	if ((unsigned int)left >= h->size && (unsigned int)right >= h->size)
		return;
	if (left < h->size && (unsigned int)right >= h->size && compare_state(h->harr[left], h->harr[smallest]) == 1)
		smallest = left;
	if (left < h->size && compare_state(h->harr[left], h->harr[smallest]) == 1)
		smallest = left;
	if (right < h->size && compare_state(h->harr[right], h->harr[smallest]) == 1)
		smallest = right;
	if (smallest != i)
	{
		swap_state(&h->harr[i], &h->harr[smallest]);
		bubbleDown_state_heap(h, smallest);
	}
}

static void insertKey_state_heap(struct MinHeapState *h, struct state k)
{
	k.time = get_timerHI();
	k.time = k.time << 32;
	k.time += get_timerLO();
	if (h->size == h->capacity)
		return;
	h->harr[h->size] = k;
	_bubbleUp_state_heap(h, h->size);
	h->size++;
}

static uint8_t isEmpty_state_heap(struct MinHeapState *h)
{
	return h->size == 0;
}

static struct state extractMin_state_heap(struct MinHeapState *h)
{
	if (h->size <= 0)
		return (struct state){-1, 0, 0};
	if (h->size == 1)
	{
		h->size--;
		return h->harr[0];
	}
	struct state root = h->harr[0];
	h->harr[0] = h->harr[h->size - 1];
	h->size--;
	bubbleDown_state_heap(h, 0);
	return root;
}

void sched_init(void *reg)
{
	kernelStartTime = get_timerLO();
	READY_HEAP.size = 0;
	READY_HEAP.capacity = NUMPROCS;
	READY_HEAP.harr = READY_QUEUE;
	STACKSTART = reg;
	PID = 0;
	for (int event = 0; event < MAXEVENT; event++)
	{
		AWAIT_INTERRUPT[event].len = 0;
		AWAIT_INTERRUPT[event].eventq_len = 0;
		AWAIT_INTERRUPT[event].eventq_head = 0;
		AWAIT_INTERRUPT[event].eventq_tail = 0;
		for (int jdx = 0; jdx < NUMPROCS; jdx++)
		{
			AWAIT_INTERRUPT[event].pid_ls[jdx].pid = 0;
			AWAIT_INTERRUPT[event].pid_ls[jdx].priority = 0;
			AWAIT_INTERRUPT[event].pid_ls[jdx].time = 0;
		}
	}
	for (int idx = 0; idx < NUMPROCS; idx++)
	{
		PROCS[idx].stackpointer = NULL;
		PROCS[idx].pcpointer = NULL;
		PROCS[idx].pstate = 0;
		PROCS[idx].parentpid = 0;
		PROCS[idx].priority = 0;
		for (int jdx = 0; jdx < 31; jdx++)
			PROCS[idx].registervalues[jdx] = 10 + jdx;
		BLOCKED_LIST[idx].pid = 0;
		BLOCKED_LIST[idx].priority = 0;
		BLOCKED_LIST[idx].time = 0;
		READY_QUEUE[idx].pid = 0;
		READY_QUEUE[idx].time = -1;
		READY_QUEUE[idx].priority = -1;
	}
	kernel_init_extras(reg);
}

void scrSchedule(int pid, uint8_t priority)
{
	struct state currItem = {pid, priority, ((uint64_t)get_timerHI() << 32) + get_timerLO()};
	insertKey_state_heap(&READY_HEAP, currItem);
}

int unblock_ind(int pid, uint8_t priority)
{
	if (BLOCKED_LIST[pid - 1].pid == 0 && BLOCKED_LIST[pid - 1].priority == priority)
		return 0;
	scrSchedule(pid, priority);
	BLOCKED_LIST[pid - 1].pid = 0;
	return 0;
}

void block(int pid, uint8_t priority)
{
	if (pid < 1 || pid > NUMPROCS)
		return;
	BLOCKED_LIST[pid - 1].pid = pid;
	BLOCKED_LIST[pid - 1].priority = priority;
	BLOCKED_LIST[pid - 1].time = SCHED_BLOCKED;
}

void unblock(struct state currItem)
{
	unblock_ind(currItem.pid, currItem.priority);
}

int scrPick(void)
{
	if (isEmpty_state_heap(&READY_HEAP))
		return -1;
	struct state currItem = extractMin_state_heap(&READY_HEAP);
	return currItem.pid;
}

void updateRunTimer(void)
{
	int p = PID - 1;
	uint32_t curtime = get_timerLO();
	PROCS[p].totaltime += curtime - PROCS[p].waketime;
}

int8_t dead(int8_t p)
{
	return (PROCS[p].stackpointer == NULL && PROCS[p].pcpointer == NULL);
}

static int heap_contains_pid(int pid)
{
	for (unsigned i = 0; i < READY_HEAP.size; i++)
	{
		if (READY_HEAP.harr[i].pid == pid)
			return 1;
	}
	return 0;
}

static void rescue_orphaned_tasks(void)
{
	for (int i = 0; i < NUMPROCS; i++)
	{
		if (PROCS[i].pcpointer == NULL)
			continue;
		if (BLOCKED_LIST[i].pid != 0)
			continue;
		int pid = i + 1;
		if (!heap_contains_pid(pid))
			scrSchedule(pid, PROCS[i].priority);
	}
}

void Schedule(void)
{
	for (;;)
	{
		PID = (uint32_t)scrPick();
		if ((int)PID == -1)
		{
			rescue_orphaned_tasks();
			PID = (uint32_t)scrPick();
		}
		if ((int)PID != -1)
			break;
		wfi();
	}
	int p = (int)PID - 1;
	PROCS[p].waketime = get_timerLO();
	Begin(&PROCS[p].registervalues[0], PROCS[p].pcpointer, PROCS[p].stackpointer, PROCS[p].pstate);
}

int KernelCreate(uint8_t priority, void (*function)(), int parent)
{
	for (int p = 0; p < NUMPROCS; p++)
	{
		if (PROCS[p].pcpointer != NULL)
			continue;
		PROCS[p].pcpointer = function;
		PROCS[p].stackpointer = ((uint8_t *)STACKSTART) + (0x10000 * (p + 1));
		PROCS[p].parentpid = parent;
		PROCS[p].priority = priority;
		PROCS[p].pid = p + 1;
		PROCS[p].pstate = 0;
		PROCS[p].waiting_reply = 0;
		PROCS[p].waiting_send = 0;
		PROCS[p].waiting_recieve_head = 0;
		PROCS[p].waiting_recieve_tail = 0;
		PROCS[p].queuesize = 0;
		PROCS[p].totaltime = 0;
		for (int jdx = 0; jdx < 31; jdx++)
			PROCS[p].registervalues[jdx] = 0;
		scrSchedule(p + 1, PROCS[p].priority);
		return p + 1;
	}
	return -2;
}

void Kill(int p)
{
	PROCS[p].stackpointer = NULL;
	PROCS[p].pcpointer = NULL;
}

#include "kernel_api.h"

#define SCHED_EVENT_READY 0

void sched_enqueue(int pid, uint8_t priority)
{
	scrSchedule(pid, priority);
}

void sched_block(int pid, uint8_t priority)
{
	block(pid, priority);
}

int sched_unblock(int pid, uint8_t priority)
{
	return unblock_ind(pid, priority);
}

int sched_pick(void)
{
	return scrPick();
}

void sched_rescue_orphans(void)
{
	rescue_orphaned_tasks();
}

void kernel_schedule(void)
{
	Schedule();
}

int kernel_create(uint8_t priority, void (*fn)(void), int parent)
{
	return KernelCreate(priority, fn, parent);
}

uint8_t sched_compare_priority(uint8_t a_pri, uint64_t a_time, uint8_t b_pri, uint64_t b_time)
{
	struct state a = {0, a_pri, a_time};
	struct state b = {0, b_pri, b_time};
	return compare_state(a, b);
}

int sched_unblock_event(uint32_t interruptid, uint64_t ret)
{
	for (int i = 0; i < AWAIT_INTERRUPT[interruptid].len; i++)
	{
		struct state freed_state = AWAIT_INTERRUPT[interruptid].pid_ls[i];
		int p_free = freed_state.pid - 1;
		freed_state.time = SCHED_EVENT_READY;
		unblock(freed_state);
		PROCS[p_free].registervalues[0] = ret;
	}
	ret = AWAIT_INTERRUPT[interruptid].len;
	AWAIT_INTERRUPT[interruptid].len = 0;
	return (int)ret;
}
