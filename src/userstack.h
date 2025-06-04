#pragma once

#include <stdint.h>

#ifdef TARGET_PLAYDATE

void validate_user_stack(void);
typedef void *(*user_stack_fn)(void *);
void *call_with_user_stack_impl(user_stack_fn, void *arg, void *arg2);
#define call_with_user_stack(fn) \
    call_with_user_stack_impl((user_stack_fn)fn, NULL, NULL)
#define call_with_user_stack_1(fn, a) \
    call_with_user_stack_impl((user_stack_fn)fn, a, NULL)
#define call_with_user_stack_2(fn, a, b) \
    call_with_user_stack_impl((user_stack_fn)fn, a, b)

#else

#define call_with_user_stack(fn) (fn())
#define call_with_user_stack_1(fn, a) (fn(a))
#define call_with_user_stack_2(fn, a, b) (fn(a, b))

#endif

void init_user_stack(void);