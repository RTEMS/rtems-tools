#ifndef _CDEFS_H_
#define  _CDEFS_H_

#include <stddef.h>

/*
 * Taken from FreeBSD 9.
 */

#if defined(__cplusplus)
#define __BEGIN_DECLS   extern "C" {
#define __END_DECLS     }
#else
#define __BEGIN_DECLS
#define __END_DECLS
#endif

/*
 * Macro to test if we're using a specific version of gcc or later.
 */
#if defined(__GNUC__) && !defined(__INTEL_COMPILER)
#define __GNUC_PREREQ__(ma, mi) \
  (__GNUC__ > (ma) || __GNUC__ == (ma) && __GNUC_MINOR__ >= (mi))
#else
#define __GNUC_PREREQ__(ma, mi) 0
#endif

/*
 * We define this here since <stddef.h>, <sys/queue.h>, and <sys/types.h>
 * require it.
 */
#if __GNUC_PREREQ__(4, 1)
#define __offsetof(type, field)  __builtin_offsetof(type, field)
#else
#ifndef __cplusplus
#define __offsetof(type, field) ((size_t)(&((type *)0)->field))
#else
#define __offsetof(type, field)                                 \
  (__offsetof__ (reinterpret_cast <size_t>                      \
                 (&reinterpret_cast <const volatile char &>     \
                  (static_cast<type *> (0)->field))))
#endif
#endif
#define __rangeof(type, start, end) \
  (__offsetof(type, end) - __offsetof(type, start))

/*
 * Hack. Have to put this somewhere.
 */
typedef int gid_t;
typedef int uid_t;

/* Macros for counting and rounding. */
#ifndef howmany
#define howmany(x, y)   (((x)+((y)-1))/(y))
#endif
#define rounddown(x, y) (((x)/(y))*(y))
#define roundup(x, y)   ((((x)+((y)-1))/(y))*(y))  /* to any y */
#define roundup2(x, y)  (((x)+((y)-1))&(~((y)-1))) /* if y is powers of two */
#define powerof2(x)     ((((x)-1)&(x))==0)

#define S_ISSOCK(_m) (0)

#endif
