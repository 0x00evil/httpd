#ifndef AP_OS_BS2000_OS_H
#define AP_OS_BS2000_OS_H
/*
 * This file in included in all Apache source code. It contains definitions
 * of facilities available on _this_ operating system (HAVE_* macros),
 * and prototypes of OS specific functions defined in os.c or os-inline.c
 */

#include "conf.h"

#if !defined(INLINE) && defined(USE_GNU_INLINE)
/* Compiler supports inline, so include the inlineable functions as
 * part of the header
 */
#define INLINE extern ap_inline
#include "os-inline.c"
#endif

#ifndef INLINE
/* Compiler does not support inline, so prototype the inlineable functions
 * as normal
 */
extern int os_is_path_absolute(const char *f);
#endif

/* Sorry if this is ugly, but the include order doesn't allow me
 * to use request_rec here... */
struct request_rec;
extern int os_checkconv(struct request_rec *r);
#endif /*AP_OS_BS2000_OS_H*/
