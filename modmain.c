#include <linux/module.h>
#include <linux/version.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/sched.h>
#include <linux/signal.h>
#include <linux/completion.h>

#include "modmain.h"

typedef struct {
	struct task_struct* task;
	wait_queue_head_t wq;
	bool wantQuit;
	struct completion completion;
} MyThread;
MyThread threads [2];


DEFINE_MUTEX(cookie_mutex);


static int threadFun (void* data);
static void get_cookie (void *data, int noOfCookies);
static void startThread (MyThread* t, int index);
static void killThread (MyThread* t);


static int msgfifo_init (void) {
	printk ("MSG FIFO Module init\n");

	startThread (&threads [0], 0);
	startThread (&threads [1], 1);

	return 0;
}

static void msgfifo_exit (void) {
	killThread (&threads [0]);
	killThread (&threads [1]);
	printk ("msgfifo module exit\n");
}


static void startThread (MyThread* t, int index) {
	t->wantQuit = false;
	init_waitqueue_head(&t->wq);
	init_completion (&t->completion);

	t->task = kthread_run(&threadFun, t, "race_test_fun_%d", index);
}

static void killThread (MyThread* t) {
	printk ("Killing Thread %s\n", t->task->comm);


	t->wantQuit = true;
	wake_up (&t->wq);
	wait_for_completion (&t->completion);
//	kill_pid (task_pid (t->task), SIGKILL, 0);
}

static int threadFun (void* data) {
	MyThread* t = (MyThread*) data;
	printk ("Thread %s starting\n", t->task->comm);
	int ret;

	while ((ret = wait_event_interruptible_timeout (t->wq, t->wantQuit, HZ / 10)) == 0 /* timeout */) {
		/*
		 * Hier Mutex auskommentieren um race conditions zu sehen
		 */
		mutex_lock (&cookie_mutex);
		get_cookie (t->task->comm, 2);
		mutex_unlock (&cookie_mutex);
	}
	printk ("Thread %s quitting\n", t->task->comm);
	complete (&t->completion);
	return 0;
}

static void get_cookie (void *data, int noOfCookies) {
	static int cookiesInTheBox = 10;
	if ((cookiesInTheBox - noOfCookies) >= 0) {
		printk ("Get_cookie: will hand over %d cookies to thread %s\n", noOfCookies, (char*) data);
		msleep (1000); //sleep a sec
		cookiesInTheBox -= noOfCookies;
	} else
		printk ("Not enough cookies available: %d in the box\n", cookiesInTheBox);
	printk ("Leaving >>get_cookie<< with %d cookies in the box\n", cookiesInTheBox);
}

module_init(msgfifo_init);
module_exit(msgfifo_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Niklas Guertler");
MODULE_DESCRIPTION("Message FIFO test module");
