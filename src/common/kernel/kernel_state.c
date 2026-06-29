#include "kernel_state.h"

void *STACKSTART;
uint32_t PID = 0;
struct process PROCS[NUMPROCS];
struct state READY_QUEUE[NUMPROCS];
struct state BLOCKED_LIST[NUMPROCS];
struct interrupt AWAIT_INTERRUPT[MAXEVENT];
uint32_t kernelStartTime = 0;
