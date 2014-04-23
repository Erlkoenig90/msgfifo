#include "msgfifo.h"
#include <stddef.h>
#include <stdarg.h>

#ifdef MSGFIFO_USER
#include <stdlib.h>
#include <stdio.h>
	static inline void* mallocWrapper (size_t s) {
		return malloc (s);
	}
	static inline void freeWrapper (void* p) {
		free (p);
	}
	static inline void* reallocWrapper (void* p, size_t s) {
		return realloc (p, s);
	}
#else
#include<linux/slab.h>
	static inline void* mallocWrapper (size_t s) {
		return kmalloc (s, GFP_KERNEL);
	}
	static inline void freeWrapper (void* p) {
		kfree (p);
	}
	static inline void* reallocWrapper (void* p, size_t s) {
		return krealloc (p, s, GFP_KERNEL);
	}
#endif


typedef struct MsgFifoEntryInt {
	struct MsgFifoEntryInt* prev;
	char message [];
} MsgFifoEntryInt;

struct MsgFifo {
	MsgFifoEntryInt* first, *last;
	size_t size;
};

MsgFifo* msgFifoNew (void) {
	MsgFifo* f = (MsgFifo*) mallocWrapper (sizeof (MsgFifo));
	if (f != NULL) {
		f->first = f->last = NULL;
		f->size = 0;
	}
	return f;
}

void msgFifoFree (MsgFifo* mf) {
	freeWrapper (mf);
}

size_t msgFifoSize (MsgFifo* mf) {
	return mf->size;
}

int msgFifoPut (MsgFifo* mf, const char* fmt, ...) {
	int size = 100;
	MsgFifoEntryInt* p = NULL;

	while (1) {
		MsgFifoEntryInt* np = (MsgFifoEntryInt*) reallocWrapper ((void*) p, size + sizeof (MsgFifoEntryInt));
		if (np == NULL) {
			if (p != NULL) freeWrapper ((void*) p);
			return 0;
		} else
			p = np;

		va_list ap;
		va_start (ap, fmt);
		int n = vsnprintf (p->message, size, fmt, ap);
		va_end(ap);

		if (n < 0) {
			freeWrapper ((void*) p);
			return 0;
		} else if (n < size) {
			++mf->size;
			p->prev = NULL;
			if (mf->first == NULL) {
				mf->last = p;
				mf->first = p;
			} else {
				mf->last->prev = p;
				mf->last = p;
			}
			return 1;
		}

		size = n + 1;
	}
}

MsgFifoEntry msgFifoGet (MsgFifo* mf) {
	MsgFifoEntryInt* p = mf->first;
	MsgFifoEntry ret;

	if (p == NULL) {
		ret.message = NULL;
		return ret;
	}
	--mf->size;
	mf->first = p->prev;
	if (p->prev == NULL) mf->last = NULL;

	ret.message = p->message;
	return ret;
}

void msgFifoEntryFree (MsgFifoEntry entry) {
	freeWrapper (((void*) (entry.message)) - offsetof (MsgFifoEntryInt, message));
}


