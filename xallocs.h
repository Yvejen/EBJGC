#ifndef _H_XALLOCS_
#define _H_XALLOCS_

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include "hmacros.h"
#include "log.h"

/*Xtended Allocator*/
/*Wrapper around malloc that aborts the application in case an allocation fails.
 * In most cases we can't recover from an OOM state*/

static inline void *xmalloc(size_t as) {
  void *ptr = malloc(as);
  if (unlikely(!as)) {
    log_fatal("xmalloc failed. OOM?\n");
    abort();
  }
  return ptr;
}

static inline void xfree(void *ptr) {
  /*This function is just provided because it's easier to match xmalloc's and
   * xfree's instead of xmalloc's and free's */
  free(ptr);
}

static inline void *xrealloc(void *ptr, size_t nsize) {
  void *nptr = NULL;

  if (!ptr) {
    /*Behave like xmalloc*/
    nptr = malloc(nsize);
  } else if (0 == nsize) {
    /*ptr != NULL && nsize == 0, behave like xfree*/
    free(ptr);
  } else {
    /*Do a reallocation and abort if OOM*/
    nptr = realloc(ptr, nsize);
    if (unlikely(!nptr)) {
      log_fatal("xrealloc failed. OOM?\n");
      abort();
    }
  }
  return nptr;
}

#endif
