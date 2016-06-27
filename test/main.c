#include "htimer.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>

int count = 0;

void cb(htimer_t *handle)
{
	count++;
}

int main(int argc, char **argv)
{
	int i;
	htimer_mgr_t mgr;
	htimer_mgr_init(&mgr);

	for (i = 0; i < 10000; i++) {
		htimer_t *timer = malloc(sizeof(*timer));
		htimer_init(&mgr, timer);
		htimer_start(timer, cb, 0, 100);
	}

	while(1) {
		int next;
		int last = count;
		htimer_perform(&mgr);
		if (count - last > 0)
			printf("t=%llu,%d\n", mgr.time, count);
		next = htimer_next_timeout(&mgr);
		usleep(next * 1000);
	}

	return 0;
}
