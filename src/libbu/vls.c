/*                           V L S . C
 * BRL-CAD
 *
 * Copyright (c) 2004-2012 United States Government as represented by
 * the U.S. Army Research Laboratory.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * version 2.1 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this file; see the file named COPYING for more
 * information.
 */

#include "common.h"

#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>

#ifdef HAVE_STDINT_H
#   include <stdint.h>
#endif

#include "bio.h"

#include "bu.h"


/* non-published globals */
extern const char bu_vls_message[];
extern const char bu_strdup_message[];

/* private constants */

/* minimum initial allocation size */
static const unsigned int _VLS_ALLOC_MIN = 32;

/* minimum vls allocation increment size */
static const size_t _VLS_ALLOC_STEP = 128;

/* minimum vls buffer allocation size */
static const unsigned int _VLS_ALLOC_READ = 4096;


void
bu_vls_init(struct bu_vls *vp)
{
    if (UNLIKELY(vp == (struct bu_vls *)NULL))
	bu_bomb("bu_vls_init() passed NULL pointer");

#if defined(DEBUG) && 0
    /* if already a vls, sanity check for a potential memory leak. */
    if (vp->vls_magic == BU_VLS_MAGIC) {
	if (vp->vls_str && vp->vls_len > 0 && vp->vls_max > 0) {
	    bu_log("bu_vls_init potential leak [%s] (vls_len=%d)\n", vp->vls_str, vp->vls_len);
	}
    }
#endif

    vp->vls_magic = BU_VLS_MAGIC;
    vp->vls_str = (char *)0;
    vp->vls_len = vp->vls_max = vp->vls_offset = 0;
}


void
bu_vls_init_if_uninit(struct bu_vls *vp)
{
    bu_log("DEPRECATION WARNING: bu_vls_init_if_uninit() should no longer be called.\n\t\tUse bu_vls_init() instead.\n");

    if (UNLIKELY(vp == (struct bu_vls *)NULL))
	bu_bomb("bu_vls_init_if_uninit() passed NULL pointer");

    if (vp->vls_magic == BU_VLS_MAGIC)
	return;
    bu_vls_init(vp);
}


struct bu_vls *
bu_vls_vlsinit(void)
{
    struct bu_vls *vp;

    vp = (struct bu_vls *)bu_malloc(sizeof(struct bu_vls), "bu_vls_vlsinit struct");
    bu_vls_init(vp);

    return vp;
}


char *
bu_vls_addr(const struct bu_vls *vp)
{
    static char nullbuf[4] = {0, 0, 0, 0};
    BU_CK_VLS(vp);

    if (vp->vls_max == 0 || vp->vls_str == (char *)NULL) {
	/* A zero-length VLS is a null string */
	nullbuf[0] = '\0'; /* sanity */
	return nullbuf;
    }

    /* Sanity checking */
    if (vp->vls_str == (char *)NULL ||
	vp->vls_len + vp->vls_offset >= vp->vls_max)
    {
	bu_log("bu_vls_addr: bad VLS.  max=%zu, len=%zu, offset=%zu\n",
	       vp->vls_max, vp->vls_len, vp->vls_offset);
	bu_bomb("bu_vls_addr\n");
    }

    return vp->vls_str+vp->vls_offset;
}


void
bu_vls_extend(struct bu_vls *vp, unsigned int extra)
{
    BU_CK_VLS(vp);

    /* allocate at least 32 bytes (8 or 4 words) extra */
    if (extra < _VLS_ALLOC_MIN)
	extra = _VLS_ALLOC_MIN;

    /* first time allocation.
     *
     * performance testing using a static buffer indicated an
     * approximate 25% gain by avoiding this first allocation but not
     * worth the complexity involved (e.g., extending the struct or
     * hijacking vls_str) and it'd be error-prone whenever the vls
     * implementation changes.
     */
    if (vp->vls_max <= 0 || vp->vls_str == (char *)0) {
	vp->vls_str = (char *)bu_malloc((size_t)extra, bu_vls_message);
	vp->vls_max = extra;
	vp->vls_len = 0;
	vp->vls_offset = 0;
	*vp->vls_str = '\0';
	return;
    }

    /* make sure to increase in step sized increments */
    if ((size_t)extra < _VLS_ALLOC_STEP)
	extra = (unsigned int)_VLS_ALLOC_STEP;
    else if ((size_t)extra % _VLS_ALLOC_STEP != 0)
	extra += (unsigned int)(_VLS_ALLOC_STEP - (extra % _VLS_ALLOC_STEP));

    /* need more space? */
    if (vp->vls_offset + vp->vls_len + extra >= vp->vls_max) {
	vp->vls_str = (char *)bu_realloc(vp->vls_str, (vp->vls_max + extra), bu_vls_message);
	vp->vls_max += extra;
    }
}


void
bu_vls_setlen(struct bu_vls *vp, size_t newlen)
{
    BU_CK_VLS(vp);

    if (vp->vls_len >= newlen)
	return;

    bu_vls_extend(vp, (unsigned)newlen - vp->vls_len);
    vp->vls_len = newlen;
}


size_t
bu_vls_strlen(const struct bu_vls *vp)
{
    BU_CK_VLS(vp);

    return vp->vls_len;
}


void
bu_vls_trunc(struct bu_vls *vp, int len)
{
    BU_CK_VLS(vp);

    if (len < 0) {
	/* now an absolute length */
	len = vp->vls_len + len;
    }
    if (vp->vls_len <= (size_t)len)
	return;
    if (len == 0)
	vp->vls_offset = 0;

    vp->vls_str[len+vp->vls_offset] = '\0'; /* force null termination */
    vp->vls_len = len;
}


void
bu_vls_trunc2(struct bu_vls *vp, int len)
{
    BU_CK_VLS(vp);

    if (vp->vls_len <= (size_t)len)
	return;

    if (len < 0)
	len = 0;
    if (len == 0)
	vp->vls_offset = 0;

    vp->vls_str[len+vp->vls_offset] = '\0'; /* force null termination */
    vp->vls_len = len;
}


void
bu_vls_nibble(struct bu_vls *vp, int len)
{
    BU_CK_VLS(vp);

    if (len < 0 && (-len) > (ssize_t)vp->vls_offset)
	len = -vp->vls_offset;
    if ((size_t)len >= vp->vls_len) {
	bu_vls_trunc(vp, 0);
	return;
    }

    vp->vls_len -= len;
    vp->vls_offset += len;
}


void
bu_vls_free(struct bu_vls *vp)
{
    BU_CK_VLS(vp);

    if (vp->vls_str && vp->vls_max > 0) {
	vp->vls_str[0] = '?'; /* Sanity */
	bu_free(vp->vls_str, "bu_vls_free");
	vp->vls_str = (char *)0;
    }

    vp->vls_offset = vp->vls_len = vp->vls_max = 0;
}


void
bu_vls_vlsfree(struct bu_vls *vp)
{
    if (UNLIKELY(*(unsigned long *)vp != BU_VLS_MAGIC))
	return;

    bu_vls_free(vp);
    bu_free(vp, "bu_vls_vlsfree");
}


char *
bu_vls_strdup(const struct bu_vls *vp)
{
    char *str;
    size_t len;

    BU_CK_VLS(vp);

    len = bu_vls_strlen(vp);
    str = bu_malloc(len+1, bu_strdup_message);
    bu_strlcpy(str, bu_vls_addr(vp), len+1);

    return str;
}


char *
bu_vls_strgrab(struct bu_vls *vp)
{
    char *str;

    BU_CK_VLS(vp);

    if (vp->vls_offset != 0 || !vp->vls_str) {
	str = bu_vls_strdup(vp);
	bu_vls_free(vp);
	return str;
    }

    str = bu_vls_addr(vp);
    vp->vls_str = (char *)0;
    vp->vls_offset = vp->vls_len = vp->vls_max = 0;
    return str;
}


void
bu_vls_strcpy(struct bu_vls *vp, const char *s)
{
    size_t len;

    BU_CK_VLS(vp);

    if (UNLIKELY(s == (const char *)NULL))
	return;

    len = strlen(s);
    if (UNLIKELY(len <= 0)) {
	vp->vls_len = 0;
	vp->vls_offset = 0;
	if (vp->vls_max > 0)
	    vp->vls_str[0] = '\0';
	return;
    }

    /* cancel offset before extending */
    vp->vls_offset = 0;
    if (len+1 >= vp->vls_max)
	bu_vls_extend(vp, (unsigned int)len+1);

    memcpy(vp->vls_str, s, len+1); /* include null */
    vp->vls_len = len;
}


void
bu_vls_strncpy(struct bu_vls *vp, const char *s, size_t n)
{
    size_t len;

    BU_CK_VLS(vp);

    if (UNLIKELY(s == (const char *)NULL))
	return;

    len = strlen(s);
    if (len > n)
	len = n;
    if (UNLIKELY(len <= 0)) {
	vp->vls_len = 0; /* ensure string is empty */
	return;
    }

    /* cancel offset before extending */
    vp->vls_offset = 0;
    if (len+1 >= vp->vls_max)
	bu_vls_extend(vp, (unsigned int)len+1);

    memcpy(vp->vls_str, s, len);
    vp->vls_str[len] = '\0'; /* force null termination */
    vp->vls_len = (int)len;
}


void
bu_vls_strcat(struct bu_vls *vp, const char *s)
{
    size_t len;

    BU_CK_VLS(vp);

    if (UNLIKELY(s == (const char *)NULL))
	return;

    len = strlen(s);
    if (UNLIKELY(len <= 0))
	return;

    if (vp->vls_offset + vp->vls_len + len+1 >= vp->vls_max)
	bu_vls_extend(vp, (unsigned int)len+1);

    memcpy(vp->vls_str +vp->vls_offset + vp->vls_len, s, len+1); /* include null */
    vp->vls_len += (int)len;
}


void
bu_vls_strncat(struct bu_vls *vp, const char *s, size_t n)
{
    size_t len;

    BU_CK_VLS(vp);

    if (UNLIKELY(s == (const char *)NULL))
	return;

    len = strlen(s);
    if (len > n)
	len = n;
    if (UNLIKELY(len <= 0))
	return;

    if (vp->vls_offset + vp->vls_len + len+1 >= vp->vls_max)
	bu_vls_extend(vp, (unsigned int)len+1);

    memcpy(vp->vls_str + vp->vls_offset + vp->vls_len, s, len);
    vp->vls_len += (int)len;
    vp->vls_str[vp->vls_offset + vp->vls_len] = '\0'; /* force null termination */
}


void
bu_vls_vlscat(struct bu_vls *dest, const struct bu_vls *src)
{
    BU_CK_VLS(src);
    BU_CK_VLS(dest);

    if (UNLIKELY(src->vls_len <= 0))
	return;

    if (dest->vls_offset + dest->vls_len + src->vls_len+1 >= dest->vls_max)
	bu_vls_extend(dest, (unsigned int)src->vls_len+1);

    /* copy source string, including null */
    memcpy(dest->vls_str +dest->vls_offset + dest->vls_len, src->vls_str+src->vls_offset, src->vls_len+1);
    dest->vls_len += src->vls_len;
}


void
bu_vls_vlscatzap(struct bu_vls *dest, struct bu_vls *src)
{
    BU_CK_VLS(src);
    BU_CK_VLS(dest);

    if (UNLIKELY(src->vls_len <= 0))
	return;

    bu_vls_vlscat(dest, src);
    bu_vls_trunc(src, 0);
}


int
bu_vls_strcmp(struct bu_vls *s1, struct bu_vls *s2)
{
    BU_CK_VLS(s1);
    BU_CK_VLS(s2);

    /* A zero-length VLS is a null string, account for it */
    if (s1->vls_max == 0 || s1->vls_str == (char *)NULL) {
	/* s1 is empty */
	/* ensure first-time allocation for null-termination */
	bu_vls_extend(s1, 1);
    }
    if (s2->vls_max == 0 || s2->vls_str == (char *)NULL) {
	/* s2 is empty */
	/* ensure first-time allocation for null-termination */
	bu_vls_extend(s2, 1);
    }

    /* neither empty, straight up comparison */
    return bu_strcmp(s1->vls_str+s1->vls_offset, s2->vls_str+s2->vls_offset);
}


int
bu_vls_strncmp(struct bu_vls *s1, struct bu_vls *s2, size_t n)
{
    BU_CK_VLS(s1);
    BU_CK_VLS(s2);

    if (UNLIKELY(n <= 0)) {
	/* they match at zero chars */
	return 0;
    }

    /* A zero-length VLS is a null string, account for it */
    if (s1->vls_max == 0 || s1->vls_str == (char *)NULL) {
	/* s1 is empty */
	/* ensure first-time allocation for null-termination */
	bu_vls_extend(s1, 1);
    }
    if (s2->vls_max == 0 || s2->vls_str == (char *)NULL) {
	/* s2 is empty */
	/* ensure first-time allocation for null-termination */
	bu_vls_extend(s2, 1);
    }

    return bu_strncmp(s1->vls_str+s1->vls_offset, s2->vls_str+s2->vls_offset, n);
}


void
bu_vls_from_argv(struct bu_vls *vp, int argc, const char *argv[])
{
    BU_CK_VLS(vp);

    for (/* nada */; argc > 0; argc--, argv++) {
	bu_vls_strcat(vp, *argv);
	if (argc > 1)  bu_vls_strcat(vp, " ");
    }
}


void
bu_vls_fwrite(FILE *fp, const struct bu_vls *vp)
{
    size_t status;

    BU_CK_VLS(vp);

    if (UNLIKELY(vp->vls_len <= 0))
	return;

    bu_semaphore_acquire(BU_SEM_SYSCALL);
    status = fwrite(vp->vls_str + vp->vls_offset, vp->vls_len, 1, fp);
    bu_semaphore_release(BU_SEM_SYSCALL);

    if (UNLIKELY(status != 1)) {
	perror("fwrite");
	bu_bomb("bu_vls_fwrite() write error\n");
    }
}


void
bu_vls_write(int fd, const struct bu_vls *vp)
{
    ssize_t status;

    BU_CK_VLS(vp);

    if (UNLIKELY(vp->vls_len <= 0))
	return;

    bu_semaphore_acquire(BU_SEM_SYSCALL);
    status = write(fd, vp->vls_str + vp->vls_offset, vp->vls_len);
    bu_semaphore_release(BU_SEM_SYSCALL);

    if (UNLIKELY(status < 0 || (size_t)status != vp->vls_len)) {
	perror("write");
	bu_bomb("bu_vls_write() write error\n");
    }
}


int
bu_vls_read(struct bu_vls *vp, int fd)
{
    size_t todo;
    int got;
    int ret = 0;

    BU_CK_VLS(vp);

    for (;;) {
	bu_vls_extend(vp, _VLS_ALLOC_READ);
	todo = vp->vls_max - vp->vls_len - vp->vls_offset - 1;

	bu_semaphore_acquire(BU_SEM_SYSCALL);
	got = (int)read(fd, vp->vls_str+vp->vls_offset+vp->vls_len, todo);
	bu_semaphore_release(BU_SEM_SYSCALL);

	if (UNLIKELY(got < 0)) {
	    /* Read error, abandon the read */
	    return -1;
	}
	if (got == 0)
	    break;

	vp->vls_len += got;
	ret += got;
    }

    /* force null termination */
    vp->vls_str[vp->vls_len+vp->vls_offset] = '\0';

    return ret;
}


int
bu_vls_gets(struct bu_vls *vp, FILE *fp)
{
    int startlen;
    int endlen;
    int done;
    char *bufp;
    char buffer[BUFSIZ+1] = {0};

    BU_CK_VLS(vp);

    startlen = bu_vls_strlen(vp);

    do {
	bufp = bu_fgets(buffer, BUFSIZ+1, fp);

	if (!bufp)
	    return -1;

	/* keep reading if we just filled the buffer */
	if ((strlen(bufp) == BUFSIZ) && (bufp[BUFSIZ-1] != '\n') && (bufp[BUFSIZ-1] != '\r')) {
	    done = 0;
	} else {
	    done = 1;
	}

	/* strip the trailing EOL (or at least part of it) */
	if ((bufp[strlen(bufp)-1] == '\n') || (bufp[strlen(bufp)-1] == '\r'))
	    bufp[strlen(bufp)-1] = '\0';

	/* handle \r\n lines */
	if (bufp[strlen(bufp)-1] == '\r')
	    bufp[strlen(bufp)-1] = '\0';

	bu_vls_printf(vp, "%s", bufp);
    } while (!done);

    /* sanity check */
    endlen = bu_vls_strlen(vp);
    if (endlen < startlen)
	return -1;

    return endlen;
}

void
bu_vls_putc(struct bu_vls *vp, int c)
{
    BU_CK_VLS(vp);

    if (vp->vls_offset + vp->vls_len+1 >= vp->vls_max)
	bu_vls_extend(vp, (unsigned int)_VLS_ALLOC_STEP);

    vp->vls_str[vp->vls_offset + vp->vls_len++] = (char)c;
    vp->vls_str[vp->vls_offset + vp->vls_len] = '\0'; /* force null termination */
}

void
bu_vls_trimspace(struct bu_vls *vp)
{
    BU_CK_VLS(vp);

    /* Remove trailing white space */
    while ((vp->vls_len > 0) &&
	   isspace(bu_vls_addr(vp)[bu_vls_strlen(vp)-1]))
	bu_vls_trunc(vp, -1);

    /* Remove leading white space */
    while ((vp->vls_len > 0) &&
	   isspace(*bu_vls_addr(vp)))
	bu_vls_nibble(vp, 1);
}

#if 0 /* comment out temporarily to prevent unused warning/error */
static int _is_format_flag(const char c);
static
int
_is_format_flag(const char c)
{
    /* from 'man sprintf' */
    switch (c) {
        /* defined in C standard; */
        case '#':
        case '0':
        case '-':
        case ' ':
        case '+':
        case '\'': /* SUSv2 */
        case 'I':  /* glibc 2.2 */
            return 1;
            break;
        default:
            return 0;
            break;
    }
}

static int _is_format_length_modifier(const char c);
static
int
_is_format_length_modifier(const char c)
{
    /* from 'man sprintf' */
    switch (c) {
        /* defined in C standard; */
        case 'h': /* can be doubled: 'hh' */
        case 'l': /* can be doubled: 'll' */
        case 'L':
        case 'j':
        case 'z':
        case 't':

        /* obsolete */
        case 'q': /* 4.4BSD and libc5 only--don't use */
            return 1;
            break;
        default:
            return 0;
            break;
    }
}

static int _is_format_conversion_specifier(const char c);
static
int
_is_format_conversion_specifier(const char c)
{
    /* from 'man sprintf' */
    switch (c) {
        case 'd':
        case 'i':
        case 'o':
        case 'u':
        case 'x':
        case 'X':
        case 'e':
        case 'E':
        case 'f':
        case 'F':
        case 'g':
        case 'G':
        case 'a':
        case 'A':
        case 'c':
        case 's':
        case 'p':
        case 'n':
        case 'm': /* glibc extension */
        case '%':
        case 'V': /* bu_vls extension */

        /* obsolete: */
        case 'C': /* (Not in C99, but in SUSv2.)  Synonym for lc.  Don't use. */
        case 'D': /* libc4--don't use */
        case 'O': /* libc5--don't use */
        case 'S': /* (Not in C99, but in SUSv2.)  Synonym for lc.  Don't use. */
        case 'U': /* libc5--don't use */
            return 1;
            break;
        default:
            return 0;
            break;
    }
}
#endif

typedef struct
vprintf_flags
{
    int fieldlen;
    int flags;
    int have_digit;
    int have_dot;
    int left_justify;
    int precision;
} vflags_t;

static void reset_vflags(vflags_t *f);
static void reset_vflags(vflags_t *f)
{
    f->fieldlen     = -1;
    f->flags        =  0;
    f->have_digit   =  0;
    f->have_dot     =  0;
    f->left_justify =  0;
    f->precision    =  0;
}

void
bu_vls_vprintf(struct bu_vls *vls, const char *fmt, va_list ap)
{
    const char *sp; /* start pointer */
    const char *ep; /* end pointer */
    int len;

    /* bit flags for fmt specifier attributes */
    /* short (char) length modifiers */
    const int SHORTINT =  0x0001;
    const int SHHRTINT =  0x0002;
    /* integer length modifiers */
    const int LONG_INT =  0x0004;
    const int LLONGINT =  0x0008;
    /* double length modifiers */
    const int LONGDBLE =  0x0010;
    /* other integer length modifiers */
    const int INTMAX_T =  0x0020;
    const int PTRDIFFT =  0x0040;
    const int SIZETINT =  0x0080;
    /* misc */
    const int FIELDLEN =  0x0100;
    const int PRECISION = 0x0200;
    /* groups */
    const int MISCINTMODS    = (INTMAX_T | PTRDIFFT | SIZETINT);
    const int SHORTINTMODS   = (SHORTINT | SHHRTINT);
    const int LONGINTMODS    = (LONG_INT | LLONGINT);
    const int ALL_INTMODS    = (SHORTINTMODS | LONGINTMODS | MISCINTMODS);
    const int ALL_DOUBLEMODS = (LONGDBLE);
    const int ALL_LENGTHMODS = (ALL_INTMODS | ALL_DOUBLEMODS);

    /* flag variables are reset for each fmt specifier */
    vflags_t f;

    char buf[BUFSIZ] = {0};
    char c;

    struct bu_vls fbuf = BU_VLS_INIT_ZERO; /* % format buffer */
    const char *fbufp  = NULL;

    if (UNLIKELY(!vls || !fmt || fmt[0] == '\0')) {
	/* nothing to print to or from */
	return;
    }

    BU_CK_VLS(vls);

    bu_vls_extend(vls, (unsigned int)_VLS_ALLOC_STEP);

    sp = fmt;
    while (*sp) {
	/* Initial state:  just printing chars */
	fmt = sp;
	while (*sp != '%' && *sp)
	    sp++;

	if (sp != fmt)
	    bu_vls_strncat(vls, fmt, (size_t)(sp-fmt));

	if (*sp == '\0')
	    break;

        /* Saw a percent sign, now need to find end of fmt specifier */
        /* All flags get reset for this fmt specifier */
        reset_vflags(&f);

        ep = sp;
	while ((c = *(++ep))) {
            if (c == ' '
                || c == '#'
                || c == '+'
                || c == '.'
                || c == '\''
                || isdigit(c)) {
                /* need to set flags for some of these */
                if (c == '.') {
                    f.have_dot = 1;
                } else if (isdigit(c)) {
                    /* set flag for later error checks */
                    f.have_digit = 1;
                }
		continue;
	    } else if (c == '-') {
                /* the first occurrence before a dot is the
                 left-justify flag, but the occurrence AFTER a dot is
                 taken to be zero precision */
                if (f.have_dot) {
                  f.precision  = 0;
                  f.have_digit = 0;
                } else if (f.have_digit) {
                    /* FIXME: ERROR condition?: invalid format string
                       (e.g., '%7.8-f') */
                    /* seems as if the fprintf man page is indefinite here,
                       looks like the '-' is passed through and
                       appears in output */
                    ;
                } else {
                    f.left_justify = 1;
                }
	    } else if (c == '*') {
                /* the first occurrence is the field width, but the
                   second occurrence is the precision specifier */
                if (!f.have_dot) {
		    f.fieldlen = va_arg(ap, int);
		    f.flags |= FIELDLEN;
                }
                else {
		    f.precision = va_arg(ap, int);
		    f.flags |= PRECISION;
                }
                /* all length modifiers below here */
	    } else if (c == 'j') {
		f.flags |= INTMAX_T;
	    } else if (c == 't') {
		f.flags |= PTRDIFFT;
	    } else if (c == 'z') {
		f.flags |= SIZETINT;
	    } else if (c == 'l') {
                /* 'l' can be doubled */
                /* clear all length modifiers AFTER we check for the
                   first 'l' */
		if (f.flags & LONG_INT) {
                    f.flags ^= ALL_LENGTHMODS;
                    f.flags |= LLONGINT;
		} else {
                    f.flags ^= ALL_LENGTHMODS;
                    f.flags |= LONG_INT;
		}
	    } else if (c == 'h') {
                /* 'h' can be doubled */
                /* clear all length modifiers AFTER we check for the
                   first 'h' */
		if (f.flags & SHORTINT) {
                    f.flags ^= ALL_LENGTHMODS;
		    f.flags |= SHHRTINT;
		} else {
                    f.flags ^= ALL_LENGTHMODS;
		    f.flags |= SHORTINT;
		}
	    } else if (c == 'L') {
                /* a length modifier for doubles */
                /* clear all length modifiers first */
                f.flags ^= ALL_LENGTHMODS;
                /* set the new flag */
                f.flags |= LONGDBLE;
	    } else
		/* Anything else must be the end of the fmt specifier */
		break;
	}

	/* libc left-justifies if there's a '-' char, even if the
	 * value is already negative, so no need to check current value
	 * of left_justify.
	 */
	if (f.fieldlen < 0) {
	    f.fieldlen = -f.fieldlen;
	    f.left_justify = 1;
	}

	/* Copy off this entire format string specifier */
	len = ep-sp+1;

	/* intentionally avoid bu_strlcpy here since the source field
	 * may be legitimately truncated.
	 */
	bu_vls_strncpy(&fbuf, sp, (size_t)len);
	fbufp = bu_vls_addr(&fbuf);

#ifndef HAVE_C99_FORMAT_SPECIFIERS
	/* if the format string uses the %z width specifier, we need
	 * to replace it with something more palatable to this busted
	 * compiler.
	 */

	if (f.flags & SIZETINT) {
	    char *fp = fbufp;
	    while (*fp) {
		if (*fp == '%') {
		    /* found the next format specifier */
		    while (*fp) {
			fp++;
			/* possible characters that can preceed the
			 * field length character (before the type).
			 */
			if (isdigit(*fp)
			    || *fp == '$'
			    || *fp == '#'
			    || *fp == '+'
			    || *fp == '.'
			    || *fp == '-'
			    || *fp == ' '
			    || *fp == '*') {
			    continue;
			}
			if (*fp == 'z') {
			    /* assume MSVC replacing instances of %z
			     * with %I (capital i) until we encounter
			     * anything different.
			     */
			    *fp = 'I';
			}

			break;
		    }
		    if (*fp == '\0') {
			break;
		    }
		}
		fp++;
	    }
	}
#endif

	/* use type specifier to grab parameter appropriately from arg
           list, and print it correctly */
	switch (c) {
	    case 's':
		{
                    /* variables used to determine final effects of
                       field length and precision (different for
                       strings versus numbers) */
                    int minfldwid = -1;
                    int maxstrlen = -1;

                    char *str = va_arg(ap, char *);

                    /* for strings only */
                    /* field length is a minimum size and precision is
                       max length of string to be printed */
		    if (f.flags & FIELDLEN) {
                        minfldwid = f.fieldlen;
                    }
		    if (f.flags & PRECISION) {
                        maxstrlen = f.precision;
                    }
		    if (str) {
                        int stringlen = (int)strlen(str);
                        struct bu_vls tmpstr = BU_VLS_INIT_ZERO;

                        /* use a copy of the string */
                        bu_vls_strcpy(&tmpstr, str);

                        /* handle a non-empty string */
                        /* strings may be truncated */
                        if (maxstrlen >= 0) {
                            if (maxstrlen < stringlen) {
                                /* have to truncate */
                                bu_vls_trunc(&tmpstr, maxstrlen);
                                stringlen = maxstrlen;
                            } else {
                                maxstrlen = stringlen;
                            }
                        }
                        minfldwid = minfldwid < maxstrlen ? maxstrlen : minfldwid;

                        if (stringlen < minfldwid) {
                            /* padding spaces needed */
                            /* start a temp string to deal with padding */
                            struct bu_vls padded = BU_VLS_INIT_ZERO;
                            int i;

			    if (f.left_justify) {
                                /* string goes before padding spaces */
				bu_vls_vlscat(&padded, &tmpstr);
                            }
                            /* now put in padding spaces in all cases */
			    for (i = 0; i < minfldwid - stringlen; ++i) {
                                bu_vls_putc(&padded, ' ');
                            }
			    if (!f.left_justify) {
                                /* string follows the padding spaces */
				bu_vls_vlscat(&padded, &tmpstr);
                            }
                            /* now we can send the padded string to the tmp string */
                            /* have to truncate it to zero length first */
                            bu_vls_trunc(&tmpstr, 0);
			    bu_vls_vlscat(&tmpstr, &padded);

                            bu_vls_free(&padded);
                        }
                        /* now take string as is */
			bu_vls_vlscat(vls, &tmpstr);

                        bu_vls_free(&tmpstr);
		    } else {
                        /* handle an empty string */
                        /* FIXME: should we trunc to precision if > fieldlen? */
                        if (f.flags & FIELDLEN) {
			    bu_vls_strncat(vls, "(null)", (size_t)f.fieldlen);
                        } else {
			    bu_vls_strcat(vls, "(null)");
                        }
		    }
		}
		break;
	    case 'V':
		{
		    struct bu_vls *vp;

		    vp = va_arg(ap, struct bu_vls *);
		    if (vp) {
			BU_CK_VLS(vp);
			if (f.flags & FIELDLEN) {
			    int stringlen = bu_vls_strlen(vp);

			    if (stringlen >= f.fieldlen)
				bu_vls_strncat(vls, bu_vls_addr(vp), (size_t)f.fieldlen);
			    else {
				struct bu_vls padded = BU_VLS_INIT_ZERO;
				int i;

				if (f.left_justify)
				    bu_vls_vlscat(&padded, vp);
				for (i = 0; i < f.fieldlen - stringlen; ++i)
				    bu_vls_putc(&padded, ' ');
				if (!f.left_justify)
				    bu_vls_vlscat(&padded, vp);
				bu_vls_vlscat(vls, &padded);
			    }
			} else {
			    bu_vls_vlscat(vls, vp);
			}
		    } else {
			if (f.flags & FIELDLEN)
			    bu_vls_strncat(vls, "(null)", (size_t)f.fieldlen);
			else
			    bu_vls_strcat(vls, "(null)");
		    }
		}
		break;
	    case 'e':
	    case 'E':
	    case 'f':
	    case 'g':
	    case 'G':
            case 'F':
		/* All floating point ==> "double" */
		{
		    double d = va_arg(ap, double);
		    if (f.flags & FIELDLEN)
			snprintf(buf, BUFSIZ, fbufp, f.fieldlen, d);
		    else
			snprintf(buf, BUFSIZ, fbufp, d);
		}
		bu_vls_strcat(vls, buf);
		break;
	    case 'o':
	    case 'u':
	    case 'x':
	    case 'X':
		if (f.flags & LONG_INT) {
		    /* Unsigned long int */
		    unsigned long l = va_arg(ap, unsigned long);
		    if (f.flags & FIELDLEN)
			snprintf(buf, BUFSIZ, fbufp, f.fieldlen, l);
		    else
			snprintf(buf, BUFSIZ, fbufp, l);
		} else if (f.flags & LLONGINT) {
		    /* Unsigned long long int */
		    unsigned long long ll = va_arg(ap, unsigned long long);
		    if (f.flags & FIELDLEN)
			snprintf(buf, BUFSIZ, fbufp, f.fieldlen, ll);
		    else
			snprintf(buf, BUFSIZ, fbufp, ll);
		} else if (f.flags & SHORTINT || f.flags & SHHRTINT) {
		    /* unsigned short int */
		    unsigned short int sh = (unsigned short int)va_arg(ap, int);
		    if (f.flags & FIELDLEN)
			snprintf(buf, BUFSIZ, fbufp, f.fieldlen, sh);
		    else
			snprintf(buf, BUFSIZ, fbufp, sh);
		} else if (f.flags & INTMAX_T) {
		    intmax_t im = va_arg(ap, intmax_t);
		    if (f.flags & FIELDLEN)
			snprintf(buf, BUFSIZ, fbufp, f.fieldlen, im);
		    else
			snprintf(buf, BUFSIZ, fbufp, im);
		} else if (f.flags & PTRDIFFT) {
		    ptrdiff_t pd = va_arg(ap, ptrdiff_t);
		    if (f.flags & FIELDLEN)
			snprintf(buf, BUFSIZ, fbufp, f.fieldlen, pd);
		    else
			snprintf(buf, BUFSIZ, fbufp, pd);
		} else if (f.flags & SIZETINT) {
		    size_t st = va_arg(ap, size_t);
		    if (f.flags & FIELDLEN)
			snprintf(buf, BUFSIZ, fbufp, f.fieldlen, st);
		    else
			snprintf(buf, BUFSIZ, fbufp, st);
		} else {
		    /* Regular unsigned int */
		    unsigned int j = (unsigned int)va_arg(ap, unsigned int);
		    if (f.flags & FIELDLEN)
			snprintf(buf, BUFSIZ, fbufp, f.fieldlen, j);
		    else
			snprintf(buf, BUFSIZ, fbufp, j);
		}
		bu_vls_strcat(vls, buf);
		break;
	    case 'd':
	    case 'i':
		if (f.flags & LONG_INT) {
		    /* Long int */
		    long l = va_arg(ap, long);
		    if (f.flags & FIELDLEN)
			snprintf(buf, BUFSIZ, fbufp, f.fieldlen, l);
		    else
			snprintf(buf, BUFSIZ, fbufp, l);
		} else if (f.flags & LLONGINT) {
		    /* Long long int */
		    long long ll = va_arg(ap, long long);
		    if (f.flags & FIELDLEN)
			snprintf(buf, BUFSIZ, fbufp, f.fieldlen, ll);
		    else
			snprintf(buf, BUFSIZ, fbufp, ll);
		} else if (f.flags & SHORTINT || f.flags & SHHRTINT) {
		    /* short int */
		    short int sh = (short int)va_arg(ap, int);
		    if (f.flags & FIELDLEN)
			snprintf(buf, BUFSIZ, fbufp, f.fieldlen, sh);
		    else
			snprintf(buf, BUFSIZ, fbufp, sh);
		} else if (f.flags & INTMAX_T) {
		    intmax_t im = va_arg(ap, intmax_t);
		    if (f.flags & FIELDLEN)
			snprintf(buf, BUFSIZ, fbufp, f.fieldlen, im);
		    else
			snprintf(buf, BUFSIZ, fbufp, im);
		} else if (f.flags & PTRDIFFT) {
		    ptrdiff_t pd = va_arg(ap, ptrdiff_t);
		    if (f.flags & FIELDLEN)
			snprintf(buf, BUFSIZ, fbufp, f.fieldlen, pd);
		    else
			snprintf(buf, BUFSIZ, fbufp, pd);
		} else if (f.flags & SIZETINT) {
		    size_t st = va_arg(ap, size_t);
		    if (f.flags & FIELDLEN)
			snprintf(buf, BUFSIZ, fbufp, f.fieldlen, st);
		    else
			snprintf(buf, BUFSIZ, fbufp, st);
		} else {
		    /* Regular int */
		    int j = va_arg(ap, int);
		    if (f.flags & FIELDLEN)
			snprintf(buf, BUFSIZ, fbufp, f.fieldlen, j);
		    else
			snprintf(buf, BUFSIZ, fbufp, j);
		}
		bu_vls_strcat(vls, buf);
		break;
	    case 'n':
	    case 'p':
		/* all pointer == "void *" */
		{
		    void *vp = (void *)va_arg(ap, void *);
		    if (f.flags & FIELDLEN)
			snprintf(buf, BUFSIZ, fbufp, f.fieldlen, vp);
		    else
			snprintf(buf, BUFSIZ, fbufp, vp);
		}
		bu_vls_strcat(vls, buf);
		break;
	    case '%':
		bu_vls_putc(vls, '%');
		break;
	    default:
		{
                    /* Something weird, maybe %c */
		    /* We hope, whatever it is, it fits in an int and the resulting
		       stringlet is smaller than sizeof(buf) bytes */
		    int j = va_arg(ap, int);
		    if (f.flags & FIELDLEN)
			snprintf(buf, BUFSIZ, fbufp, f.fieldlen, j);
		    else
			snprintf(buf, BUFSIZ, fbufp, j);
		}
		bu_vls_strcat(vls, buf);
		break;
	}
	sp = ep + 1;
    }

    va_end(ap);

    bu_vls_free(&fbuf);
}


void
bu_vls_printf(struct bu_vls *vls, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);

    BU_CK_VLS(vls);

    bu_vls_vprintf(vls, fmt, ap);
    va_end(ap);
}


void
bu_vls_sprintf(struct bu_vls *vls, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);

    BU_CK_VLS(vls);

    bu_vls_trunc(vls, 0); /* poof */
    bu_vls_vprintf(vls, fmt, ap);
    va_end(ap);
}


void
bu_vls_spaces(struct bu_vls *vp, int cnt)
{
    BU_CK_VLS(vp);

    if (UNLIKELY(cnt <= 0))
	return;
    if (vp->vls_offset + vp->vls_len + cnt+1 >= vp->vls_max)
	bu_vls_extend(vp, (unsigned)cnt);

    memset(vp->vls_str + vp->vls_offset + vp->vls_len, ' ', (size_t)cnt);
    vp->vls_len += cnt;
}


int
bu_vls_print_positions_used(const struct bu_vls *vp)
{
    char *start;
    int used;

    BU_CK_VLS(vp);

    start = strrchr(bu_vls_addr(vp), '\n');
    if (start == NULL)
	start = bu_vls_addr(vp);

    used = 0;
    while (*start != '\0') {
	if (*start == '\t') {
	    used += 8 - (used % 8);
	} else {
	    used++;
	}
	start++;
    }

    return used;
}


void
bu_vls_detab(struct bu_vls *vp)
{
    struct bu_vls src = BU_VLS_INIT_ZERO;
    char *cp;
    int used;

    BU_CK_VLS(vp);

    bu_vls_vlscatzap(&src, vp);	/* make temporary copy of src */
    bu_vls_extend(vp, (unsigned int)bu_vls_strlen(&src) + (unsigned int)_VLS_ALLOC_STEP);

    cp = bu_vls_addr(&src);
    used = 0;
    while (*cp != '\0') {
	if (*cp == '\t') {
	    int todo;
	    todo = 8 - (used % 8);
	    bu_vls_spaces(vp, todo);
	    used += todo;
	} else if (*cp == '\n') {
	    bu_vls_putc(vp, '\n');
	    used = 0;
	} else {
	    bu_vls_putc(vp, *cp);
	    used++;
	}
	cp++;
    }

    bu_vls_free(&src);
}


void
bu_vls_prepend(struct bu_vls *vp, char *str)
{
    size_t len = strlen(str);

    bu_vls_extend(vp, (unsigned int)len);

    /* memmove is supposed to be safe even if strings overlap */
    memmove(vp->vls_str+vp->vls_offset+len, vp->vls_str+vp->vls_offset, vp->vls_len);

    /* insert the data at the head of the string */
    memcpy(vp->vls_str+vp->vls_offset, str, len);

    vp->vls_len += (int)len;
}

/*
 * Local Variables:
 * mode: C
 * tab-width: 8
 * indent-tabs-mode: t
 * c-file-style: "stroustrup"
 * End:
 * ex: shiftwidth=4 tabstop=8
 */
