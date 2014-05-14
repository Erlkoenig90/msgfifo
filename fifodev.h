#ifndef FIFODEV_H_
#define FIFODEV_H_

#include <linux/fs.h>

#include "msgfifo.h"


struct FifoDev {
	MsgFifo* fifo;
	struct cdev* chrDev;
	struct device* devfile;
	dev_t id;
	int handle;
};
typedef struct FifoDev FifoDev;

FifoDev* fifoDevNew (dev_t id);
void fifoDevFree (FifoDev* dev);

#endif /* FIFODEV_H_ */
