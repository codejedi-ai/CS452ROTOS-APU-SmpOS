#ifndef TERMINAL_SHELL_H
#define TERMINAL_SHELL_H 1

/*
 * UART-only interactive shell for DarcyOS APU — no display server, no UI layer.
 * Created from kmain when START_SHELL is enabled.
 * Well below servers (0–20); idle stays at SCHED_LOWEST_PRIORITY (255).
 */
#define TERMINAL_SHELL_PRIORITY 20

void terminal_shell_entry(void);

#endif
