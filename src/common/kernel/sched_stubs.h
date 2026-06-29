#ifndef SCHED_STUBS_H
#define SCHED_STUBS_H

#include <stdint.h>
#include "syscall.h"

#define SCHED_READY  0
#define SCHED_BLOCKED 1

void sched_init(void *reg);
void scrSchedule(int pid, uint8_t priority);
int scrPick(void);
void block(int pid, uint8_t priority);
int unblock_ind(int pid, uint8_t priority);
void unblock(struct state currItem);
void updateRunTimer(void);
int8_t dead(int8_t p);
void Schedule(void);
int KernelCreate(uint8_t priority, void (*function)(), int parent);
void Kill(int p);

#endif
