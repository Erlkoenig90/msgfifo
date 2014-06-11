#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>

const char* filename = "/dev/msgfifo_0";
char buffer [1025];

static int fd = -1;
static int run = 1;

void onclose (int sig) {
	if (fd != -1) {
		close (fd);
		fd = -1;
	}
	run = 0;
}

int main (int argc, char* argv []) {
	signal (SIGTERM, &onclose);
	signal (SIGQUIT, &onclose);
	signal (SIGINT, &onclose);
	signal (SIGKILL, &onclose);
	signal (SIGHUP, &onclose);

	int sleeptime = 100000;
	if (argc >= 2)
		sleeptime = atoi (argv [1]);

	fd = open (filename, O_RDONLY);

	if (fd == -1) {
		fprintf (stderr, "Opening \"%s\" failed: %s\n", filename, strerror (errno));
		return -1;
	}

	printf ("Successfully opened!\n");
	fflush (stdout);
//	sleep (1);

	while (run) {
//		printf ("Reading...\n");
		fflush (stdout);

		int r = read (fd, buffer, 1024);
		if (r == -1) {
			fprintf (stderr, "Read failed: %s\n", strerror (errno));
			close (fd);
			return 1;
		}
//		printf ("Read %d.\n", r);
		fflush (stdout);

		if (r > 0) {
			fwrite (buffer, 1, r, stdout);
			puts ("");
		} else
			usleep (sleeptime);
	}

	printf ("Closing...\n"); fflush (stdout);
	if (fd != -1) close (fd);
	printf ("Closed.\n"); fflush (stdout);
	return 0;
}
