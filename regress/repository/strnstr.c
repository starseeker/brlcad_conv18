/*
 * Copyright (c) 2005-2018 Rich Felker
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * This is a length limited version of strstr, based on the OpenBSD
 * implementation:
 *
 * OpenBSD: strstr.c,v 1.9 2020/04/16 12:37:52 claudio Exp
 * https://github.com/libressl-portable/openbsd/blob/master/src/lib/libc/string/strstr.c
 *
 * It's primary use is for situations like libbu mapped files where a NULL
 * termination is not guaranteed, but we still need a fast search for a
 * substring within the file contents.  With this API we can use
 * bu_mapped_file's buflen to limit the search to the file contents even if
 * there is no NULL termination.
 */

#include <string.h>
#include <stdint.h>

static char *
twobyte_strstr(const unsigned char *h, const unsigned char *n, size_t hlen)
{
    size_t hpos = 0;
    uint16_t nw = n[0]<<8 | n[1], hw = h[0]<<8 | h[1];
    for (h++; *h && hpos++ && hpos < hlen && hw != nw; hw = hw<<8 | *++h);
    return *h ? (char *)h-1 : 0;
}

static char *
threebyte_strstr(const unsigned char *h, const unsigned char *n, size_t hlen)
{
    size_t hpos = 0;
    uint32_t nw = n[0]<<24 | n[1]<<16 | n[2]<<8;
    uint32_t hw = h[0]<<24 | h[1]<<16 | h[2]<<8;
    for (h+=2; *h && hpos++ && hpos < hlen && hw != nw; hw = (hw|*++h)<<8);
    return *h ? (char *)h-2 : 0;
}

static char *
fourbyte_strstr(const unsigned char *h, const unsigned char *n, size_t hlen)
{
    size_t hpos = 0;
    uint32_t nw = n[0]<<24 | n[1]<<16 | n[2]<<8 | n[3];
    uint32_t hw = h[0]<<24 | h[1]<<16 | h[2]<<8 | h[3];
    for (h+=3; *h && hpos++ && hpos < hlen && hw != nw; hw = hw<<8 | *++h);
    return *h ? (char *)h-3 : 0;
}

#define MAX(a,b) ((a)>(b)?(a):(b))
#define MIN(a,b) ((a)<(b)?(a):(b))

#define BITOP(a,b,op) \
    ((a)[(size_t)(b)/(8*sizeof *(a))] op (size_t)1<<((size_t)(b)%(8*sizeof *(a))))

/*
 * Maxime Crochemore and Dominique Perrin, Two-way string-matching,
 * Journal of the ACM, 38(3):651-675, July 1991.
 */
static char *
twoway_strstr(const unsigned char *h, const unsigned char *n, size_t hlen)
{
    const unsigned char *z;
    size_t l, ip, jp, k, p, ms, p0, mem, mem0;
    size_t byteset[32 / sizeof(size_t)] = { 0 };
    size_t shift[256];

    /* Computing length of needle and fill shift table */
    for (l=0; n[l] && h[l]; l++)
	BITOP(byteset, n[l], |=), shift[n[l]] = l+1;
    if (n[l]) return 0; /* hit the end of h */

    /* Compute maximal suffix */
    ip = -1; jp = 0; k = p = 1;
    while (jp+k<l) {
	if (n[ip+k] == n[jp+k]) {
	    if (k == p) {
		jp += p;
		k = 1;
	    } else k++;
	} else if (n[ip+k] > n[jp+k]) {
	    jp += k;
	    k = 1;
	    p = jp - ip;
	} else {
	    ip = jp++;
	    k = p = 1;
	}
    }
    ms = ip;
    p0 = p;

    /* And with the opposite comparison */
    ip = -1; jp = 0; k = p = 1;
    while (jp+k<l) {
	if (n[ip+k] == n[jp+k]) {
	    if (k == p) {
		jp += p;
		k = 1;
	    } else k++;
	} else if (n[ip+k] < n[jp+k]) {
	    jp += k;
	    k = 1;
	    p = jp - ip;
	} else {
	    ip = jp++;
	    k = p = 1;
	}
    }
    if (ip+1 > ms+1) ms = ip;
    else p = p0;

    /* Periodic needle? */
    if (memcmp(n, n+p, ms+1)) {
	mem0 = 0;
	p = MAX(ms, l-ms-1) + 1;
    } else mem0 = l-p;
    mem = 0;

    /* Initialize incremental end-of-haystack pointer */
    size_t hcnt = 0;
    z = h;

    /* Search loop */
    for (;;) {
	if (hcnt >= hlen) return 0;

	/* Update incremental end-of-haystack pointer */
	if (z-h < (long)l) {
	    /* Fast estimate for MIN(l,63) */
	    size_t grow = l | 63;
	    if (grow + hcnt > hlen) {
		grow = hlen - hcnt;
	    }
	    const unsigned char *z2 = (const unsigned char *)memchr(z, 0, grow);
	    if (z2) {
		z = z2;
		if (z-h < (long)l) return 0;
	    } else {
		z += grow;
		hcnt = hcnt + grow;
	    }
	}

	/* Check last byte first; advance by shift on mismatch */
	if (BITOP(byteset, h[l-1], &)) {
	    k = l-shift[h[l-1]];
	    if (k) {
		if (k < mem) k = mem;
		h += k;
		mem = 0;
		continue;
	    }
	} else {
	    h += l;
	    mem = 0;
	    continue;
	}

	/* Compare right half */
	for (k=MAX(ms+1,mem); n[k] && n[k] == h[k]; k++);
	if (n[k]) {
	    h += k-ms;
	    mem = 0;
	    continue;
	}
	/* Compare left half */
	for (k=ms+1; k>mem && n[k-1] == h[k-1]; k--);
	if (k <= mem) return (char *)h;
	h += p;
	mem = mem0;
    }
}

char *
bu_strnstr(const char *h, const char *n, size_t hlen)
{
    /* Return immediately on empty needle */
    if (!n[0]) return (char *)h;

    /* Use faster algorithms for short needles */
    const char *tmph = (const char *)memchr((void *)h, *n, hlen);
    if (!tmph || !n[1]) return (char *)h;
    if (!h[1]) return 0;
    if (!n[2]) return twobyte_strstr((const unsigned char *)h, (const unsigned char *)n, hlen);
    if (!h[2]) return 0;
    if (!n[3]) return threebyte_strstr((const unsigned char *)h, (const unsigned char *)n, hlen);
    if (!h[3]) return 0;
    if (!n[4]) return fourbyte_strstr((const unsigned char *)h, (const unsigned char *)n, hlen);

    return twoway_strstr((const unsigned char *)h, (const unsigned char *)n, hlen);
}

/*
 * Local Variables:
 * tab-width: 8
 * mode: C
 * indent-tabs-mode: t
 * c-file-style: "stroustrup"
 * End:
 * ex: shiftwidth=4 tabstop=8
 */
