#ifndef _H_HMACROS_
#define _H_HMACROS_

#define likely(x) __builtin_expect((x), 1)
#define unlikely(x) __builtin_expect((x), 0)

#define ASIZE(array) (sizeof(array) / sizeof(array[0]))

#endif
