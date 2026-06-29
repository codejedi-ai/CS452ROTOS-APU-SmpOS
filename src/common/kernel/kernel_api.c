#include "kernel_api.h"

__attribute__((weak)) void kernel_init_extras(void *reg)
{
	(void)reg;
}

__attribute__((weak)) int kernel_my_core_id(void)
{
	return 0;
}

__attribute__((weak)) void kernel_mailbox_notify(int dest_core, int slot)
{
	(void)dest_core;
	(void)slot;
}

int ipc_send(int tid, const char *msg, int msglen, char *reply, int replylen)
{
	(void)tid;
	(void)msg;
	(void)msglen;
	(void)reply;
	(void)replylen;
	asm("svc 5");
	return 0;
}

int ipc_receive(int *tid, char *msg, int msglen)
{
	(void)tid;
	(void)msg;
	(void)msglen;
	asm("svc 6");
	return 0;
}

int ipc_reply(int tid, const void *reply, int replylen)
{
	(void)tid;
	(void)reply;
	(void)replylen;
	asm("svc 7");
	return 0;
}

int kernel_await_event(int event_id)
{
	(void)event_id;
	asm("svc 10");
	return 0;
}
