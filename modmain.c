#include <linux/module.h>
#include <linux/version.h>
#include <linux/fs.h>
#include <linux/cdev.h>

#include "ringbuffer.h"
#include "fifodev.h"
#include "modmain.h"

static dev_t chrDevID = -1;
static FifoDev* fifoDev = NULL;
struct class* ffdev_class;
struct workqueue_struct* msgTimerQueue = NULL;
static char controlState = 0;

static void onMsgTimer (struct work_struct *work);

DECLARE_DELAYED_WORK(fifo_put_timer, &onMsgTimer);


static int msgfifo_init (void) {
	printk("MSG FIFO Module init\n");

	controlState = 0;
	if ((msgTimerQueue = create_singlethread_workqueue ("msgfifo_queue")) == NULL) {
		printk ("create_singlethread_workqueue failed!\n");
		return -ENOMEM;
	}
	if (!queue_delayed_work (msgTimerQueue, &fifo_put_timer, HZ)) {
		printk ("queue_delayed_work failed!\n");
		destroy_workqueue (msgTimerQueue);
		msgTimerQueue = NULL;
		return -ENOMEM;
	}

	if (IS_ERR (ffdev_class = class_create (THIS_MODULE, "msgfifo"))) {
		printk ("msgfifo class creation failed!\n");
		destroy_workqueue (msgTimerQueue);
		msgTimerQueue = NULL;
		return -ENOMEM;
	}

	if (alloc_chrdev_region (&chrDevID, 0, 1, "msgfifo") == -1) {
		printk ("Device ID allocation failed!\n");
		chrDevID = -1;
		class_destroy (ffdev_class);
		destroy_workqueue (msgTimerQueue);
		msgTimerQueue = NULL;
		return -ENOMEM;
	} else {
		printk ("MSG FIFO Got device ID %d (%d:%d)\n", chrDevID, MAJOR (chrDevID), MINOR (chrDevID));
		fifoDevsInit ();

		if ((fifoDev = fifoDevNew (chrDevID)) == NULL) {
			printk ("FIFO device creation failed!\n");
			unregister_chrdev_region (chrDevID, 1);
			chrDevID = -1;
			class_destroy (ffdev_class);
			destroy_workqueue (msgTimerQueue);
			msgTimerQueue = NULL;
			return -ENOMEM;
		} else {
			printk ("FIFO device created.\n");
		}
	}

	return 0;
}

static void msgfifo_exit (void) {
	printk("MSG FIFO Module exit\n");
	if (msgTimerQueue  != NULL) {
		controlState = 1;
		if (!cancel_delayed_work (&fifo_put_timer)) {
			while (controlState != 2);
			flush_workqueue (msgTimerQueue);
		}
		destroy_workqueue (msgTimerQueue);
	}
	if (chrDevID != -1) {
		unregister_chrdev_region (chrDevID, 1);
	}
	if (fifoDev != NULL) {
		fifoDevFree(fifoDev);
	}
	if (ffdev_class != NULL) {
		class_destroy (ffdev_class);
	}
}

static void onMsgTimer (struct work_struct *work) {
	if (controlState > 0) { controlState = 2; return; }
	printk ("queued work gets executed\n");

	if (!queue_delayed_work (msgTimerQueue, &fifo_put_timer, HZ)) {
		printk ("queue_delayed_work failed!\n");
	}

	static int i = 0;
	ringBufferPut(fifoDev->rb, "Work %d", ++i);
}

module_init(msgfifo_init);
module_exit(msgfifo_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Niklas Guertler");
MODULE_DESCRIPTION("Message FIFO test module");
