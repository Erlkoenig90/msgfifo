#ifndef MSGFIFO_HH
#define MSGFIFO_HH

#include <stddef.h>

// Strong typedef
typedef struct {
	char* message;
} MsgFifoEntry;

struct MsgFifo;
typedef struct MsgFifo MsgFifo;

MsgFifo* msgFifoNew (void);
void msgFifoFree (MsgFifo* mf);

// Put FIFO message. returns 1 on success, 0 on failure
int msgFifoPut (MsgFifo* mf, const char* fmt, ...);
MsgFifoEntry msgFifoGet (MsgFifo* mf);
void msgFifoEntryFree (MsgFifoEntry entry);
size_t msgFifoSize (MsgFifo* mf);


#endif
