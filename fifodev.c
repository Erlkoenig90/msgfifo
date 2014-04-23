#include<linux/slab.h>
#include <linux/cdev.h>
#include <linux/export.h>

#include "fifodev.h"

struct FifoDev {
	MsgFifo* fifo;
	struct cdev* chrDev;
};

FifoDev* fifoDevNew (dev_t id) {
	FifoDev* dev = kmalloc (sizeof (FifoDev), GFP_KERNEL);
	if (dev == NULL) return NULL;

	if ((dev->fifo = msgFifoNew ()) == NULL) {
		kfree (dev);
		return NULL;
	}

	if ((dev->chrDev = cdev_alloc ()) == NULL) {
		msgFifoFree (dev->fifo);
		kfree (dev);
		return NULL;
	}
	dev->chrDev->owner = THIS_MODULE;
	return dev;

	// TODO ...
	if (cdev_add (dev->chrDev, id, 1) < 0) {
		cdev_del (dev->chrDev);
		msgFifoFree (dev->fifo);
		kfree (dev);
		return NULL;
	}
	return dev;
}

void fifoDevFree (FifoDev* dev) {
	cdev_del (dev->chrDev);
	msgFifoFree (dev->fifo);
	kfree (dev);
}
