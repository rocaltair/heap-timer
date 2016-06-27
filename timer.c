#include "heap-inl.h"
#include "timer.h"
#include <assert.h>
#include <limits.h>
#include <errno.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>

#if (defined(WIN32) || defined(_WIN32))
# include <windows.h>
# include <time.h>
#else
# include <unistd.h>
# include <sys/time.h>
#endif

#ifndef container_of
# define container_of(ptr, type, field)                            \
	  ((type*) ((char*) (ptr) - offsetof(type, field)))
#endif

#define timer__handle_start(h)                          \
  do {                                                  \
    assert(((h)->flags & TIMER_CLOSING) == 0);          \
    if (((h)->flags & TIMER_ACTIVE) != 0) break;        \
    (h)->flags |= TIMER_ACTIVE;                         \
  }                                                     \
  while (0)

#define timer__handle_stop(h)                           \
  do {                                                  \
    assert(((h)->flags & TIMER_CLOSING) == 0);          \
    if (((h)->flags & TIMER_ACTIVE) == 0) break;        \
    (h)->flags &= ~TIMER_ACTIVE;                        \
  }                                                     \
  while (0)

static int timer_less_than(const struct heap_node *ha,
			   const struct heap_node *hb)
{
	const timer_t *a;
	const timer_t *b;

	a = container_of(ha, const timer_t, heap_node);
	b = container_of(hb, const timer_t, heap_node);

	if (a->timeout < b->timeout)
		return 1;
	if (b->timeout < a->timeout)
		return 0;

	/* Compare start_id when both have the same timeout. start_id is
	 * allocated with mgr->timer_counter in timer_start().
	 */
	if (a->start_id < b->start_id)
		return 1;
	if (b->start_id < a->start_id)
		return 0;
	return 0;
}

int timer_mgr_init(timer_mgr_t *mgr)
{
	memset(mgr, 0, sizeof(*mgr));
	mgr->time = timer_get_ms_time();
	return 0;
}


int timer_init(timer_mgr_t * mgr, timer_t * handle)
{
	memset(handle, 0, sizeof(*handle));
	handle->mgr = mgr;
	handle->timer_cb = NULL;
	handle->repeat = 0;
	return 0;
}

int timer_start(timer_t * handle,
		timer_cb_t cb, uint64_t timeout, uint64_t repeat)
{
	uint64_t clamped_timeout;

	if (cb == NULL)
		return -EINVAL;

	if (timer_is_active(handle))
		timer_stop(handle);

	clamped_timeout = handle->mgr->time + timeout;
	if (clamped_timeout < timeout)
		clamped_timeout = (uint64_t) - 1;

	handle->timer_cb = cb;
	handle->timeout = clamped_timeout;
	handle->repeat = repeat;

	handle->start_id = handle->mgr->timer_counter++;

	heap_insert((struct heap *)&handle->mgr->timer_heap,
		    (struct heap_node *)&handle->heap_node, timer_less_than);
	timer__handle_start(handle);

	return 0;
}

int timer_stop(timer_t * handle)
{
	if (!timer_is_active(handle))
		return 0;

	heap_remove((struct heap *)&handle->mgr->timer_heap,
		    (struct heap_node *)&handle->heap_node, timer_less_than);
	timer__handle_stop(handle);
	return 0;
}

int timer_again(timer_t * handle)
{
	if (handle->timer_cb == NULL)
		return -EINVAL;

	if (handle->repeat) {
		timer_stop(handle);
		timer_start(handle, handle->timer_cb, handle->repeat,
			    handle->repeat);
	}

	return 0;
}

void timer_set_repeat(timer_t * handle, uint64_t repeat)
{
	handle->repeat = repeat;
}

uint64_t timer_get_repeat(const timer_t * handle)
{
	return handle->repeat;
}

int timer_next_timeout(const timer_mgr_t * mgr)
{
	const struct heap_node *heap_node;
	const timer_t *handle;
	uint64_t diff;

	heap_node = heap_min((const struct heap *)&mgr->timer_heap);
	if (heap_node == NULL)
		return -1;	/* block indefinitely */

	handle = container_of(heap_node, const timer_t, heap_node);
	if (handle->timeout <= mgr->time)
		return 0;

	diff = handle->timeout - mgr->time;
	if (diff > INT_MAX)
		diff = INT_MAX;

	return diff;
}

void timer_close(timer_t * handle)
{
	timer_stop(handle);
}

static size_t run_timers(timer_mgr_t * mgr)
{
	struct heap_node *heap_node;
	timer_t *handle;
	size_t count = 0;

	for (;;) {
		heap_node = heap_min((struct heap *)&mgr->timer_heap);
		if (heap_node == NULL)
			break;

		handle = container_of(heap_node, timer_t, heap_node);
		if (handle->timeout > mgr->time)
			break;

		timer_stop(handle);
		timer_again(handle);
		handle->timer_cb(handle);
		count++;
	}
	return count;
}

uint64_t timer_get_ms_time()
{
	uint64_t tn;
	struct timeval tv;
#if (defined(WIN32) || defined(_WIN32))
	time_t clock;
	struct tm tm;
	SYSTEMTIME wtm;

	GetLocalTime(&wtm);
	tm.tm_year = wtm.wYear - 1900;
	tm.tm_mon = wtm.wMonth - 1;
	tm.tm_mday = wtm.wDay;
	tm.tm_hour = wtm.wHour;
	tm.tm_min = wtm.wMinute;
	tm.tm_sec = wtm.wSecond;
	tm.tm_isdst = -1;
	clock = mktime(&tm);
	tv.tv_sec = clock;
	tv.tv_usec = wtm.wMilliseconds * 1000;
#else
	gettimeofday(&tv, NULL);
#endif
	tn = tv.tv_sec*1000 + tv.tv_usec / 1000;
	return tn;
}

void timer_ms_sleep(uint32_t ms)
{
#if (defined(WIN32) || defined(_WIN32))
	Sleep(ms);
#else
	usleep(ms * 1000);
#endif
}

size_t timer_perform(timer_mgr_t *mgr)
{
	mgr->time = timer_get_ms_time();
	return run_timers(mgr);
}

