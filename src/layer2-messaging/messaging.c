#include "messaging.h"
#include "layer0.h"

int Send(int tid, const char *msg, int msglen, char *reply, int replylen)
{
	(void)tid; (void)msg; (void)msglen; (void)reply; (void)replylen;
	return asm_svc_5();
}

int Receive(int *tid, char *msg, int msglen)
{
	(void)tid; (void)msg; (void)msglen;
	return asm_svc_6();
}

int Reply(int tid, const void *reply, int replylen)
{
	(void)tid; (void)reply; (void)replylen;
	return asm_svc_7();
}
