#include <linux/slab.h>
#include <linux/cdev.h>
#include <linux/export.h>

#include "fifodev.h"
#include "modmain.h"

/*
struct file_operations {
	struct module *owner;
	loff_t (*llseek) (struct file *, loff_t, int);
	ssize_t (*read) (struct file *, char __user *, size_t, loff_t *);
	ssize_t (*write) (struct file *, const char __user *, size_t, loff_t *);
	ssize_t (*aio_read) (struct kiocb *, const struct iovec *, unsigned long, loff_t);
	ssize_t (*aio_write) (struct kiocb *, const struct iovec *, unsigned long, loff_t);
	int (*iterate) (struct file *, struct dir_context *);
	unsigned int (*poll) (struct file *, struct poll_table_struct *);
	long (*unlocked_ioctl) (struct file *, unsigned int, unsigned long);
	long (*compat_ioctl) (struct file *, unsigned int, unsigned long);
	int (*mmap) (struct file *, struct vm_area_struct *);
	int (*open) (struct inode *, struct file *);
	int (*flush) (struct file *, fl_owner_t id);
	int (*release) (struct inode *, struct file *);
	int (*fsync) (struct file *, loff_t, loff_t, int datasync);
	int (*aio_fsync) (struct kiocb *, int datasync);
	int (*fasync) (int, struct file *, int);
	int (*lock) (struct file *, int, struct file_lock *);
	ssize_t (*sendpage) (struct file *, struct page *, int, size_t, loff_t *, int);
	unsigned long (*get_unmapped_area)(struct file *, unsigned long, unsigned long, unsigned long, unsigned long);
	int (*check_flags)(int);
	int (*flock) (struct file *, int, struct file_lock *);
	ssize_t (*splice_write)(struct pipe_inode_info *, struct file *, loff_t *, size_t, unsigned int);
	ssize_t (*splice_read)(struct file *, loff_t *, struct pipe_inode_info *, size_t, unsigned int);
	int (*setlease)(struct file *, long, struct file_lock **);
	long (*fallocate)(struct file *file, int mode, loff_t offset,
			  loff_t len);
	int (*show_fdinfo)(struct seq_file *m, struct file *f);
};
 */

int ffdev_open (struct inode* in, struct file* filp);
ssize_t ffdev_read (struct file* filp, char* buffer, size_t size, loff_t* off);
int ffdev_close (struct inode * in, struct file* filp);

static const struct file_operations ffdev_fops = {
		.owner = THIS_MODULE,
		.read = &ffdev_read,
		.open = &ffdev_open,
		.release = &ffdev_close,
};


struct FifoDev {
	MsgFifo* fifo;
	struct cdev* chrDev;
	struct device* sysfsdev;
	dev_t id;
};

FifoDev* fifoDevNew (dev_t id) {
	FifoDev* dev = kmalloc (sizeof (FifoDev), GFP_KERNEL);
	if (dev == NULL) return NULL;
	dev->id = id;

	if ((dev->fifo = msgFifoNew ()) == NULL) {
		kfree (dev);
		return NULL;
	}

	if ((dev->chrDev = cdev_alloc ()) == NULL) {
		msgFifoFree (dev->fifo);
		kfree (dev);
		return NULL;
	}
	if (kobject_set_name (&dev->chrDev->kobj, "Message Fifo Read End %d:%d", MAJOR (id), MINOR (id)) < 0) {
		cdev_del (dev->chrDev);
		msgFifoFree (dev->fifo);
		kfree (dev);
		return NULL;
	}

	dev->chrDev->owner = THIS_MODULE;
	cdev_init (dev->chrDev, &ffdev_fops);

	if (cdev_add (dev->chrDev, id, 1) < 0) {
		cdev_del (dev->chrDev);
		msgFifoFree (dev->fifo);
		kfree (dev);
		return NULL;
	}


	if ((void*)(dev->sysfsdev = device_create (ffdev_class, NULL, id, (void*) dev, "msgfifo_%d", MINOR (id))) == ERR_PTR) {
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
	device_destroy (ffdev_class, dev->id);
	kfree (dev);
}

int ffdev_open (struct inode* in, struct file* filp) {
	return -1;
}

ssize_t ffdev_read (struct file* filp, char* buffer, size_t size, loff_t* off) {
	return -1;
}

int ffdev_close (struct inode * in, struct file* filp) {
	return -1;
}
