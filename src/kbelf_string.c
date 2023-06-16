/*
	MIT License

	Copyright (c) 2023 Julian Scheffers

	Permission is hereby granted, free of charge, to any person obtaining a copy
	of this software and associated documentation files (the "Software"), to deal
	in the Software without restriction, including without limitation the rights
	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
	copies of the Software, and to permit persons to whom the Software is
	furnished to do so, subject to the following conditions:

	The above copyright notice and this permission notice shall be included in all
	copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
	OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
	SOFTWARE.
*/

#include <kbelf/string.h>



// Measure the length of `str`.
size_t kbelfq_strlen(const char *str) {
	const char *z = str;
	while (*str) str++;
	return str - z;
}

// Copy string from `src` to `dst`.
void kbelfq_strcpy(char *dst, const char *src) {
	do {
		*dst = *src;
		dst++, src++;
	} while(*src);
}

// Find last occurrance of `c` in `str`.
const char *kbelfq_strrchr(const char *str, char c) {
	const char *ref = NULL;
	while (*str) {
		if (*str == c) ref = str;
		str++;
	}
	return ref;
}

// Compare string `a` to `b`.
bool kbelfq_streq(const char *a, const char *b) {
	while (*a == *b) {
		if (!*a) return true;
		a++, b++;
	}
	return false;
}


// Copy memory from `src` to `dst`.
void kbelfq_memcpy(void *dst, const void *src, size_t nmemb) {
	if (((size_t) dst | (size_t) src | nmemb) % sizeof(unsigned long) == 0) {
		// Long word copying optimisation.
		nmemb /= sizeof(unsigned long);
		unsigned long *ptrd = dst;
		const unsigned long *ptrs = src;
		for (size_t i = 0; i < nmemb; i++) {
			ptrd[i] = ptrs[i];
		}
	} else {
		// Plain old byte copy.
		uint8_t *ptrd = dst;
		const uint8_t *ptrs = src;
		for (size_t i = 0; i < nmemb; i++) {
			ptrd[i] = ptrs[i];
		}
	}
}

// Fill memory `dst` with `c`.
void kbelfq_memset(void *dst, uint8_t c, size_t nmemb) {
	if (((size_t) dst | nmemb) % sizeof(unsigned long) == 0) {
		// Long word copying optimisation.
		nmemb /= sizeof(unsigned long);
		unsigned long *ptr = dst;
		unsigned long  tmp = c * (unsigned long) 0x0101010101010101;
		for (size_t i = 0; i < nmemb; i++) {
			ptr[i] = tmp;
		}
	} else {
		// Plain old byte copy.
		uint8_t *ptr = dst;
		for (size_t i = 0; i < nmemb; i++) {
			ptr[i] = c;
		}
	}
}

// Compare memory `a` to `b`.
bool kbelfq_memeq(const void *a, const void *b, size_t nmemb) {
	if (((size_t) a | (size_t) b | nmemb) % sizeof(unsigned long) == 0) {
		// Long word compare optimisation.
		nmemb /= sizeof(unsigned long);
		const unsigned long *ptra = a;
		const unsigned long *ptrb = b;
		for (size_t i = 0; i < nmemb; i++) {
			if (ptra[i] != ptrb[i]) return false;
		}
	} else {
		// Plain old byte compare.
		const uint8_t *ptra = a;
		const uint8_t *ptrb = b;
		for (size_t i = 0; i < nmemb; i++) {
			if (ptra[i] != ptrb[i]) return false;
		}
	}
	return true;
}


