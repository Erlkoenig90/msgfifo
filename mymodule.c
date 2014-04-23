#include <linux/module.h>
#include <linux/version.h>

static int mymodule_init (void) {
	printk("Mymodule init\n");
	return 0;
}

static void mymodule_exit (void) {
	printk("Mymodule exit\n");
}

module_init(mymodule_init);
module_exit(mymodule_exit);

MODULE_LICENSE("BSD"); 
MODULE_AUTHOR("Niklas Guertler");
MODULE_DESCRIPTION("Test module");
