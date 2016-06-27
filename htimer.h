#ifndef  _HTIMER_H_257XCZ8O_
#define  _HTIMER_H_257XCZ8O_

#if defined (__cplusplus)
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>

enum timer_flag_t{
	HTIMER_ACTIVE = 1,
	HTIMER_CLOSING = 2,
	HTIMER_CLOSED = 4,
};


#define timer_is_active(h) \
  (((h)->flags & HTIMER_ACTIVE) != 0)

#define uv__is_closing(h) \
  (((h)->flags & (HTIMER_CLOSING |  HTIMER_CLOSED)) != 0)

struct htimer_mgr_s {
	struct {
		void* min;
		unsigned int nelts;
	} timer_heap;
	uint64_t timer_counter;
	uint64_t time;
};
typedef struct htimer_mgr_s htimer_mgr_t;

struct htimer_s;
typedef struct htimer_s htimer_t;

typedef void(*timer_cb_t)(htimer_t *handler);

struct htimer_s {
	void* heap_node[3];
	htimer_mgr_t *mgr;
	uint32_t flags;
	uint64_t timeout;
	uint64_t repeat;
	uint64_t start_id;
	timer_cb_t timer_cb;
};

int htimer_init(htimer_mgr_t * mgr, htimer_t * handle);
int htimer_start(htimer_t * handle,
		timer_cb_t cb, uint64_t timeout, uint64_t repeat);
int htimer_stop(htimer_t * handle);
int htimer_again(htimer_t * handle);
void htimer_set_repeat(htimer_t * handle, uint64_t repeat);
uint64_t htimer_get_repeat(const htimer_t * handle);
int htimer_next_timeout(const htimer_mgr_t * mgr);
void htimer_close(htimer_t * handle);
size_t htimer_perform(htimer_mgr_t *mgr);

uint64_t htimer_get_ms_time();
void htimer_ms_sleep(uint32_t ms);

int htimer_mgr_init(htimer_mgr_t *mgr);

#if defined (__cplusplus)
}	/*end of extern "C"*/
#endif

#endif /* end of include guard:  _HTIMER_H_257XCZ8O_ */

