#include <linux/slab.h>
#include <linux/cdev.h>
#include <linux/export.h>
#include <asm/uaccess.h>

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


static FifoDev* registeredDevs = NULL;

static struct mutex mtxDevList;

void fifoDevsInit (void) {
	mutex_init (&mtxDevList);
}


FifoDev* fifoDevNew (dev_t id) {
	FifoDev* dev = kmalloc (sizeof (FifoDev), GFP_KERNEL);
	if (dev == NULL) return NULL;
	dev->id = id;
	dev->handle = 0;

	if ((dev->rb = ringBufferNew (10, 1024)) == NULL) {
		kfree (dev);
		return NULL;
	}

	if ((dev->chrDev = cdev_alloc ()) == NULL) {
		ringBufferFree(dev->rb);
		kfree (dev);
		return NULL;
	}
	if (kobject_set_name (&dev->chrDev->kobj, "Message Fifo Read End %d:%d", MAJOR (id), MINOR (id)) < 0) {
		cdev_del (dev->chrDev);
		ringBufferFree(dev->rb);
		kfree (dev);
		return NULL;
	}

	dev->chrDev->owner = THIS_MODULE;
	cdev_init (dev->chrDev, &ffdev_fops);

	if (cdev_add (dev->chrDev, id, 1) < 0) {
		cdev_del (dev->chrDev);
		ringBufferFree(dev->rb);
		kfree (dev);
		return NULL;
	}

	if ((void*) (dev->devfile = device_create (ffdev_class, NULL, id, (void*) dev, "msgfifo_%d", MINOR (id))) == ERR_PTR) {
		cdev_del (dev->chrDev);
		ringBufferFree(dev->rb);
		kfree (dev);
		return NULL;
	}

	dev->prev = NULL;
	mutex_lock (&mtxDevList);
	if (registeredDevs == NULL) {
		dev->next = NULL;
		registeredDevs = dev;
	} else {
		dev->next = registeredDevs;
		registeredDevs->prev = dev;
		registeredDevs = dev;
	}
	mutex_unlock (&mtxDevList);

	return dev;
}

void fifoDevFree (FifoDev* dev) {
	mutex_lock (&mtxDevList);

	if (dev->prev != NULL) {
		dev->prev->next = dev->next;
	} else {
		registeredDevs = dev;
	}
	if (dev->next != NULL) {
		dev->next->prev = dev->prev;
	}
	mutex_unlock (&mtxDevList);

	cdev_del (dev->chrDev);
	ringBufferFree(dev->rb);
	device_destroy (ffdev_class, dev->id);
	kfree (dev);
}

FifoDev* getDevByID (dev_t id) {
	mutex_lock (&mtxDevList);
	for (FifoDev* dev = registeredDevs; dev != NULL; dev = dev->next) {
		if (dev->id == id) {
			mutex_unlock (&mtxDevList);
			return dev;
		}
	}
	mutex_unlock (&mtxDevList);
	return NULL;
}

int ffdev_open (struct inode* in, struct file* filp) {
	FifoDev* dev = getDevByID (in->i_rdev);
	if (dev == NULL) return -ENODEV;

	RingBuffer* b = dev->rb;

	RingReader* r = ringReaderNew (b);

	filp->private_data = (void*) r;

	return 0;
}

ssize_t ffdev_read (struct file* filp, char __user * buffer, size_t size, loff_t* off) {
	char* msg; size_t len;
	RingReader* r = (RingReader*) filp->private_data;

	ringReaderLock (r);

//	printk ("Peek...\n");
	if (ringBufferPeek(r, &msg, &len) == 0) {
//		printk("Peek == 0 ... Unlocking \n");
		ringReaderUnLock (r);
//		printk ("Unlocked\n");
		return 0;
	}
	if (len > size) {
		ringReaderUnLock (r);
		return -ENOMEM;
	}

	unsigned long ret = copy_to_user (buffer, msg, len);

	ringBufferConsume (r);
	ringReaderUnLock (r);

	return ret == 0 ? len : -EIO;
}

int ffdev_close (struct inode * in, struct file* filp) {
	ringReaderFree((RingReader*) filp->private_data);
	return -1;
}
