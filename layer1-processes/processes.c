#include "processes.h"
#include "rpi.h"
#include "asm.h"
#include "syscall.h"
#include "util.h"

#define DISPLAY 1
/*
These are the most essential terminal control sequences that you will need for your train program.

Code	Effect
"\033[2J"	Clear the screen.
"\033[H"	Move the cursor to the upper-left corner of the screen.
"\033[r;cH"	Move the cursor to row r, column c. Note that both the rows and columns are indexed starting at 1.
"\033[?25l"	Hide the cursor.
"\033[K"	Delete everything from the cursor to the end of the line.
These control sequences can help make your program's display more lively.

Code	Effect
"\033[0m"	Reset special formatting (such as colour).
"\033[30m"	Black text.
"\033[31m"	Red text.
"\033[32m"	Green text.
"\033[33m"	Yellow text.
"\033[34m"	Blue text.
"\033[35m"	Magenta text.
"\033[36m"	Cyan text.
"\033[37m"	White text.

*/

#define UARTINTER 153
void init_trains(){
  Exit();
}

// new paradymn, run tests for each k# assignment (other than 3) before running the shell
void init_solonoids() // First task as dictated in the reqs
{	
	// Simplified version without service dependencies
	Exit();
}

// Context switching test tasks
// These tasks demonstrate context switching by alternating execution

void context_test_task_A() {
    for (int i = 0; i < 3; i++) {
        uart_printf(CONSOLE, "  [A-%d] Before Yield\r\n", i);
        Yield();
        uart_printf(CONSOLE, "  [A-%d] After Yield\r\n", i);
    }
    uart_printf(CONSOLE, "  [A] Exiting\r\n");
    Exit();
}

void context_test_task_B() {
    for (int i = 0; i < 3; i++) {
        uart_printf(CONSOLE, "    [B-%d] Before Yield\r\n", i);
        Yield();
        uart_printf(CONSOLE, "    [B-%d] After Yield\r\n", i);
    }
    uart_printf(CONSOLE, "    [B] Exiting\r\n");
    Exit();
}

void context_test_task_C() {
    for (int i = 0; i < 3; i++) {
        uart_printf(CONSOLE, "      [C-%d] Before Yield\r\n", i);
        Yield();
        uart_printf(CONSOLE, "      [C-%d] After Yield\r\n", i);
    }
    uart_printf(CONSOLE, "      [C] Exiting\r\n");
    Exit();
}

// Master task that creates the context switching test tasks
void context_switch_test() {
    uart_printf(CONSOLE, "\r\n========== CONTEXT SWITCHING TEST ==========\r\n");
    uart_printf(CONSOLE, "Creating 3 tasks to demonstrate context switching...\r\n\r\n");
    
    // Create test tasks with different priorities
    int tid_a = KernelCreate(1, context_test_task_A, 1);  
    int tid_b = KernelCreate(2, context_test_task_B, 1);  
    int tid_c = KernelCreate(3, context_test_task_C, 1);  
    
    uart_printf(CONSOLE, "Master: Created tasks - A(PID %d) B(PID %d) C(PID %d)\r\n", tid_a, tid_b, tid_c);
    uart_printf(CONSOLE, "Master: Yielding to let tasks run...\r\n\r\n");
    
    // Yield multiple times to allow test tasks to complete
    for (int i = 0; i < 15; i++) {
        uart_printf(CONSOLE, "[MASTER-%d] Yielding\r\n", i);
        Yield();
        uart_printf(CONSOLE, "[MASTER-%d] Resumed\r\n", i);
    }
    
    uart_printf(CONSOLE, "\r\n========== CONTEXT SWITCHING TEST COMPLETE ==========\r\n");
    Exit();
}

void first_user_task() {
    uart_printf(CONSOLE, "Hello from first_user_task\r\n");
    Exit();
}