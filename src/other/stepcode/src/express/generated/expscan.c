/* Generated by re2c 0.13.5 */
/*
* Lex source for Fed-X lexical analyzer.
*
* This software was developed by U.S. Government employees as part of
* their official duties and is not subject to copyright.
*
* $Log: expscan.l,v $
* Revision 1.12 1997/05/29 20:17:34 sauderd
* Made some changes to Symbol (to be Symbol_) and false and true to be False
* and True. These changes affect the generated expscan.c file so that it will
* compile.
*
* Revision 1.11 1994/11/22 18:32:39 clark
* Part 11 IS; group reference
*
* Revision 1.10 1994/05/12 17:22:23 libes
* added #ifdefs for flex
*
* Revision 1.9 1994/05/12 17:18:10 libes
* made flex understand multiple files
*
* Revision 1.8 1994/05/11 19:50:00 libes
* numerous fixes
*
* Revision 1.7 1993/10/15 18:47:26 libes
* CADDETC certified
*
* Revision 1.5 1993/02/22 21:46:33 libes
* fixed unmatched_open_comment handler
*
* Revision 1.4 1992/08/18 17:11:36 libes
* rm'd extraneous error messages
*
* Revision 1.3 1992/06/08 18:05:20 libes
* prettied up interface to print_objects_when_running
*
* Revision 1.2 1992/05/31 08:30:54 libes
* multiple files
*
* Revision 1.1 1992/05/28 03:52:25 libes
* Initial revision
*
* Revision 1.4 1992/05/05 19:49:03 libes
* final alpha
*
* Revision 1.3 1992/02/12 07:02:49 libes
* do sub/supertypes
*
* Revision 1.2 1992/02/09 00:49:04 libes
* does ref/use correctly
*
* Revision 1.1 1992/02/05 08:40:30 libes
* Initial revision
*
* Revision 1.0.1.1 1992/01/22 02:47:57 libes
* copied from ~pdes
*
* Revision 4.9 1991/06/14 20:49:12 libes
* removed old infinity, added backslash
*
* Revision 4.8.1.1 1991/05/16 04:07:57 libes
* made scanner (under lex) understand hooks for doing include directive
*
* Revision 4.8.1.0 1991/05/16 01:10:15 libes
* branch for fixes to old code
*
* Revision 4.8 1991/05/03 21:09:02 libes
* Added sanity check to make sure lex/flex match -DLEX/FLEX
*
* Revision 4.7 1991/05/02 05:49:18 libes
* fixed bug in testing for exceeding open_comment[nesting_level]
*
* Revision 4.6 1991/04/29 19:44:40 libes
* Print all open comments rather than just one.
*
* Revision 4.5 1991/04/29 15:39:02 libes
* Changed commenting style (back) as per SNC who claims that N9 meant to
* say that tail remarks cannot occur in an open comment, nor can nested
* comments begin in a tail remark.
*
* Revision 4.4 1991/04/29 15:01:46 libes
* Add bounds checking to nesting level history
*
* Revision 4.3 1991/04/26 20:12:50 libes
* Made scanner work with lex
* Simulated exclusive states with inclusive states
* Fixed line counting
* Speeded up whitespace matching
* Convert unknown chars to whitespace
* Disabled default rule matching (enabled "jamming")
* Enabled detection/diagnostics of unterminated comments and strings literals
* Enabled detection/diagnostics of unexpected close comments
* Disabled detection/diagnostics of nested comments
*
* Revision 4.2 1990/12/18 14:00:04 clark
* Cosmetic changes
*
* Revision 4.1 90/09/13 16:29:00 clark
* BPR 2.1 alpha
* */
#include "express/basic.h"
#include "express/error.h"
#include "express/lexact.h"
#include "express/express.h"
#include "expparse.h"
#include "expscan.h"
enum { INITIAL, code, comment, return_end_schema };
extern int	yylineno;
extern bool	yyeof;
static int	nesting_level = 0;
/* can't imagine this will ever be more than 2 or 3 - DEL */
#define MAX_NESTED_COMMENTS 20
static struct Symbol_ open_comment[MAX_NESTED_COMMENTS];
static_inline
int
SCANnextchar(char* buffer)
{
extern bool SCANread(void);
#ifdef keep_nul
static int escaped = 0;
#endif
if (SCANtext_ready || SCANread()) {
#ifdef keep_nul
if (!*SCANcurrent) {
buffer[0] = SCAN_ESCAPE;
*SCANcurrent = '0';
return 1;
} else if ((*SCANcurrent == SCAN_ESCAPE) && !escaped) {
escaped = 1;
buffer[0] = SCAN_ESCAPE;
return 1;
}
SCANbuffer.numRead--;
#endif
buffer[0] = *(SCANcurrent++);
if (!isascii(buffer[0])) {
ERRORreport_with_line(NONASCII_CHAR,yylineno,
0xff & buffer[0]);
buffer[0] = ' ';	/* substitute space */
}
return 1;
} else
return 0;
}
#define NEWLINE (yylineno++)
/* when lex looks ahead over a newline, error messages get thrown off */
/* Fortunately, we know when that occurs, so adjust for it by this hack */
#define LINENO_FUDGE (yylineno - 1)
/* added for re-initializing parser -snc 22-Apr-1992 */
void
SCAN_lex_init(char *filename, FILE *fp) {
/* return to initial scan buffer */
SCAN_current_buffer = 0;
*(SCANcurrent = SCANbuffer.text) = '\0';
SCANbuffer.readEof = false;
SCANbuffer.file = fp;
SCANbuffer.filename = (filename ? filename : "");
current_filename = SCANbuffer.filename;
}
#define PERPLEX_USING_CONDITIONS


/*              S C A N N E R _ T E M P L A T E . C
 * BRL-CAD
 *
 * Copyright (c) 1990-2011 United States Government as represented by
 * the U.S. Army Research Laboratory.
 *
 * Copyright (c) 1990 The Regents of the University of California.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following
 * disclaimer in the documentation and/or other materials provided
 * with the distribution.
 *
 * 3. The name of the author may not be used to endorse or promote
 * products derived from this software without specific prior written
 * permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/*  Parts of this file are based on sources from the flex project.
 *
 *  This code is derived from software contributed to Berkeley by
 *  Vern Paxson.
 *
 *  The United States Government has rights in this work pursuant
 *  to contract no. DE-AC03-76SF00098 between the United States
 *  Department of Energy and the University of California.
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* --- from flex's flexdef.h --- */
void buf_init(struct Buf * buf, size_t elem_size);
void buf_destroy(struct Buf * buf);
struct Buf *buf_append(struct Buf * buf, const void *ptr, int n_elem);
struct Buf *buf_concat(struct Buf* dest, const struct Buf* src);
struct Buf *buf_strappend(struct Buf *, const char *str);
struct Buf *buf_strnappend(struct Buf *, const char *str, int nchars);
struct Buf *buf_strdefine(struct Buf * buf, const char *str, const char *def);
struct Buf *buf_prints(struct Buf *buf, const char *fmt, const char* s);
struct Buf *buf_m4_define(struct Buf *buf, const char* def, const char* val);
struct Buf *buf_m4_undefine(struct Buf *buf, const char* def);
struct Buf *buf_print_strings(struct Buf * buf, FILE* out);
struct Buf *buf_linedir(struct Buf *buf, const char* filename, int lineno);

/* --- from flex's misc.c --- */
static void*
allocate_array(int size, size_t element_size)
{
    return malloc(element_size * size);
}

static void*
reallocate_array(void *array, int size, size_t element_size)
{
    return realloc(array, element_size * size);
}

/* --- from flex's buf.c --- */
/* Take note: The buffer object is sometimes used as a String buffer (one
 * continuous string), and sometimes used as a list of strings, usually line by
 * line.
 * 
 * The type is specified in buf_init by the elt_size. If the elt_size is
 * sizeof(char), then the buffer should be treated as string buffer. If the
 * elt_size is sizeof(char*), then the buffer should be treated as a list of
 * strings.
 *
 * Certain functions are only appropriate for one type or the other. 
 */

struct Buf*
buf_print_strings(struct Buf * buf, FILE* out)
{
    int i;

    if(!buf || !out) {
        return buf;
    }

    for (i = 0; i < buf->nelts; i++) {
        const char *s = ((char**)buf->elts)[i];
        if(s) {
            fprintf(out, "%s", s);
	}
    }
    return buf;
}

/* Append a "%s" formatted string to a string buffer */
struct Buf*
buf_prints(struct Buf *buf, const char *fmt, const char *s)
{
    char *t;

    t = (char*)malloc(strlen(fmt) + strlen(s) + 1);
    sprintf(t, fmt, s);
    buf = buf_strappend(buf, t);
    free(t);
    return buf;
}

int numDigits(int n)
{
    int digits;

    /* take absolute value of n */
    n = n >= 0 ? n : -n;

    if (n == 0) {
	return 1;
    }

    for (digits = 0; n > 0; digits++) {
	n /= 10;
    }

    return digits;
}

/** Append a line directive to the string buffer.
 * @param buf A string buffer.
 * @param filename file name
 * @param lineno line number
 * @return buf
 */
struct Buf*
buf_linedir(struct Buf *buf, const char* filename, int lineno)
{
    char *t;
    const char fmt[] = "#line %d \"%s\"\n";
    
    t = (char*)malloc(strlen(fmt) + strlen(filename) + numDigits(lineno) + 1);
    sprintf(t, fmt, lineno, filename);
    buf = buf_strappend(buf, t);
    free(t);
    return buf;
}


/** Append the contents of @a src to @a dest.
 * @param @a dest the destination buffer
 * @param @a dest the source buffer
 * @return @a dest
 */
struct Buf*
buf_concat(struct Buf* dest, const struct Buf* src)
{
    buf_append(dest, src->elts, src->nelts);
    return dest;
}


/* Appends n characters in str to buf. */
struct Buf*
buf_strnappend(struct Buf *buf, const char *str, int n)
{
    buf_append(buf, str, n + 1);

    /* "undo" the '\0' character that buf_append() already copied. */
    buf->nelts--;

    return buf;
}

/* Appends characters in str to buf. */
struct Buf*
buf_strappend(struct Buf *buf, const char *str)
{
    return buf_strnappend(buf, str, strlen(str));
}

/* appends "#define str def\n" */
struct Buf*
buf_strdefine(struct Buf *buf, const char *str, const char *def)
{
    buf_strappend(buf, "#define ");
    buf_strappend(buf, " ");
    buf_strappend(buf, str);
    buf_strappend(buf, " ");
    buf_strappend(buf, def);
    buf_strappend(buf, "\n");
    return buf;
}

/** Pushes "m4_define( [[def]], [[val]])m4_dnl" to end of buffer.
 * @param buf A buffer as a list of strings.
 * @param def The m4 symbol to define.
 * @param val The definition; may be NULL.
 * @return buf
 */
struct Buf*
buf_m4_define(struct Buf *buf, const char* def, const char* val)
{
    const char *fmt = "m4_define( [[%s]], [[%s]])m4_dnl\n";
    char *str;

    val = val ? val : "";
    str = (char*)malloc(strlen(fmt) + strlen(def) + strlen(val) + 2);

    sprintf(str, fmt, def, val);
    buf_append(buf, &str, 1);
    return buf;
}

/** Pushes "m4_undefine([[def]])m4_dnl" to end of buffer.
 * @param buf A buffer as a list of strings.
 * @param def The m4 symbol to undefine.
 * @return buf
 */
struct Buf*
buf_m4_undefine(struct Buf *buf, const char* def)
{
    const char *fmt = "m4_undefine( [[%s]])m4_dnl\n";
    char *str;

    str = (char*)malloc(strlen(fmt) + strlen(def) + 2);

    sprintf(str, fmt, def);
    buf_append(buf, &str, 1);
    return buf;
}

/* create buf with 0 elements, each of size elem_size. */
void
buf_init(struct Buf *buf, size_t elem_size)
{
    buf->elts = (void*)0;
    buf->nelts = 0;
    buf->elt_size = elem_size;
    buf->nmax = 0;
}

/* frees memory */
void
buf_destroy(struct Buf *buf)
{
    if (buf && buf->elts) {
	free(buf->elts);
    }
    if (buf)
	buf->elts = (void*)0;
}

/* appends ptr[] to buf, grow if necessary.
 * n_elem is number of elements in ptr[], NOT bytes.
 * returns buf.
 * We grow by mod(512) boundaries.
 */
struct Buf*
buf_append(struct Buf *buf, const void *ptr, int n_elem)
{
    int n_alloc = 0;

    if (!ptr || n_elem == 0) {
	return buf;
    }

    /* May need to alloc more. */
    if (n_elem + buf->nelts > buf->nmax) {
	/* exact amount needed... */
	n_alloc = (n_elem + buf->nelts) * buf->elt_size;

	/* ...plus some extra */
	if (((n_alloc * buf->elt_size) % 512) != 0 && buf->elt_size < 512) {
	    n_alloc += (512 - ((n_alloc * buf->elt_size) % 512)) / buf->elt_size;
	}
	if (!buf->elts) {
	    buf->elts = allocate_array(n_alloc, buf->elt_size);
	} else {
	    buf->elts = reallocate_array(buf->elts, n_alloc, buf->elt_size);
	}
	buf->nmax = n_alloc;
    }
    memcpy((char*)buf->elts + buf->nelts * buf->elt_size, ptr,
	n_elem * buf->elt_size);

    buf->nelts += n_elem;

    return buf;
}

/* --- */
/* input buffering support
 * note that these routines assume buf->elt_size == sizeof(char)
 */

/* get pointer to the start of the first element */
static char*
bufferFirstElt(struct Buf *buf)
{
    return (char*)buf->elts;
}

/* get pointer to the start of the last element */
static char*
bufferLastElt(struct Buf *buf)
{
    if (buf->nelts < 1) {
	return NULL;
    }
    return bufferFirstElt(buf) + buf->nelts - 1;
}

static void
bufferAppendChar(struct Buf *buf, char c)
{
    char *cp = &c;
    buf_append(buf, cp, 1);
}

/* Copy up to n input characters to the end of scanner buffer. If EOF is
 * encountered before n characters are copied, scanner->atEOI flag is set.
 */
static void
bufferAppend(perplex_t scanner, size_t n)
{
    struct Buf *buf;
    FILE *in;
    size_t i;
    int c;
    char *bufStart;
    size_t markerOffset, tokenStartOffset, cursorOffset;

    buf = scanner->buffer;
    in = scanner->inFile;

    /* save marker offsets */
    bufStart = (char*)buf->elts;
    cursorOffset = (size_t)(scanner->cursor - bufStart);
    markerOffset = (size_t)(scanner->marker - bufStart);
    tokenStartOffset = (size_t)(scanner->tokenStart - bufStart);

    /* remove last (null) element */
    buf->nelts--;

    for (i = 0; i < n; i++) {
	if ((c = fgetc(in)) == EOF) {
	    scanner->atEOI = 1;
	    break;
	}
	bufferAppendChar(buf, c);
    }

    /* (scanner->null - eltSize) should be the last input element,
     * we put a literal null after this element for debugging
     */
    bufferAppendChar(buf, '\0');
    scanner->null = bufferLastElt(buf);

    /* update markers in case append caused buffer to be reallocated */
    bufStart = (char*)buf->elts;
    scanner->cursor = bufStart + cursorOffset;
    scanner->marker = bufStart + markerOffset;
    scanner->tokenStart = bufStart + tokenStartOffset;
}

/* Appends up to n characters of input to scanner buffer. */
static void
bufferFill(perplex_t scanner, size_t n)
{
    struct Buf *buf;
    size_t totalElts, usedElts, freeElts;

    if (scanner->atEOI) {
	/* nothing to add to buffer */
	return;
    }

    buf = scanner->buffer;

    totalElts = (size_t)buf->nmax;
    usedElts = (size_t)buf->nelts;
    freeElts = totalElts - usedElts;

    /* not enough room for append, shift buffer contents to avoid realloc */
    if (n > freeElts) {
	void *bufFirst, *scannerFirst, *tokenStart, *marker, *null;
	size_t bytesInUse, shiftSize;

	tokenStart = (void*)scanner->tokenStart;
	marker = (void*)scanner->marker;
	null = (void*)scanner->null;

	bufFirst = bufferFirstElt(buf);

	/* Find first buffer element still in use by scanner. Will be
	 * tokenStart unless backtracking marker is in use.
	 */
	scannerFirst = tokenStart;
	if (marker >= bufFirst && marker < tokenStart) {
	    scannerFirst = marker;
	}

	/* bytes of input being used by scanner */
	bytesInUse = (size_t)null - (size_t)scannerFirst + 1;

	/* copy in-use elements to start of buffer */
	memmove(bufFirst, scannerFirst, bytesInUse);

	/* update number of elements */
        buf->nelts = bytesInUse / buf->elt_size;

	/* update markers */
	shiftSize = (size_t)scannerFirst - (size_t)bufFirst;
	scanner->marker     -= shiftSize;
	scanner->cursor     -= shiftSize;
	scanner->null       -= shiftSize;
	scanner->tokenStart -= shiftSize;
    }
    bufferAppend(scanner, n);
}

static char*
getTokenText(perplex_t scanner)
{
    int tokenChars = scanner->cursor - scanner->tokenStart;

    if (scanner->tokenText != NULL) {
	free(scanner->tokenText);
    }

    scanner->tokenText = (char*)malloc(sizeof(char) * (tokenChars + 1));

    memcpy(scanner->tokenText, scanner->tokenStart, tokenChars);
    scanner->tokenText[tokenChars] = '\0';

    return scanner->tokenText;
}

/* scanner helpers */
#define UPDATE_START  scanner->tokenStart = scanner->cursor;
#define IGNORE_TOKEN  UPDATE_START; continue;
#define       yytext  getTokenText(scanner)
#define      yyextra  scanner->extra

static perplex_t
newScanner()
{
    perplex_t scanner;
    scanner = (perplex_t)calloc(1, sizeof(struct perplex));

    return scanner;
}

static void
initBuffer(perplex_t scanner)
{
    scanner->buffer = (struct Buf*)malloc(sizeof(struct Buf));
    buf_init(scanner->buffer, sizeof(char));
}

/* public functions */

perplex_t
perplexStringScanner(char *firstChar, size_t numChars)
{
    size_t i;
    struct Buf *buf;
    perplex_t scanner = newScanner();

    scanner->inFile = NULL;

    initBuffer(scanner);
    buf = scanner->buffer;

    /* copy string to buffer */
    for (i = 0; i < numChars; i++) {
	bufferAppendChar(buf, firstChar[i]);
    }
    bufferAppendChar(buf, '\0');

    scanner->marker = scanner->cursor = bufferFirstElt(buf);
    scanner->null = bufferLastElt(buf);
    scanner->atEOI = 1;

    return scanner;
}

perplex_t
perplexFileScanner(FILE *input)
{
    char *bufFirst;
    perplex_t scanner = newScanner();

    scanner->inFile = input;

    initBuffer(scanner);
    bufferAppendChar(scanner->buffer, '\0');

    bufFirst = bufferFirstElt(scanner->buffer);
    scanner->null = scanner->marker = scanner->cursor = bufFirst;

    return scanner;
}

void
perplexFree(perplex_t scanner)
{
    if (scanner->buffer != NULL) {
	buf_destroy(scanner->buffer);
	free(scanner->buffer);
    }

    free(scanner);
}

void
perplexSetExtra(perplex_t scanner, void *extra)
{
    scanner->extra = extra;
}

void*
perplexGetExtra(perplex_t scanner)
{
    return scanner->extra;
}

/* Make c the next character to be scanned.
 *
 * This performs an insert, leaving the input buffer prior to the cursor
*  unchanged.
 */
void
perplexUnput(perplex_t scanner, char c)
{
    struct Buf *buf;
    char *curr, *cursor, *bufStart;
    size_t markerOffset, tokenStartOffset, cursorOffset;

    buf = scanner->buffer;

    /* save marker offsets */
    bufStart = bufferFirstElt(buf);
    cursorOffset = (size_t)(scanner->cursor - bufStart);
    markerOffset = (size_t)(scanner->marker - bufStart);
    tokenStartOffset = (size_t)(scanner->tokenStart - bufStart);

    /* append character to create room for shift */
    bufferAppendChar(buf, '\0');
    scanner->null = bufferLastElt(buf);

    /* update markers in case append caused buffer to be reallocated */
    bufStart = bufferFirstElt(buf);
    scanner->cursor = bufStart + cursorOffset;
    scanner->marker = bufStart + markerOffset;
    scanner->tokenStart = bufStart + tokenStartOffset;

    /* input from cursor to null is shifted to the right */
    cursor = scanner->cursor;
    for (curr = scanner->null; curr != cursor; curr--) {
	curr[0] = curr[-1];
    }

    /* insert c */
    *curr = c;
}

#ifdef PERPLEX_USING_CONDITIONS
/* start-condition support */
static void
setCondition(perplex_t scanner, int cond)
{
    scanner->condition = cond;
}

static int
getCondition(perplex_t scanner)
{
    return scanner->condition;
}

/* required re2c macros */
#define YYGETCONDITION     getCondition(scanner)
#define YYSETCONDITION(c)  setCondition(scanner, c)
#endif

/* required re2c macro */
#define YYFILL(n)          bufferFill(scanner, n)

/* scanner */
static int PERPLEX_PRIVATE_LEXER;

int
PERPLEX_PUBLIC_LEXER {
    int ret;

    UPDATE_START;

    scanner->tokenText = NULL;

    ret = PERPLEX_LEXER_private(scanner);

    if (scanner->tokenText != NULL) {
	free(scanner->tokenText);
	scanner->tokenText = NULL;
    }

    return ret;
}

static int
PERPLEX_PRIVATE_LEXER {
    char yych;

    PERPLEX_ON_ENTER;

    while (1) {
	if (scanner->atEOI && scanner->cursor >= scanner->null) {
	    return YYEOF;
	}

	{
		unsigned int yyaccept = 0;
		switch (YYGETCONDITION) {
		case 0: goto yyc_0;
		case code: goto yyc_code;
		case comment: goto yyc_comment;
		case return_end_schema: goto yyc_return_end_schema;
		}
/* *********************************** */
yyc_0:

		YYSETCONDITION(code);
		{
}
/* *********************************** */
yyc_code:
		if ((scanner->null - scanner->cursor) < 4) YYFILL(4);
		yych = *scanner->cursor;
		switch (yych) {
		case '\t':
		case ' ':	goto yy8;
		case '\n':	goto yy9;
		case '"':	goto yy11;
		case '$':
		case '&':
		case '@':
		case '^':
		case '~':	goto yy12;
		case '%':	goto yy14;
		case '\'':	goto yy15;
		case '(':	goto yy16;
		case ')':	goto yy18;
		case '*':	goto yy20;
		case '+':	goto yy22;
		case ',':	goto yy24;
		case '-':	goto yy26;
		case '.':	goto yy28;
		case '/':	goto yy30;
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':	goto yy32;
		case ':':	goto yy34;
		case ';':	goto yy36;
		case '<':	goto yy38;
		case '=':	goto yy40;
		case '>':	goto yy42;
		case '?':	goto yy44;
		case 'A':
		case 'B':
		case 'C':
		case 'D':
		case 'E':
		case 'F':
		case 'G':
		case 'H':
		case 'I':
		case 'J':
		case 'K':
		case 'L':
		case 'M':
		case 'N':
		case 'O':
		case 'P':
		case 'Q':
		case 'R':
		case 'S':
		case 'T':
		case 'U':
		case 'V':
		case 'W':
		case 'X':
		case 'Y':
		case 'Z':
		case 'a':
		case 'b':
		case 'c':
		case 'd':
		case 'e':
		case 'f':
		case 'g':
		case 'h':
		case 'i':
		case 'j':
		case 'k':
		case 'l':
		case 'm':
		case 'n':
		case 'o':
		case 'p':
		case 'q':
		case 'r':
		case 's':
		case 't':
		case 'u':
		case 'v':
		case 'w':
		case 'x':
		case 'y':
		case 'z':	goto yy46;
		case '[':	goto yy48;
		case '\\':	goto yy50;
		case ']':	goto yy52;
		case '_':	goto yy54;
		case '{':	goto yy56;
		case '|':	goto yy58;
		case '}':	goto yy60;
		default:	goto yy6;
		}
yy5:
		{
IGNORE_TOKEN; }
yy6:
		++scanner->cursor;
yy7:
		{
IGNORE_TOKEN; }
yy8:
		yych = *++scanner->cursor;
		goto yy127;
yy9:
		++scanner->cursor;
		{
NEWLINE;
IGNORE_TOKEN;
}
yy11:
		yyaccept = 0;
		yych = *(scanner->marker = ++scanner->cursor);
		goto yy121;
yy12:
		++scanner->cursor;
yy13:
		{
ERRORreport_with_line(UNEXPECTED_CHARACTER,yylineno,yytext[0]);
IGNORE_TOKEN;
}
yy14:
		yych = *++scanner->cursor;
		switch (yych) {
		case '0':
		case '1':	goto yy117;
		default:	goto yy13;
		}
yy15:
		yyaccept = 0;
		yych = *(scanner->marker = ++scanner->cursor);
		goto yy112;
yy16:
		++scanner->cursor;
		switch ((yych = *scanner->cursor)) {
		case '*':	goto yy109;
		default:	goto yy17;
		}
yy17:
		{
return TOK_LEFT_PAREN; }
yy18:
		++scanner->cursor;
		{
return TOK_RIGHT_PAREN; }
yy20:
		++scanner->cursor;
		switch ((yych = *scanner->cursor)) {
		case ')':	goto yy105;
		case '*':	goto yy107;
		default:	goto yy21;
		}
yy21:
		{
return TOK_TIMES; }
yy22:
		++scanner->cursor;
		{
return TOK_PLUS; }
yy24:
		++scanner->cursor;
		{
return TOK_COMMA; }
yy26:
		yyaccept = 1;
		yych = *(scanner->marker = ++scanner->cursor);
		switch (yych) {
		case '-':	goto yy101;
		default:	goto yy27;
		}
yy27:
		{
return TOK_MINUS; }
yy28:
		++scanner->cursor;
		{
return TOK_DOT; }
yy30:
		++scanner->cursor;
		{
return TOK_REAL_DIV; }
yy32:
		++scanner->cursor;
		switch ((yych = *scanner->cursor)) {
		case '.':	goto yy94;
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':	goto yy92;
		default:	goto yy33;
		}
yy33:
		{
return SCANprocess_integer_literal(yytext);
}
yy34:
		yyaccept = 2;
		yych = *(scanner->marker = ++scanner->cursor);
		switch (yych) {
		case '<':	goto yy84;
		case '=':	goto yy85;
		default:	goto yy35;
		}
yy35:
		{
return TOK_COLON; }
yy36:
		yyaccept = 3;
		yych = *(scanner->marker = ++scanner->cursor);
		switch (yych) {
		case '\t':
		case ' ':	goto yy76;
		case '-':	goto yy79;
		default:	goto yy37;
		}
yy37:
		{
return SCANprocess_semicolon(yytext, 0); }
yy38:
		++scanner->cursor;
		switch ((yych = *scanner->cursor)) {
		case '*':	goto yy74;
		case '=':	goto yy72;
		case '>':	goto yy70;
		default:	goto yy39;
		}
yy39:
		{
return TOK_LESS_THAN; }
yy40:
		++scanner->cursor;
		{
return TOK_EQUAL; }
yy42:
		++scanner->cursor;
		switch ((yych = *scanner->cursor)) {
		case '=':	goto yy68;
		default:	goto yy43;
		}
yy43:
		{
return TOK_GREATER_THAN; }
yy44:
		++scanner->cursor;
		{
return TOK_QUESTION_MARK; }
yy46:
		++scanner->cursor;
		yych = *scanner->cursor;
		goto yy67;
yy47:
		{
return SCANprocess_identifier_or_keyword(yytext);
}
yy48:
		++scanner->cursor;
		{
return TOK_LEFT_BRACKET; }
yy50:
		++scanner->cursor;
		{
return TOK_BACKSLASH; }
yy52:
		++scanner->cursor;
		{
return TOK_RIGHT_BRACKET; }
yy54:
		++scanner->cursor;
		yych = *scanner->cursor;
		goto yy65;
yy55:
		{
ERRORreport_with_line(BAD_IDENTIFIER, yylineno, yytext);
return SCANprocess_identifier_or_keyword(yytext);
}
yy56:
		++scanner->cursor;
		{
return TOK_LEFT_CURL; }
yy58:
		++scanner->cursor;
		switch ((yych = *scanner->cursor)) {
		case '|':	goto yy62;
		default:	goto yy59;
		}
yy59:
		{
return TOK_SUCH_THAT; }
yy60:
		++scanner->cursor;
		{
return TOK_RIGHT_CURL; }
yy62:
		++scanner->cursor;
		{
return TOK_CONCAT_OP; }
yy64:
		++scanner->cursor;
		if (scanner->null <= scanner->cursor) YYFILL(1);
		yych = *scanner->cursor;
yy65:
		switch (yych) {
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
		case 'A':
		case 'B':
		case 'C':
		case 'D':
		case 'E':
		case 'F':
		case 'G':
		case 'H':
		case 'I':
		case 'J':
		case 'K':
		case 'L':
		case 'M':
		case 'N':
		case 'O':
		case 'P':
		case 'Q':
		case 'R':
		case 'S':
		case 'T':
		case 'U':
		case 'V':
		case 'W':
		case 'X':
		case 'Y':
		case 'Z':
		case '_':
		case 'a':
		case 'b':
		case 'c':
		case 'd':
		case 'e':
		case 'f':
		case 'g':
		case 'h':
		case 'i':
		case 'j':
		case 'k':
		case 'l':
		case 'm':
		case 'n':
		case 'o':
		case 'p':
		case 'q':
		case 'r':
		case 's':
		case 't':
		case 'u':
		case 'v':
		case 'w':
		case 'x':
		case 'y':
		case 'z':	goto yy64;
		default:	goto yy55;
		}
yy66:
		++scanner->cursor;
		if (scanner->null <= scanner->cursor) YYFILL(1);
		yych = *scanner->cursor;
yy67:
		switch (yych) {
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
		case 'A':
		case 'B':
		case 'C':
		case 'D':
		case 'E':
		case 'F':
		case 'G':
		case 'H':
		case 'I':
		case 'J':
		case 'K':
		case 'L':
		case 'M':
		case 'N':
		case 'O':
		case 'P':
		case 'Q':
		case 'R':
		case 'S':
		case 'T':
		case 'U':
		case 'V':
		case 'W':
		case 'X':
		case 'Y':
		case 'Z':
		case '_':
		case 'a':
		case 'b':
		case 'c':
		case 'd':
		case 'e':
		case 'f':
		case 'g':
		case 'h':
		case 'i':
		case 'j':
		case 'k':
		case 'l':
		case 'm':
		case 'n':
		case 'o':
		case 'p':
		case 'q':
		case 'r':
		case 's':
		case 't':
		case 'u':
		case 'v':
		case 'w':
		case 'x':
		case 'y':
		case 'z':	goto yy66;
		default:	goto yy47;
		}
yy68:
		++scanner->cursor;
		{
return TOK_GREATER_EQUAL; }
yy70:
		++scanner->cursor;
		{
return TOK_NOT_EQUAL; }
yy72:
		++scanner->cursor;
		{
return TOK_LESS_EQUAL; }
yy74:
		++scanner->cursor;
		{
return TOK_ALL_IN; }
yy76:
		++scanner->cursor;
		if ((scanner->null - scanner->cursor) < 2) YYFILL(2);
		yych = *scanner->cursor;
		switch (yych) {
		case '\t':
		case ' ':	goto yy76;
		case '-':	goto yy79;
		default:	goto yy78;
		}
yy78:
		scanner->cursor = scanner->marker;
		switch (yyaccept) {
		case 0: 	goto yy7;
		case 1: 	goto yy27;
		case 2: 	goto yy35;
		case 3: 	goto yy37;
		case 4: 	goto yy96;
		case 5: 	goto yy114;
		}
yy79:
		yych = *++scanner->cursor;
		switch (yych) {
		case '-':	goto yy80;
		default:	goto yy78;
		}
yy80:
		++scanner->cursor;
		if (scanner->null <= scanner->cursor) YYFILL(1);
		yych = *scanner->cursor;
		switch (yych) {
		case '\n':	goto yy82;
		default:	goto yy80;
		}
yy82:
		++scanner->cursor;
		{
NEWLINE;
return SCANprocess_semicolon(yytext, 1);
}
yy84:
		yych = *++scanner->cursor;
		switch (yych) {
		case '>':	goto yy89;
		default:	goto yy78;
		}
yy85:
		++scanner->cursor;
		switch ((yych = *scanner->cursor)) {
		case ':':	goto yy87;
		default:	goto yy86;
		}
yy86:
		{
return TOK_ASSIGNMENT; }
yy87:
		++scanner->cursor;
		{
return TOK_INST_EQUAL; }
yy89:
		yych = *++scanner->cursor;
		switch (yych) {
		case ':':	goto yy90;
		default:	goto yy78;
		}
yy90:
		++scanner->cursor;
		{
return TOK_INST_NOT_EQUAL; }
yy92:
		++scanner->cursor;
		if (scanner->null <= scanner->cursor) YYFILL(1);
		yych = *scanner->cursor;
		switch (yych) {
		case '.':	goto yy94;
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':	goto yy92;
		default:	goto yy33;
		}
yy94:
		yyaccept = 4;
		scanner->marker = ++scanner->cursor;
		if ((scanner->null - scanner->cursor) < 3) YYFILL(3);
		yych = *scanner->cursor;
		switch (yych) {
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':	goto yy94;
		case 'E':
		case 'e':	goto yy97;
		default:	goto yy96;
		}
yy96:
		{
return SCANprocess_real_literal(yytext);
}
yy97:
		yych = *++scanner->cursor;
		switch (yych) {
		case '+':
		case '-':	goto yy98;
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':	goto yy99;
		default:	goto yy78;
		}
yy98:
		yych = *++scanner->cursor;
		switch (yych) {
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':	goto yy99;
		default:	goto yy78;
		}
yy99:
		++scanner->cursor;
		if (scanner->null <= scanner->cursor) YYFILL(1);
		yych = *scanner->cursor;
		switch (yych) {
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':	goto yy99;
		default:	goto yy96;
		}
yy101:
		++scanner->cursor;
		if (scanner->null <= scanner->cursor) YYFILL(1);
		yych = *scanner->cursor;
		switch (yych) {
		case '\n':	goto yy103;
		default:	goto yy101;
		}
yy103:
		++scanner->cursor;
		{
NEWLINE;
SCANsave_comment(yytext);
IGNORE_TOKEN;
}
yy105:
		++scanner->cursor;
		{
ERRORreport_with_line(UNMATCHED_CLOSE_COMMENT, yylineno);
IGNORE_TOKEN;
}
yy107:
		++scanner->cursor;
		{
return TOK_EXP; }
yy109:
		++scanner->cursor;
		YYSETCONDITION(comment);
		{
if (nesting_level <MAX_NESTED_COMMENTS) {
open_comment[nesting_level].line = yylineno;
open_comment[nesting_level].filename = current_filename;
}
nesting_level++;
IGNORE_TOKEN;
}
yy111:
		++scanner->cursor;
		if (scanner->null <= scanner->cursor) YYFILL(1);
		yych = *scanner->cursor;
yy112:
		switch (yych) {
		case '\n':	goto yy115;
		case '\'':	goto yy113;
		default:	goto yy111;
		}
yy113:
		yyaccept = 5;
		scanner->marker = ++scanner->cursor;
		if (scanner->null <= scanner->cursor) YYFILL(1);
		yych = *scanner->cursor;
		switch (yych) {
		case '\'':	goto yy111;
		default:	goto yy114;
		}
yy114:
		{
return SCANprocess_string(yytext);
}
yy115:
		++scanner->cursor;
		{
ERRORreport_with_line(UNTERMINATED_STRING, LINENO_FUDGE);
NEWLINE;
return SCANprocess_string(yytext);
}
yy117:
		++scanner->cursor;
		if (scanner->null <= scanner->cursor) YYFILL(1);
		yych = *scanner->cursor;
		switch (yych) {
		case '0':
		case '1':	goto yy117;
		default:	goto yy119;
		}
yy119:
		{
return SCANprocess_binary_literal(yytext);
}
yy120:
		++scanner->cursor;
		if (scanner->null <= scanner->cursor) YYFILL(1);
		yych = *scanner->cursor;
yy121:
		switch (yych) {
		case '\n':	goto yy122;
		case '"':	goto yy124;
		default:	goto yy120;
		}
yy122:
		++scanner->cursor;
		{
ERRORreport_with_line(UNTERMINATED_STRING, LINENO_FUDGE);
NEWLINE;
return SCANprocess_encoded_string(yytext);
}
yy124:
		++scanner->cursor;
		{
return SCANprocess_encoded_string(yytext);
}
yy126:
		++scanner->cursor;
		if (scanner->null <= scanner->cursor) YYFILL(1);
		yych = *scanner->cursor;
yy127:
		switch (yych) {
		case '\t':
		case ' ':	goto yy126;
		default:	goto yy5;
		}
/* *********************************** */
yyc_comment:
		if ((scanner->null - scanner->cursor) < 2) YYFILL(2);
		yych = *scanner->cursor;
		switch (yych) {
		case '\t':
		case ' ':	goto yy132;
		case '\n':	goto yy133;
		case '(':	goto yy135;
		case ')':	goto yy137;
		case '*':	goto yy138;
		default:	goto yy131;
		}
yy130:
		{
IGNORE_TOKEN; }
yy131:
		yych = *++scanner->cursor;
		goto yy144;
yy132:
		yych = *++scanner->cursor;
		switch (yych) {
		case '\t':
		case ' ':	goto yy145;
		default:	goto yy144;
		}
yy133:
		++scanner->cursor;
		{
NEWLINE;
IGNORE_TOKEN;
}
yy135:
		++scanner->cursor;
		switch ((yych = *scanner->cursor)) {
		case '*':	goto yy141;
		default:	goto yy136;
		}
yy136:
		{
IGNORE_TOKEN; }
yy137:
		yych = *++scanner->cursor;
		goto yy136;
yy138:
		yych = *++scanner->cursor;
		switch (yych) {
		case ')':	goto yy139;
		default:	goto yy136;
		}
yy139:
		++scanner->cursor;
		{
if (0 == --nesting_level) {
YYSETCONDITION(code);
}
IGNORE_TOKEN;
}
yy141:
		++scanner->cursor;
		{
if (nesting_level <MAX_NESTED_COMMENTS) {
open_comment[nesting_level].line = yylineno;
open_comment[nesting_level].filename = current_filename;
}
nesting_level++;
IGNORE_TOKEN;
}
yy143:
		++scanner->cursor;
		if (scanner->null <= scanner->cursor) YYFILL(1);
		yych = *scanner->cursor;
yy144:
		switch (yych) {
		case '\n':
		case '(':
		case ')':
		case '*':	goto yy130;
		default:	goto yy143;
		}
yy145:
		++scanner->cursor;
		if (scanner->null <= scanner->cursor) YYFILL(1);
		yych = *scanner->cursor;
		switch (yych) {
		case '\t':
		case ' ':	goto yy145;
		case '\n':
		case '(':
		case ')':
		case '*':	goto yy130;
		default:	goto yy143;
		}
/* *********************************** */
yyc_return_end_schema:
		if ((scanner->null - scanner->cursor) < 2) YYFILL(2);
		yych = *scanner->cursor;
		switch (yych) {
		case '\t':
		case ' ':	goto yy152;
		case '\n':	goto yy153;
		case '(':	goto yy155;
		case 'X':
		case 'x':	goto yy156;
		default:	goto yy150;
		}
yy149:
		{
IGNORE_TOKEN; }
yy150:
		++scanner->cursor;
yy151:
		{
IGNORE_TOKEN; }
yy152:
		yych = *++scanner->cursor;
		goto yy161;
yy153:
		++scanner->cursor;
		{
NEWLINE;
IGNORE_TOKEN;
}
yy155:
		yych = *++scanner->cursor;
		switch (yych) {
		case '*':	goto yy158;
		default:	goto yy151;
		}
yy156:
		++scanner->cursor;
		YYSETCONDITION(code);
		{
return TOK_END_SCHEMA;
}
yy158:
		++scanner->cursor;
		YYSETCONDITION(comment);
		{
if (nesting_level <MAX_NESTED_COMMENTS) {
open_comment[nesting_level].line = yylineno;
open_comment[nesting_level].filename = current_filename;
}
nesting_level++;
IGNORE_TOKEN;
}
yy160:
		++scanner->cursor;
		if (scanner->null <= scanner->cursor) YYFILL(1);
		yych = *scanner->cursor;
yy161:
		switch (yych) {
		case '\t':
		case ' ':	goto yy160;
		default:	goto yy149;
		}
	}

    }
}

/* start code */
void
SCANskip_to_end_schema(perplex_t scanner)
{
while (yylex(scanner) != TOK_END_SCHEMA);
perplexUnput(scanner, 'X');	/* any old character */
YYSETCONDITION(return_end_schema);
}
