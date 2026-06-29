#ifndef NAMESERVER_H
#define NAMESERVER_H

#include <stdint.h>

#define NAME_SERVER_PRIORITY  3
#define NS_NAME_MAX 32
#define NS_MAX_NAMES 16

typedef enum {
	NS_MSG_REGISTER = 1,
	NS_MSG_WHOIS = 2,
} NameServerMsgType;

typedef struct {
	int type;
	char name[NS_NAME_MAX];
} NameServerMsg;

void name_server_entry(void);
int NameServerTid(void);
/* Set the name server's tid synchronously. Must be called from kmain right
 * after KernelCreate(name_server_entry, ...) so the value is visible before
 * any other task runs RegisterAs/WhoIs. Without this, name_server_entry's
 * own `ns_tid = MyTid()` only stores after MyTid yields, by which point
 * higher-FIFO-priority tasks have already raced past their RegisterAs. */
void NameServerSetTid(int tid);
void RegisterAs(const char *name);
int WhoIs(const char *name);

#endif
