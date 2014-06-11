#ifndef FIFODEV_H_
#define FIFODEV_H_

#include <linux/fs.h>

#include "ringbuffer.h"


struct FifoDev {
	RingBuffer* rb;
	struct cdev* chrDev;
	struct device* devfile;
	dev_t id;
	int handle;
	struct FifoDev* next, *prev;
};
typedef struct FifoDev FifoDev;

void fifoDevsInit (void);
FifoDev* fifoDevNew (dev_t id);
void fifoDevFree (FifoDev* dev);

#endif /* FIFODEV_H_ */
