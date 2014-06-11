#include <linux/slab.h>
#include <stddef.h>
#include "ringbuffer.h"

RingBuffer* ringBufferNew (size_t size, size_t msgLength) {
	RingBuffer* b = kmalloc (sizeof (RingBuffer), GFP_KERNEL);
	if (b == NULL) return NULL;

	if ((b->messages = kmalloc (size * msgLength, GFP_KERNEL)) == NULL) {
		kfree (b); return NULL;
	}
	if ((b->msgLengths = kmalloc (size * sizeof (size_t), GFP_KERNEL)) == NULL) {
		kfree (b->messages);
		kfree (b);
		return NULL;
	}
	spin_lock_init(&b->lock);
	b->msgLength = msgLength;
	b->size = size;
	b->wpointer = 0;
	b->firstReader = NULL;
	b->maxfill = 0;
	b->oldestMsg = 0;

	return b;
}

void ringBufferFree (RingBuffer* b) {
	kfree (b->messages);
	kfree (b->msgLengths);
	kfree (b);
}

RingReader* ringReaderNew (RingBuffer* b) {
	RingReader* r = kmalloc (sizeof (RingReader), GFP_KERNEL);
	if (r == NULL) return NULL;
	r->prev = NULL;
	r->buffer = b;

	spin_lock (&b->lock);

	r->pointer = b->oldestMsg;
	r->fill = b->maxfill;
	r->next = b->firstReader;
	if (b->firstReader != NULL)
		b->firstReader->prev = r;
	b->firstReader = r;

	printk ("Registering RingReader @ %d / %d\n", r->pointer, r->fill);

	spin_unlock (&b->lock);

	return r;
}

void ringReaderFree (RingReader* r) {
	RingBuffer* buffer = r->buffer;
	spin_lock (&buffer->lock);

	if (r->prev != NULL) {
		r->prev->next = r->next;
	} else {
		r->buffer->firstReader = r->next;
	}

	if (r->next != NULL) {
		r->next->prev = r->prev;
	}

	kfree (r);

	spin_unlock (&buffer->lock);
}

int ringBufferPut (RingBuffer* b, const char* fmt, ...) {
	spin_lock (&b->lock);

	size_t old = b->wpointer;
	char* message = &b->messages[old * b->msgLength];

	if (b->maxfill == b->size) {
		--(b->maxfill);
		if (b->oldestMsg == b->size - 1)
			b->oldestMsg = 0;
		else
			++(b->oldestMsg);
	}

	if (b->wpointer == b->size-1) {
		b->wpointer = 0;
	} else
		++(b->wpointer);

	for (RingReader* r = b->firstReader; r != NULL; r = r->next) {
		if (r->fill == b->size) {
			r->pointer = b->wpointer;
			--(r->fill);
		}
	}

	printk ("ringBufferPut -> %d\n", old);

	spin_unlock (&b->lock);


	va_list ap;
	va_start (ap, fmt);
	int n = vsnprintf (message, b->msgLength, fmt, ap);
	va_end(ap);

	if (n < 0) return -1;
	if (n >= b->msgLength) n = b->msgLength-1;

	b->msgLengths [old] = n;

	spin_lock (&b->lock);
	++(b->maxfill);
	printk ("maxfill -> %d\n", b->maxfill);
	for (RingReader* r = b->firstReader; r != NULL; r = r->next) {
		printk ("Reader fill %d -> %d\n", r->fill, r->fill + 1);
		++(r->fill);
	}
	spin_unlock (&b->lock);

	return n;
}

size_t ringBufferPeek (RingReader* reader, char** message, size_t* length) {
	if (reader->fill == 0) {
		return 0;
	}

	if (message != NULL)
		*message = &reader->buffer->messages [reader->buffer->msgLength * reader->pointer];

	if (length != NULL)
		*length = reader->buffer->msgLengths [reader->pointer];

	size_t rem = reader->fill;

	return rem;
}

int ringBufferConsume (RingReader* reader) {
	if (reader->fill == 0) {
		return 0;
	}
	--(reader->fill);
	if (reader->pointer == reader->buffer->size - 1) {
		reader->pointer = 0;
	} else {
		++(reader->pointer);
	}

	return 1;
}


void ringReaderLock (RingReader* reader) {
	spin_lock (&reader->buffer->lock);
}

void ringReaderUnLock (RingReader* reader) {
	spin_unlock (&reader->buffer->lock);
}
