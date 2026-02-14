#include "rpi.h"
#include "syscall.h"
#include "processes.h"
#include "gic.h"

void context_switch_test(void);
void run_layer2_tests(void);

void* STACK_EL0_START; // Maybe delete this later
#define CLOCKINTID 99
#define CLOCKSERVERON 1
#define UARTSERVERON 1
void idle(){
	int count = 0;
	while(1){
    		// uart_printf(CONSOLE, "idle: WFI <Print time here>\r\n");
		// uart_printf(CONSOLE, "idle: time = %u\r\n", time);
		uint32_t runtime = GetRuntime();
		uint32_t kernelrt = GetKernelRuntime();
    // print the column and row onto 2 and 1
    uart_printf(CONSOLE, "\033[2;1H");
		uart_printf(CONSOLE, "idle: [%d] runprecentage = %u %% \r\n", count++, (100 * runtime) / kernelrt);
		asm("WFI");
	}
	Exit();
}

int kmain(void *reg) {

  STACK_EL0_START = reg; // Immediately calls this to store stack_end point as x0
  
  // Early boot diagnostics
  uart_init();
  uart_config_and_enable(CONSOLE, 115200);
  uart_putc(CONSOLE, '[');
  uart_putc(CONSOLE, '1');
  uart_putc(CONSOLE, ']');
  uart_putc(CONSOLE, '\r');
  uart_putc(CONSOLE, '\n');

  InitSys(reg);
  
  uart_putc(CONSOLE, '[');
  uart_putc(CONSOLE, '2');
  uart_putc(CONSOLE, ']');
  uart_putc(CONSOLE, '\r');
  uart_putc(CONSOLE, '\n');

  // Don't create idle task during testing - it interferes with messaging tests
  // int tid = KernelCreate(255, idle, 0);
  // (void)tid; // Unused for now
  
  uart_putc(CONSOLE, '[');
  uart_putc(CONSOLE, '3');
  uart_putc(CONSOLE, ']');
  uart_putc(CONSOLE, '\r');
  uart_putc(CONSOLE, '\n');
  
  #if CLOCKSERVERON == 1
  // Basic timer setup
  route_interrupt(CLOCKINTID, 0);
  enable_interrupt(CLOCKINTID);
  
  uart_putc(CONSOLE, '[');
  uart_putc(CONSOLE, '4');
  uart_putc(CONSOLE, ']');
  uart_putc(CONSOLE, '\r');
  uart_putc(CONSOLE, '\n');
  #endif

  // Create messaging test task with low priority so created threads run first
  int tid = KernelCreate(10, run_layer2_tests, 0);
  
  uart_putc(CONSOLE, '[');
  uart_putc(CONSOLE, '5');
  uart_putc(CONSOLE, ']');
  uart_putc(CONSOLE, '\r');
  uart_putc(CONSOLE, '\n');

  Schedule();
  // U-Boot displays the return value from main - might be handy for debugging

  return 0;
}
