#ifndef IPC_STUBS_H
#define IPC_STUBS_H

#include <stdint.h>

void send_helper(void);
void recieve_helper(int recv_tid);
void reply_helper(void);

void ipc_handle_send(int p);
void ipc_handle_receive(int p);
void ipc_handle_reply(int p);

#endif
