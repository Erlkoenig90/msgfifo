#include <linux/module.h>
#include <linux/version.h>
#include <linux/fs.h>

#include "msgfifo.h"

static dev_t chrDevID;

static int msgfifo_init (void) {
	printk("MSG FIFO Module init\n");

	if (alloc_chrdev_region (&chrDevID, 0, 1, "msgfifo") == -1) {
		printk ("Device ID allocation failed!\n");
		chrDevID = -1;
	} else {
		printk ("MSG FIFO Got device ID %d (%d:%d)\n", chrDevID, MAJOR (chrDevID), MINOR (chrDevID));
	}

	return 0;
}

static void msgfifo_exit (void) {
	printk("MSG FIFO Module exit\n");
	if (chrDevID != -1) {
		unregister_chrdev_region (chrDevID, 1);
	}
}

module_init(msgfifo_init);
module_exit(msgfifo_exit);

MODULE_LICENSE("BSD"); 
MODULE_AUTHOR("Niklas Guertler");
MODULE_DESCRIPTION("Message FIFO test module");
