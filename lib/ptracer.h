/* 
 * Copyright (c) 2024, The Pallene Developers
 * Pallene Tracer is licensed under the MIT license.
 * Please refer to the LICENSE and AUTHORS files for details
 * SPDX-License-Identifier: MIT 
 */

#ifndef PALLENE_TRACER_H
#define PALLENE_TRACER_H

#include <lua.h>
#include <lauxlib.h>

#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#if LUA_VERSION_RELEASE_NUM < 50400
#error "Pallene Tracer needs atleast Lua 5.4 to work properly"
#endif

/* ---------------- MACRO DEFINITIONS ---------------- */

#if defined(__GNUC__) || defined(__clang__) 
#define PALLENE_TRACER_UNREACHABLE    __builtin_unreachable()
#define pt_noret                      __attribute__((noreturn)) void
#elif defined(_MSC_VER) // MSVC
#define PALLENE_TRACER_UNREACHABLE    __assume(false)
#define pt_noret                      __declspec(noreturn) void
#endif

#ifdef PT_BUILD_AS_DLL
#ifdef PT_LIB
#define PT_API    __declspec(dllexport)
#else
#define PT_API    __declspec(dllimport)
#endif // PT_LIB
#else
#define PT_API    extern
#endif // PT_BUILD_AS_DLL

/* Pallene stack reference entry for the registry. */
/* DO NOT CHANGE EVEN BY MISTAKE. */
#define PALLENE_TRACER_CONTAINER_ENTRY  "__PALLENE_TRACER_CONTAINER"

/* Finalizer metatable key. */
/* DO NOT CHANGE EVEN BY MISTAKE. */
#define PALLENE_TRACER_FINALIZER_ENTRY  "__PALLENE_TRACER_FINALIZER"

/* The size of the Pallene call-stack. */
/* DO NOT CHANGE EVEN BY MISTAKE. */
#define PALLENE_TRACER_MAX_CALLSTACK         100000

/* Traceback ellipsis top threshold. How many frames should we print
   first to trigger ellipsis? */
#define PALLENE_TRACEBACK_TOP_THRESHOLD      10
/* This should always be 2 fewer than top threshold, for symmetry.
   Becuase we will always have 2 tail frames lingering around at
   at the end which is not captured by '_countlevels'. Lua also 
   do it like this. */
#define PALLENE_TRACEBACK_BOTTOM_THRESHOLD    8

/* API wrapper macros. Using these wrappers instead is raw functions
 * are highly recommended. */
#ifdef PT_DEBUG
#define PALLENE_TRACER_FRAMEENTER(L, fnstack, frame)    pallene_tracer_frameenter(L, fnstack, frame)
#define PALLENE_TRACER_SETLINE(fnstack, line)           pallene_tracer_setline(fnstack, line)
#define PALLENE_TRACER_FRAMEEXIT(fnstack)               pallene_tracer_frameexit(fnstack)

#else 
#define PALLENE_TRACER_FRAMEENTER(L, fnstack, frame) 
#define PALLENE_TRACER_SETLINE(fnstack, line) 
#define PALLENE_TRACER_FRAMEEXIT(fnstack) 
#endif // PT_DEBUG

/* Not part of the API. */
#ifdef PT_DEBUG
#define _PALLENE_TRACER_FINALIZER(L, location)           lua_pushvalue(L, (location));    \
    lua_toclose(L, -1);
#else
#define _PALLENE_TRACER_FINALIZER(L, location) 
#endif // PT_DEBUG

/* ---- DATA-STRUCTURE HELPER MACROS ---- */

/* Use this macro to fill in the details structure. */
/* E.U.: 
       pt_fn_details_t det = PALLENE_TRACER_FN_DETAILS("fn_name", "some_mod.c");
 */
#define PALLENE_TRACER_FN_DETAILS(name, fname)    \
{ .fn_name = name, .filename = fname }

/* Use this macro to fill in the frame structure as a 
   Lua interface frame. */
/* E.U.: `pt_frame_t frame = PALLENE_TRACER_LUA_FRAME(lua_fn);` */
#define PALLENE_TRACER_LUA_FRAME(fnptr)           \
{ .type = PALLENE_TRACER_FRAME_TYPE_LUA,          \
  .shared = { .c_fnptr = fnptr } }

/* Use this macro to fill in the frame structure as a
   C interface frame. */
/* E.U.: `pt_frame_t frame = PALLENE_TRACER_C_FRAME(_details);` */
#define PALLENE_TRACER_C_FRAME(detl)              \
{ .type = PALLENE_TRACER_FRAME_TYPE_C,            \
  .shared = { .details = &detl } }

/* ---- DATA-STRUCTURE HELPER MACROS END ---- */

/* ---- API HELPER MACROS ---- */

/* Use this macro the bypass some frameenter boilerplates for Lua interface frames. */
/* Note: `location` is where the finalizer object is in the stack, acquired from 
   `pallene_tracer_init()` function. If the object is passed to Lua C functions as an
   upvalue, this should be `lua_upvalueindex(n)`. Otherwise, it should just be a number 
   denoting the parameter index where the object is found if passed as a plain parameter
   to the functon. */
/* The `var_name` indicates the name of the `pt_frame_t` structure variable. */
#define PALLENE_TRACER_LUA_FRAMEENTER(L, fnstack, fnptr, location, var_name)    \
pt_frame_t var_name = PALLENE_TRACER_LUA_FRAME(fnptr);                          \
PALLENE_TRACER_FRAMEENTER(L, fnstack, &var_name);                               \
_PALLENE_TRACER_FINALIZER(L, location)

/* Use this macro the bypass some frameenter boilerplates for C interface frames. */
/* The `var_name` indicates the name of the `pt_frame_t` structure variable. */
#define PALLENE_TRACER_C_FRAMEENTER(L, fnstack, fn_name, filename, var_name)    \
pt_fn_details_t var_name##_details =                                            \ 
    PALLENE_TRACER_FN_DETAILS(fn_name, filename);                               \
pt_frame_t var_name = PALLENE_TRACER_C_FRAME(var_name##_details);               \
PALLENE_TRACER_FRAMEENTER(L, fnstack, &var_name);

/* -- GENERIC MACROS -- */

/* FOR NORMAL C MODULES THESE MACROS SHOULD SUFFICE.  */
#define PALLENE_TRACER_GENERIC_C_FRAMEENTER(L, fnstack, var_name)               \
    PALLENE_TRACER_C_FRAMEENTER(L, fnstack, __func__, __FILE__, var_name)

#define PALLENE_TRACER_GENERIC_C_SETLINE(fnstack)                               \
    PALLENE_TRACER_SETLINE(fnstack, __LINE__ + 1)

/* ---- API HELPER MACROS END ---- */

/* ---------------- MACRO DEFINITIONS END ---------------- */

/* ---------------- DATA STRUCTURES ---------------- */

/* What type of frame we are dealing with? Is it just a normal
   C function or Lua C Function? */
typedef enum frame_type {
    PALLENE_TRACER_FRAME_TYPE_C,
    PALLENE_TRACER_FRAME_TYPE_LUA
} frame_type_t;

/* Details of the callee function (name, where it is from etc.) */
/* Optimization Tip: Try declaring the struct 'static'. */
typedef struct pt_fn_details {
    const char *const fn_name;
    const char *const filename;
} pt_fn_details_t;

/* A single frame representation. */
typedef struct pt_frame {
    frame_type_t type;
    int line;

    union {
        pt_fn_details_t *details;
        lua_CFunction c_fnptr;
    } shared;
} pt_frame_t;

/* Our stack is fully heap-allocated stack. We need some structure to hold
   the stack information. This structure will be an Userdatum. */
typedef struct pt_fnstack {
    pt_frame_t *stack;
    int count;
} pt_fnstack_t;

/* ---------------- DATA STRUCTURES END ---------------- */

/* ---------------- DECLARATIONS ---------------- */

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

/* Initializes the Pallene Tracer. The initialization refers to creating the stack
   if not created, preparing the traceback fn and finalizers. */
/* This function must only be called from Lua module entry point. */
/* NOTE: Pushes the finalizer object to the stack. The object has to be closed
   everytime you are in a Lua C function using `lua_toclose(L, idx)`. */
PT_API pt_fnstack_t *pallene_tracer_init(lua_State *L);

/* Pushes a frame to the stack. The frame structure is self-managed for every function. */
PT_API void pallene_tracer_frameenter(lua_State *L, pt_fnstack_t *fnstack, pt_frame_t *restrict frame);

/* Sets line number to the topmost frame in the stack. */
PT_API void pallene_tracer_setline(pt_fnstack_t *fnstack, int line);

/* Removes the last frame from the stack. */
PT_API void pallene_tracer_frameexit(pt_fnstack_t *fnstack);

/* Pallene Tracer explicit traceback function to show Pallene call-stack
   tracebacks. */
PT_API int  pallene_tracer_debug_traceback(lua_State *L);

/* When we encounter a runtime error, `pallene_tracer_frameexit()` may not
   get called. Therefore, the stack will get corrupted if the previous
   call-frames are not removed. The finalizer function makes sure it
   does not happen. Its guardian angel. */
/* The finalizer function will be called from a to-be-closed value (since
   Lua 5.4). If you are using Lua version prior 5.4, you are outta luck. */
PT_API int pallene_tracer_finalizer(lua_State *L);

/* Runtime error to invoke when Pallene call-stack overflowed. */
pt_noret pallene_tracer_runtime_callstack_overflow_error(lua_State *L);

#ifdef __cplusplus
}
#endif // __cplusplus

/* ---------------- DECLARATIONS END ---------------- */

#endif // PALLENE_TRACER_H

#if defined(PT_IMPLEMENTATION) && !defined(PT_IMPLEMENTED)
/* This is implementation guard, making sure we include the implementation just one time. */
#define PT_IMPLEMENTED

/* ---------------- PRIVATE ---------------- */

/* Global table name deduction. Can we find a function name? */
static bool _pallene_tracer_findfield(lua_State *L, int fn_idx, int level) {
    if(level == 0 || !lua_istable(L, -1))
        return false;

    lua_pushnil(L);    /* Initial key. */

    while(lua_next(L, -2)) {
        /* We are only interested in String keys. */
        if(lua_type(L, -2) == LUA_TSTRING) {
            /* Avoid "_G" recursion in global table. The global table is also part of
               global table :). */
            if(!strcmp(lua_tostring(L, -2), "_G")) {
                /* Remove value and continue. */
                lua_pop(L, 1);
                continue;
            }

            /* Is it the function we are looking for? */
            if(lua_rawequal(L, fn_idx, -1)) {
                /* Remove value and keep name. */
                lua_pop(L, 1);
                return true;
            }
            /* If not go one level deeper and get the value recursively. */
            else if(_pallene_tracer_findfield(L, fn_idx, level - 1)) {
                /* Remove the table but keep name. */
                lua_remove(L, -2);

                /* Add a "." in between. */
                lua_pushliteral(L, ".");
                lua_insert(L, -2);

                /* Concatenate last 3 values, resulting "table.some_func". */
                lua_concat(L, 3);

                return true;
            }
        }

        /* Pop the value. */
        lua_pop(L, 1);
    }

    return false;
}

/* Pushes a function name if found in the global table and returns true.
   Returns false otherwise. */
/* Expects the function to be pushed in the stack. */
static bool _pallene_tracer_pgf_name(lua_State *L) {
    int top = lua_gettop(L);

    /* Start from the global table. */
    lua_pushglobaltable(L);

    if(_pallene_tracer_findfield(L, top, 2)) {
        lua_remove(L, -2);
        return true;
    }

    lua_pop(L, 1);
    return false;
}

/* Returns the maximum number of levels in Lua stack. */
static int _pallene_tracer_countlevels(lua_State *L) {
    lua_Debug ar;
    int li = 1, le = 1;

    /* Find an upper bound */
    while (lua_getstack(L, le, &ar)) {
        li = le, le *= 2;
    }

    /* Do a binary search */
    while (li < le) {
        int m = (li + le) / 2;

        if (lua_getstack(L, m, &ar)) li = m + 1;
        else le = m;
    }

    return le - 1;
}

/* Counts the number of white and black frames in the Pallene call stack. */
static void _pallene_tracer_countframes(pt_fnstack_t *fnstack, int *mwhite, int *mblack) {
    *mwhite = *mblack = 0;

    for(int i = 0; i < fnstack->count; i++) {
        *mwhite += (fnstack->stack[i].type == PALLENE_TRACER_FRAME_TYPE_C);
        *mblack += (fnstack->stack[i].type == PALLENE_TRACER_FRAME_TYPE_LUA);
    }
}

/* Responsible for printing and controlling some of the traceback fn parameters. */
static void _pallene_tracer_dbg_print(const char *buf, bool *ellipsis, int *pframes, int nframes) {
    /* We have printed the frame, even tho it might not be visible ;). */
    (*pframes)++;

    /* Should we print? Are we at any point in top or bottom printing threshold? */
    bool should_print = (*pframes <= PALLENE_TRACEBACK_TOP_THRESHOLD)
        || ((nframes - *pframes) <= PALLENE_TRACEBACK_BOTTOM_THRESHOLD);

    if(luai_likely(should_print))
        fprintf(stderr, buf);
    else if(*ellipsis) {
        fprintf(stderr, "\n    ... (Skipped %d frames) ...\n\n",
            nframes - (PALLENE_TRACEBACK_TOP_THRESHOLD
            + PALLENE_TRACEBACK_BOTTOM_THRESHOLD));

        *ellipsis = false;
    }
}

/* Frees the heap-allocated resources. */
/* This function will be used as `__gc` metamethod to free our stack. */
static int _pallene_tracer_free_resources(lua_State *L) {
    pt_fnstack_t *fnstack = (pt_fnstack_t *) lua_touserdata(L, 1);
    free(fnstack->stack);

    return 0;
}

/* ---------------- PRIVATE END ---------------- */

/* ---------------- DEFINITIONS ---------------- */

/* Initializes the Pallene Tracer. The initialization refers to creating the stack
   if not created, preparing the traceback fn and finalizers. */
/* This function must only be called from Lua module entry point. */
/* NOTE: Pushes the finalizer object to the stack. The object has to be closed
   everytime you are in a Lua C function using `lua_toclose(L, idx)`. */
/* ALSO NOTE: The stack and finalizer object would be returned if and only if `PT_DEBUG`
   is set. Otherwise, a NULL pointer would be returned alongside a NIL value pushed onto the stack. */
pt_fnstack_t *pallene_tracer_init(lua_State *L) {
#ifdef PT_DEBUG
    pt_fnstack_t *fnstack = NULL;

    /* Try getting the userdata. */
    lua_getfield(L, LUA_REGISTRYINDEX, PALLENE_TRACER_CONTAINER_ENTRY);

    /* If we don't find any userdata, initialize resources. */
    if(luai_unlikely(lua_isnil(L, -1) == 1)) {
        fnstack = (pt_fnstack_t *) lua_newuserdata(L, sizeof(pt_fnstack_t));
        fnstack->stack = malloc(PALLENE_TRACER_MAX_CALLSTACK * sizeof(pt_frame_t));
        fnstack->count = 0;

        /* Prepare the `__gc` finalizer to free the stack. */
        lua_newtable(L);
        lua_pushcfunction(L, _pallene_tracer_free_resources);
        lua_setfield(L, -2, "__gc");
        lua_setmetatable(L, -2);

        /* This is our finalizer which will reside in the value stack. */
        lua_newtable(L);
        lua_newtable(L);
        lua_pushvalue(L, -3);

        /* Our finalizer fn. */
        lua_pushcclosure(L, pallene_tracer_finalizer, 1);
        lua_setfield(L, -2, "__close");
        lua_setmetatable(L, -2);

        /* Set finalizer object to registry. */
        lua_setfield(L, LUA_REGISTRYINDEX, PALLENE_TRACER_FINALIZER_ENTRY);

        /* Set stack function stack container to registry .*/
        lua_setfield(L, LUA_REGISTRYINDEX, PALLENE_TRACER_CONTAINER_ENTRY);

        /* The debug traceback fn. */
        lua_register(L, "pallene_tracer_debug_traceback", pallene_tracer_debug_traceback);

        /* Push the finalizer object in the stack. */
        lua_getfield(L, LUA_REGISTRYINDEX, PALLENE_TRACER_FINALIZER_ENTRY);
    } else {
        fnstack = lua_touserdata(L, -1);
        lua_getfield(L, LUA_REGISTRYINDEX, PALLENE_TRACER_FINALIZER_ENTRY);
    }

    return fnstack;
#else
    /* No debug mode, no stack and finalizer object. Regardless we need to fill in the blanks. */
    lua_pushnil(L);
    return NULL;
#endif // PT_DEBUG
}

/* Pushes a frame to the stack. The frame structure is self-managed for every function. */
void pallene_tracer_frameenter(lua_State *L, pt_fnstack_t *fnstack, pt_frame_t *restrict frame) {
    /* Have we ran out of stack entries? */
    if(luai_unlikely(fnstack->count + 1 >= PALLENE_TRACER_MAX_CALLSTACK)) {
        pallene_tracer_runtime_callstack_overflow_error(L);
    }

    fnstack->stack[fnstack->count++] = *frame;
}

/* Sets line number to the topmost frame in the stack. */
void pallene_tracer_setline(pt_fnstack_t *fnstack, int line) {
    if(luai_likely(fnstack->count != 0))
        fnstack->stack[fnstack->count - 1].line = line;
}

/* Removes the last frame from the stack. */
void pallene_tracer_frameexit(pt_fnstack_t *fnstack) {
    fnstack->count -= (fnstack->count > 0);
}

/* Helper macro specific to this function only :). */
#define DBG_PRINT() _pallene_tracer_dbg_print(buf, &ellipsis, &pframes, nframes)
/* Pallene Tracer explicit traceback function to show Pallene call-stack
   tracebacks. */
int pallene_tracer_debug_traceback(lua_State *L) {
    lua_getfield(L, LUA_REGISTRYINDEX, PALLENE_TRACER_CONTAINER_ENTRY);
    pt_fnstack_t *fnstack = (pt_fnstack_t *) lua_touserdata(L, -1);
    pt_frame_t *stack = fnstack->stack;
    /* The point where we are in the Pallene stack. */
    int index = fnstack->count - 1;
    lua_pop(L, 1);

    /* Max number of white and black frames. */
    int mwhite, mblack;
    _pallene_tracer_countframes(fnstack, &mwhite, &mblack);
    /* Max levels of Lua stack. */
    int mlevel = _pallene_tracer_countlevels(L);

    /* Total frames we are going to print. */
    /* Black frames are used for switching and we will start from
       Lua stack level 1. */
    int nframes = mlevel + mwhite - mblack - 1;
    /* Amount of frames printed. */
    int pframes = 0;
    /* Should we print ellipsis? */
    bool ellipsis = nframes > (PALLENE_TRACEBACK_TOP_THRESHOLD
        + PALLENE_TRACEBACK_BOTTOM_THRESHOLD);

    /* Buffer to store for a single frame line to be printed. */
    int _BUFSIZ = 1023;
    char buf[_BUFSIZ + 1];

    const char *message = lua_tostring(L, 1);
    fprintf(stderr, "Runtime error: %s\nStack traceback:\n", message);

    lua_Debug ar;
    int top = lua_gettop(L);
    int level = 1;

    while(lua_getstack(L, level++, &ar)) {
        /* Get information regarding the frame: name, source, linenumbers etc. */
        lua_getinfo(L, "Slnf", &ar);

        /* If the frame is a C frame. */
        if(lua_iscfunction(L, -1)) {
            if(index >= 0) {
                /* Check whether this frame is tracked (C interface frames). */
                int check = index;
                while(stack[check].type != PALLENE_TRACER_FRAME_TYPE_LUA)
                    check--;

                /* If the frame matches, we switch to printing Pallene frames. */
                if(lua_tocfunction(L, -1) == stack[check].shared.c_fnptr) {
                    /* Now print all the frames in Pallene stack. */
                    for(; index > check; index--) {
                        snprintf(buf, _BUFSIZ, "    %s:%d: in function '%s'\n",
                            stack[index].shared.details->filename,
                            stack[index].line, stack[index].shared.details->fn_name);
                        DBG_PRINT();
                    }

                    /* 'check' idx is guaranteed to be a Lua interface frame.
                       Which is basically our 'stack' index at this point. So,
                       we simply ignore the Lua interface frame. */
                    index--;

                    /* We are done. */
                    lua_settop(L, top);
                    continue;
                }
            }

            /* Then it's an untracked C frame. */
            if(_pallene_tracer_pgf_name(L))
                lua_pushfstring(L, "%s", lua_tostring(L, -1));
            else lua_pushliteral(L, "<?>");

            snprintf(buf, _BUFSIZ, "    C: in function '%s'\n", lua_tostring(L, -1));
            DBG_PRINT();
        } else {
            /* It's a Lua frame. */

            /* Do we have a name? */
            if(*ar.namewhat != '\0')
                lua_pushfstring(L, "function '%s'", ar.name);
            /* Is it the main chunk? */
            else if(*ar.what == 'm')
                lua_pushliteral(L, "<main>");
            /* Can we deduce the name from the global table? */
            else if(_pallene_tracer_pgf_name(L))
                lua_pushfstring(L, "function '%s'", lua_tostring(L, -1));
            else lua_pushliteral(L, "function '<?>'");

            snprintf(buf, _BUFSIZ, "    %s:%d: in %s\n", ar.short_src,
                ar.currentline, lua_tostring(L, -1));
            DBG_PRINT();
        }

        lua_settop(L, top);
    }

    return 0;
}
#undef DBG_PRINT

/* When we encounter a runtime error, `pallene_tracer_frameexit()` may not
   get called. Therefore, the stack will get corrupted if the previous
   call-frames are not removed. The finalizer function makes sure it
   does not happen. Its guardian angel. */
/* The finalizer function will be called from a to-be-closed value (since
   Lua 5.4). If you are using Lua version prior 5.4, you are outta luck. */
int pallene_tracer_finalizer(lua_State *L) {
    /* Get the userdata. */
    pt_fnstack_t *fnstack = (pt_fnstack_t *) lua_touserdata(L, lua_upvalueindex(1));

    /* Remove all the frames until last Lua frame. */
    int idx = fnstack->count - 1;
    while(fnstack->stack[idx].type != PALLENE_TRACER_FRAME_TYPE_LUA)
        idx--;

    /* Remove the Lua frame as well. */
    fnstack->count = idx;

    return 0;
}

/* Runtime error to invoke when Pallene call-stack overflowed. */
pt_noret pallene_tracer_runtime_callstack_overflow_error(lua_State *L) {
    luaL_error(L, "pallene callstack overflow");
    PALLENE_TRACER_UNREACHABLE;
}

/* ---------------- DEFINITIONS END ---------------- */

#endif
