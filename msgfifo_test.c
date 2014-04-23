#include <stdio.h>
#include "msgfifo.h"

int flushFifo (MsgFifo* fifo) {
	MsgFifoEntry e;
	
	while (msgFifoSize (fifo) > 0) {
		e = msgFifoGet (fifo);
		if (e.message == NULL) {
			printf ("Get failed\n");
			return 0;
		}
		printf ("Got message: %s\n", e.message);
	}
	e = msgFifoGet (fifo);
	if (e.message != NULL) {
		printf ("Expected NULL Get\n");
		return 0;
	}
	return 1;
}

int main (void) {
	MsgFifo* fifo = msgFifoNew ();
	
	if (!msgFifoPut (fifo, "Hello %d %s", 42, "worlds!")) { printf ("Enqueue failed\n"); return -1; }
	if (!msgFifoPut (fifo, "Pi = %f", 3.14)) { printf ("Enqueue failed\n"); return -1; }
	
	printf ("Flush 1\n");
	if (!flushFifo (fifo)) { printf ("flush failed\n"); return -1; }

	if (!msgFifoPut (fifo, "blargh")) { printf ("Enqueue failed\n"); return -1; }
	
	printf ("Flush 2\n");
	if (!flushFifo (fifo)) { printf ("flush failed\n"); return -1; }
	
	
	return 0;
}
