#ifndef KERNEL_STATE_H
#define KERNEL_STATE_H

#include "syscall.h"

extern void *STACKSTART;
extern uint32_t PID;
extern struct process PROCS[NUMPROCS];
extern struct state READY_QUEUE[NUMPROCS];
extern struct state BLOCKED_LIST[NUMPROCS];
extern struct interrupt AWAIT_INTERRUPT[MAXEVENT];
extern uint32_t kernelStartTime;

#endif
