#include "timer.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>

int count = 0;

void cb(timer_t *handle)
{
	count++;
}

int main(int argc, char **argv)
{
	int i;
	timer_mgr_t mgr;
	timer_mgr_init(&mgr);

	for (i = 0; i < 10000; i++) {
		timer_t *timer = malloc(sizeof(*timer));
		timer_init(&mgr, timer);
		timer_start(timer, cb, 0, 100);
	}

	while(1) {
		int next;
		int last = count;
		timer_perform(&mgr);
		if (count - last > 0)
			printf("t=%llu,%d\n", mgr.time, count);
		next = timer_next_timeout(&mgr);
		usleep(next * 1000);
	}

	return 0;
}
