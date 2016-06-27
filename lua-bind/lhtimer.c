#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <lua.h>
#include <lauxlib.h>

#include "htimer.h"

#if LUA_VERSION_NUM < 502 && (!defined(luaL_newlib))
#  define luaL_newlib(L,l) (lua_newtable(L), luaL_register(L,NULL,l))
#endif

#define TIMER_LOOP_CLS_NAME "lhtimer.mgr"
#define TIMER_TIMER_CLS_NAME "lhtimer.timer"

#define ENABLE_LTIMER_DEBUG 0
#if (defined(ENABLE_LTIMER_DEBUG) && !defined(DLOG) && ENABLE_LTIMER_DEBUG)
# define DLOG(fmt, ...) fprintf(stderr, "<lhtimer>" fmt "\n", ##__VA_ARGS__)
#else
# define DLOG(...)
#endif

struct lhtimer_s {
	htimer_t timer;
	int ref_id;
	int func;
	int mgr_ref;
};

struct lmgr_s {
	htimer_mgr_t mgr;
	lua_State *L;
};

static int luac__timer_unref(lua_State *L, int idx)
{
	int top = lua_gettop(L);
	struct lhtimer_s *s = *(struct lhtimer_s **)luaL_checkudata(L, idx, TIMER_TIMER_CLS_NAME);
	lua_getfenv(L, -1);
	luaL_checktype(L, -1, LUA_TTABLE);
	lua_rawgeti(L, -1, s->mgr_ref);
	lua_getfenv(L, -1);
	luaL_checktype(L, -1, LUA_TTABLE);
	luaL_unref(L, -1, s->ref_id);
	s->ref_id = LUA_NOREF;
	lua_settop(L, top);
	return 0;
}

static void htimer_cb(htimer_t *t)
{
	struct lhtimer_s *ts = (struct lhtimer_s *)t;
	struct lmgr_s *mgr = (struct lmgr_s *)t->mgr;
	lua_State *L = mgr->L;
	int top = lua_gettop(L);
	lua_rawgeti(L, 2, ts->ref_id);
	lua_getfenv(L, -1);
	lua_rawgeti(L, -1, ts->func);
	lua_pushvalue(L, -3);
	if (!htimer_get_repeat(t)) {
		luac__timer_unref(L, -1);
	}
	lua_pcall(L, 1, 0, 0);
	lua_settop(L, top);
}

static int lua__htimer_mgr_new(lua_State *L)
{
	struct lmgr_s *mgr = (struct lmgr_s *)malloc(sizeof(*mgr));
	if (mgr == NULL)
		goto err_mgr;
	if (htimer_mgr_init((htimer_mgr_t *)mgr))
		goto err_heap;
	*(struct lmgr_s **)lua_newuserdata(L, sizeof(void *)) = mgr;
	mgr->L = L;

	lua_newtable(L);
	lua_setfenv(L, -1);

	luaL_getmetatable(L, TIMER_LOOP_CLS_NAME);
	lua_setmetatable(L, -2);
	return 1;
err_heap:
	free(mgr);
err_mgr:
	return 0;
}

static int lua__htimer_mgr_gc(lua_State *L)
{
	htimer_mgr_t *mgr = *(htimer_mgr_t **)luaL_checkudata(L, 1, TIMER_LOOP_CLS_NAME);
	DLOG("lua__htimer_mgr_gc=%p\n", mgr);
	free(mgr);
	return 0;
}

static int lua__htimer_new(lua_State *L)
{
	struct lhtimer_s *s;
	htimer_mgr_t *mgr = *(htimer_mgr_t **)luaL_checkudata(L, 1, TIMER_LOOP_CLS_NAME);

       	s = (struct lhtimer_s *)malloc(sizeof(*s));

	*(struct lhtimer_s **)lua_newuserdata(L, sizeof(void *)) = s;

	lua_newtable(L);
	lua_pushvalue(L, 1);
	s->mgr_ref = luaL_ref(L, -2);
	lua_setfenv(L, -2);

	htimer_init(mgr, (htimer_t *)s);

	luaL_getmetatable(L, TIMER_TIMER_CLS_NAME);
	lua_setmetatable(L, -2);

	s->ref_id = LUA_NOREF;

	return 1;
}

static int lua__htimer_gc(lua_State *L)
{
	struct lhtimer_s *s = *(struct lhtimer_s **)luaL_checkudata(L, 1, TIMER_TIMER_CLS_NAME);
	DLOG("lua__htimer_gc=%p\n", s);
	free(s);
	return 0;
}

static int lua__htimer_next_timeout(lua_State *L)
{
	htimer_mgr_t *mgr = *(htimer_mgr_t **)luaL_checkudata(L, 1, TIMER_LOOP_CLS_NAME);
	lua_pushinteger(L, htimer_next_timeout(mgr));
	return 1;
}

static int lua__htimer_start(lua_State *L)
{
	struct lhtimer_s *s = *(struct lhtimer_s **)luaL_checkudata(L, 1, TIMER_TIMER_CLS_NAME);
	uint64_t interval = luaL_checkinteger(L, 2);
	uint64_t repeat = luaL_checkinteger(L, 3);
	luaL_checktype(L, 4, LUA_TFUNCTION);

	lua_getfenv(L, 1);
	luaL_checktype(L, -1, LUA_TTABLE);
	if (s->func != LUA_NOREF) {
		luaL_unref(L, -1, s->func);
	}
	lua_pushvalue(L, 4);
	s->func = luaL_ref(L, -2);

	lua_rawgeti(L, -1, s->mgr_ref);
	lua_getfenv(L, -1);
	lua_pushvalue(L, 1);

	s->ref_id = luaL_ref(L, -2);
	htimer_start((htimer_t *)s, htimer_cb, interval, repeat);
	return 0;
}

static int lua__htimer_stop(lua_State *L)
{
	struct lhtimer_s *s = *(struct lhtimer_s **)luaL_checkudata(L, 1, TIMER_TIMER_CLS_NAME);
	luac__timer_unref(L, 1);
	htimer_stop((htimer_t *)s);
	return 0;
}

static int lua__htimer_get_repeat(lua_State *L)
{
	htimer_t *s = *(htimer_t **)luaL_checkudata(L, 1, TIMER_TIMER_CLS_NAME);
	lua_pushinteger(L, htimer_get_repeat(s));
	return 1;
}

static int lua__htimer_set_repeat(lua_State *L)
{
	htimer_t *s = *(htimer_t **)luaL_checkudata(L, 1, TIMER_TIMER_CLS_NAME);
	uint64_t repeat = (uint64_t)luaL_checkinteger(L, 2);
	htimer_set_repeat(s, repeat);
	return 0;
}

static int lua__htimer_get_func(lua_State *L)
{
	struct lhtimer_s *s = *(struct lhtimer_s **)luaL_checkudata(L, 1, TIMER_TIMER_CLS_NAME);
	lua_getfenv(L, -1);
	lua_rawgeti(L, -1, s->func);
	return 1;
}

static int lua__htimer_set_func(lua_State *L)
{
	struct lhtimer_s *s = *(struct lhtimer_s **)luaL_checkudata(L, 1, TIMER_TIMER_CLS_NAME);
	luaL_checktype(L, 2, LUA_TFUNCTION);
	lua_getfenv(L, 1);
	luaL_checktype(L, -1, LUA_TTABLE);
	if (s->func != LUA_NOREF) {
		luaL_unref(L, -1, s->func);
	}
	lua_pushvalue(L, 2);
	s->func = luaL_ref(L, -2);
	return 0;
}

static int lua__htimer_perform(lua_State *L)
{
	int n = 0;
	struct lmgr_s *mgr = *(struct lmgr_s **)luaL_checkudata(L, 1, TIMER_LOOP_CLS_NAME);
	lua_settop(L, 1);
	lua_getfenv(L, 1);
	mgr->L = L;
	n = htimer_perform((htimer_mgr_t *)mgr);
	lua_pushinteger(L, n);
	return 1;
}

static int opencls__lhtimer_mgr(lua_State *L)
{
	luaL_Reg lmethods[] = {
		{"perform", lua__htimer_perform},
		{"next_timeout", lua__htimer_next_timeout},
		{NULL, NULL},
	};
	luaL_newmetatable(L, TIMER_LOOP_CLS_NAME);
	lua_newtable(L);
	luaL_register(L, NULL, lmethods);
	lua_setfield(L, -2, "__index");
	lua_pushcfunction (L, lua__htimer_mgr_gc);
	lua_setfield (L, -2, "__gc");
	return 1;
}

static int opencls__lhtimer_timer(lua_State *L)
{
	luaL_Reg lmethods[] = {
		{"start", lua__htimer_start},
		{"stop", lua__htimer_stop},
		{"get_repeat", lua__htimer_get_repeat},
		{"set_repeat", lua__htimer_set_repeat},
		{"get_func", lua__htimer_get_func},
		{"set_func", lua__htimer_set_func},
		{NULL, NULL},
	};
	luaL_newmetatable(L, TIMER_TIMER_CLS_NAME);
	lua_newtable(L);
	luaL_register(L, NULL, lmethods);
	lua_setfield(L, -2, "__index");
	lua_pushcfunction (L, lua__htimer_gc);
	lua_setfield (L, -2, "__gc");
	return 1;
}

static int lua__htimer_timenow(lua_State *L)
{
	lua_pushinteger(L, htimer_get_ms_time());
	return 1;
}

static int lua__htimer_sleep(lua_State *L)
{
	uint64_t ms = luaL_checkinteger(L, 1);
	htimer_ms_sleep(ms);
	return 0;
}

int luaopen_lhtimer(lua_State* L)
{
	luaL_Reg lfuncs[] = {
		{"new_mgr", lua__htimer_mgr_new},
		{"new_timer", lua__htimer_new},
		{"timenow", lua__htimer_timenow},
		{"sleep", lua__htimer_sleep},
		{NULL, NULL},
	};
	opencls__lhtimer_mgr(L);
	opencls__lhtimer_timer(L);
	luaL_newlib(L, lfuncs);
	return 1;
}
