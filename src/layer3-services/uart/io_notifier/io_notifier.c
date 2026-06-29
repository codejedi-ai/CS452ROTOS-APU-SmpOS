#include "io_notifier.h"
#include "io_api.h"
#include "UART1_CONSOLE_server.h"
#include "../../layer1-processes/processes.h"
#include "../../layer1-processes/rpi.h"
#include "../../layer1-processes/syscall.h"
#include "../../nameserver.h"

void io_notifier(void)
{
	/* Register synchronously so UART1_CONSOLE_server's WhoIs("io_notifier")
	 * resolves before our first Send. This relies on name_server being the
	 * first task created in kmain (so ns_tid is set before this runs). */
	RegisterAs("io_notifier");

	int uart2_marklin_tid = -1;
	int uart1_console_tid = -1;

	while (1)
	{
		uint64_t event = AwaitEvent(UARTINTER);
		int ret;

		/* Look up server TIDs AFTER AwaitEvent — those servers register later
		 * than this notifier, so a pre-AwaitEvent lookup caches -1 and the
		 * first UART RX event would be silently dropped. */
		if (uart1_console_tid <= 0)
			uart1_console_tid = WhoIs(UART1_CONSOLE_SERVER);
		if (uart2_marklin_tid <= 0)
			uart2_marklin_tid = WhoIs(UART2_MARKLIN_SERVER);

		uint8_t type = event & 0xFF;
		uint8_t channel = (event >> 8) & 0xFF;

		if (channel == CONSOLE)
		{
			if (uart1_console_tid > 0)
				Send(uart1_console_tid, (const char *)&event, 8, (char *)&ret, 0);
		}
		else if (channel == MARKLIN && uart2_marklin_tid > 0)
		{
			if (type == RXIC || type == TXIC || type == CTSMIM)
				Send(uart2_marklin_tid, (const char *)&event, 8, (char *)&ret, 0);
		}
	}
	Exit();
}
