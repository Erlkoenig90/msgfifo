#ifndef RINGBUFFER_H_
#define RINGBUFFER_H_

#include <linux/mutex.h>

struct RingBuffer;

typedef struct RingReader {
	struct RingBuffer* buffer;
	struct RingReader* next, *prev;
	size_t pointer, fill;
} RingReader;

typedef struct RingBuffer {
	spinlock_t lock;

	RingReader* firstReader;
	char* messages;
	size_t* msgLengths;
	size_t size, msgLength, wpointer, maxfill, oldestMsg;
} RingBuffer;

RingBuffer* ringBufferNew (size_t size, size_t msgLength);
void ringBufferFree (RingBuffer* b);

RingReader* ringReaderNew (RingBuffer* b);
void ringReaderFree (RingReader* r);

int ringBufferPut (RingBuffer* b, const char* fmt, ...);	// Returns >= 0 on success (= number of bytes written) and -1 on error
size_t ringBufferPeek (RingReader* reader, char** message, size_t* length);	// Fetches a pointer to the next message in the buffer without removing it. Returns the number of messages available. If 0 is returned, the pointers will not be assigned.
int ringBufferConsume (RingReader* reader);	// Removes the last message from the RingBuffer for this reader. Returns 0 on error, 1 on success
void ringReaderLock (RingReader* reader);
void ringReaderUnLock (RingReader* reader);

#endif /* RINGBUFFER_H_ */
