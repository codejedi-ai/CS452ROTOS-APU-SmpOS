#include "ipc_stubs.h"
#include "kernel_state.h"
#include "sched_stubs.h"
#include "syscall.h"

extern void *memcpy(void *restrict dest, const void *restrict src, unsigned long n);

static inline uint64_t ipc_min(uint64_t a, uint64_t b)
{
	return a < b ? a : b;
}

void send_helper(void)
{
	int p = (int)PID - 1;
	int tid = (int)PROCS[p].registervalues[0];
	char *msg = (char *)PROCS[p].registervalues[1];
	uint64_t msglen = PROCS[p].registervalues[2];
	char *reply = (char *)PROCS[p].registervalues[3];
	uint64_t replylen = PROCS[p].registervalues[4];

	PROCS[p].message_sent.tid = tid;
	PROCS[p].message_sent.msg = msg;
	PROCS[p].message_sent.msglen = msglen;
	PROCS[p].message_sent.reply = reply;
	PROCS[p].message_sent.replylen = replylen;
	PROCS[p].waiting_reply = 1;

	int p_to = tid - 1;
	int tail = (int)PROCS[p_to].waiting_recieve_tail;
	PROCS[p_to].message_recieved[tail].tid = (int)PID;
	PROCS[p_to].message_recieved[tail].msg = msg;
	PROCS[p_to].message_recieved[tail].msglen = msglen;
	PROCS[p_to].message_recieved[tail].reply = reply;
	PROCS[p_to].message_recieved[tail].replylen = replylen;
	PROCS[p_to].waiting_recieve_tail++;
	PROCS[p_to].waiting_recieve_tail %= QUEUESIZE;
	PROCS[p_to].queuesize++;

	if (PROCS[p_to].waiting_send == 1)
		recieve_helper(tid);
}

void recieve_helper(int recv_tid)
{
	int p = recv_tid - 1;
	int head = (int)PROCS[p].waiting_recieve_head;
	int tail = (int)PROCS[p].waiting_recieve_tail;

	int *tid = (int *)PROCS[p].registervalues[0];
	char *msg = (char *)PROCS[p].registervalues[1];
	int msglen = (int)PROCS[p].registervalues[2];

	if (head == tail)
	{
		PROCS[p].waiting_send = 1;
		return;
	}
	PROCS[p].waiting_send = 0;

	uint64_t curread_message_length = PROCS[p].message_recieved[head].msglen;
	char *curread_msg = PROCS[p].message_recieved[head].msg;
	int sender_tid = (int)PROCS[p].message_recieved[head].tid;
	*tid = sender_tid;

	msglen = (int)ipc_min((uint64_t)msglen, curread_message_length);
	memcpy(msg, curread_msg, (unsigned long)msglen);

	PROCS[p].waiting_recieve_head++;
	if (PROCS[p].queuesize > 0)
		PROCS[p].queuesize--;
	PROCS[p].waiting_recieve_head %= QUEUESIZE;
	PROCS[p].registervalues[0] = (uint64_t)msglen;
	unblock_ind(recv_tid, PROCS[p].priority);
}

void reply_helper(void)
{
	int p = (int)PID - 1;
	int tid = (int)PROCS[p].registervalues[0];
	char *reply = (char *)PROCS[p].registervalues[1];
	uint64_t replylen = PROCS[p].registervalues[2];
	char *reply_buffer = PROCS[tid - 1].message_sent.reply;
	uint64_t reply_buffer_len = PROCS[tid - 1].message_sent.replylen;

	replylen = ipc_min(reply_buffer_len, replylen);
	memcpy(reply_buffer, reply, (unsigned long)replylen);

	unblock_ind(tid, PROCS[tid - 1].priority);
	PROCS[tid - 1].registervalues[0] = replylen;
	PROCS[tid - 1].waiting_reply = 0;
	PROCS[p].registervalues[0] = replylen;
}

void ipc_kernel_send(void)
{
	send_helper();
}

void ipc_kernel_receive(int recv_tid)
{
	recieve_helper(recv_tid);
}

void ipc_kernel_reply(void)
{
	reply_helper();
}
