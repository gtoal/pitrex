/**
 * @file string.h
 *
 */
/* Copyright (C) 2018 by Arjan van Vught mailto:info@raspberrypi-dmx.nl
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:

 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef STRING_H_
#define STRING_H_

#include <ctype.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

inline static int memcmp(const void *s1, const void *s2, size_t n) {
	unsigned char u1, u2;
	unsigned char *t1, *t2;

	t1 = (unsigned char *) s1;
	t2 = (unsigned char *) s2;

	for (; n-- != (size_t) 0; t1++, t2++) {
		u1 = *t1;
		u2 = *t2;
		if (u1 != u2) {
			return (int) (u1 - u2);
		}
	}

	return 0;
}

inline static void *memcpy(/*@only@*/void *dest, const void *src, size_t n) {
	char *dp = (char *) dest;
	const char *sp = (const char *) src;

	while (n-- != (size_t) 0) {
		*dp++ = *sp++;
	}

	return dest;
}

inline static void *memmove(/*@only@*/void *dst, const void *src, size_t n) {
	char *dp = (char *) dst;
	const char *sp = (const char *) src;

	if (dp < sp) {
		while (n-- != (size_t) 0) {
			*dp++ = *sp++;
		}
	} else {
		sp += n;
		dp += n;
		while (n-- != (size_t) 0) {
			*--dp = *--sp;
		}
	}

	return dst;
}

inline static void *memset(/*@only@*/void *dest, int c, size_t n) {
	char *dp = (char *) dest;

	while (n-- != (size_t) 0) {
		*dp++ = (char) c;
	}

	return dest;
}

inline static size_t strlen(const char *s) {
	const char *p = s;

	while (*s != (char) 0) {
		++s;
	}

	return (size_t) (s - p);
}

inline static char *strcpy(/*@only@*/char *s1, const char *s2) {
	char *s = s1;

	while ((*s++ = *s2++) != '\0')
		;
	return s1;
}

inline static char *strncpy(/*@only@*/char *s1, const char *s2, size_t n) {
	char *s = s1;

	while (n > 0 && *s2 != '\0') {
		*s++ = *s2++;
		--n;
	}

	while (n > 0) {
		*s++ = '\0';
		--n;
	}

	return s1;
}

inline static int strcmp(const char *s1, const char *s2) {
	unsigned char *p1 = (unsigned char *) s1;
	unsigned char *p2 = (unsigned char *) s2;

	for (; *p1 == *p2; p1++, p2++) {
		if (*p1 == (unsigned char) '\0') {
			return 0;
		}
	}

	return (int) (*p1 - *p2);
}

inline static int strncmp(const char *s1, const char *s2, size_t n) {
	unsigned char *p1 = (unsigned char *) s1;
	unsigned char *p2 = (unsigned char *) s2;

	for (; n > 0; p1++, p2++, --n) {
		if (*p1 != *p2) {
			return (int) (*p1 - *p2);
		} else if (*p1 == (unsigned char) '\0') {
			return 0;
		}
	}

	return 0;
}

inline static int strcasecmp(const char *s1, const char *s2) {
	unsigned char *p1 = (unsigned char *) s1;
	unsigned char *p2 = (unsigned char *) s2;

	for (; tolower((int) *p1) == tolower((int) *p2); p1++, p2++) {
		if (*p1 == (unsigned char) '\0') {
			return 0;
		}
	}

	return (int) (tolower((int) *p1) - tolower((int) *p2));
}

inline static int strncasecmp(const char *s1, const char *s2, size_t n) {
	unsigned char *p1 = (unsigned char *) s1;
	unsigned char *p2 = (unsigned char *) s2;

	for (; n > 0; p1++, p2++, --n) {
		if (tolower((int) *p1) != tolower((int) *p2)) {
			return (int) (tolower((int) *p1) - tolower((int) *p2));
		} else if (*p1 == (unsigned char) '\0') {
			return 0;
		}
	}

	return 0;
}

#ifdef __cplusplus
}
#endif

#endif /* STRING_H_ */
