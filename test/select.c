#include <sys/select.h>
#include <stdlib.h>
#include <stdio.h>
#include "htimer.h"


int count = 0;

void cb(htimer_t *handle)
{
	count++;
}

int main(int argc, char **argv)
{
	fd_set rfds;
	struct timeval tv;
	int rc;
	int i;

	htimer_mgr_t mgr;
	htimer_mgr_init(&mgr);

	FD_ZERO(&rfds);
	FD_SET(0, &rfds);
	tv.tv_sec = 0;

	for (i = 0; i < 10000; i++) {
		htimer_t *timer = malloc(sizeof(*timer));
		htimer_init(&mgr, timer);
		htimer_start(timer, cb, 0, 100);
	}

	while (1) {
		char line[1024];
		int last;
		int next = htimer_next_timeout(&mgr);
		tv.tv_usec = next * 1e3;
		rc = select(1, &rfds, NULL, NULL, &tv);
		if (rc < 0) {
			break;
		}
		printf("on timer\n");
		last = count;
		htimer_perform(&mgr);
		if (count - last > 0)
			printf("t=%llu,%d\n", mgr.time, count);
	}
	return EXIT_SUCCESS;
}

