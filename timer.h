#ifndef  _TIMER_H_257XCZ8O_
#define  _TIMER_H_257XCZ8O_

#if defined (__cplusplus)
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>

enum timer_flag_t{
	TIMER_ACTIVE = 1,
	TIMER_CLOSING = 2,
	TIMER_CLOSED = 4,
};


#define timer_is_active(h) \
  (((h)->flags & TIMER_ACTIVE) != 0)

#define uv__is_closing(h) \
  (((h)->flags & (TIMER_CLOSING |  TIMER_CLOSED)) != 0)

struct timer_mgr_s {
	struct {
		void* min;
		unsigned int nelts;
	} timer_heap;
	uint64_t timer_counter;
	uint64_t time;
};
typedef struct timer_mgr_s timer_mgr_t;

struct timer_s;
typedef struct timer_s timer_t;

typedef void(*timer_cb_t)(timer_t *handler);

struct timer_s {
	void* heap_node[3];
	timer_mgr_t *mgr;
	uint32_t flags;
	uint64_t timeout;
	uint64_t repeat;
	uint64_t start_id;
	timer_cb_t timer_cb;
};

int timer_init(timer_mgr_t * mgr, timer_t * handle);
int timer_start(timer_t * handle,
		timer_cb_t cb, uint64_t timeout, uint64_t repeat);
int timer_stop(timer_t * handle);
int timer_again(timer_t * handle);
void timer_set_repeat(timer_t * handle, uint64_t repeat);
uint64_t timer_get_repeat(const timer_t * handle);
int timer_next_timeout(const timer_mgr_t * mgr);
void timer_close(timer_t * handle);
size_t timer_perform(timer_mgr_t *mgr);

uint64_t timer_get_ms_time();
void timer_ms_sleep(uint32_t ms);

int timer_mgr_init(timer_mgr_t *mgr);

#if defined (__cplusplus)
}	/*end of extern "C"*/
#endif

#endif /* end of include guard:  _TIMER_H_257XCZ8O_ */

