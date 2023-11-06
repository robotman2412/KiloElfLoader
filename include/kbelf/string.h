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

#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// Measure the length of `str`.
size_t      kbelfq_strlen(char const *str) __attribute__((pure));
// Copy string from `src` to `dst`.
void        kbelfq_strcpy(char *dst, char const *src);
// Find last occurrance of `c` in `str`.
char const *kbelfq_strrchr(char const *str, char c) __attribute__((pure));
// Compare string `a` to `b`.
bool        kbelfq_streq(char const *a, char const *b) __attribute__((pure));

// Copy memory from `src` to `dst`.
void kbelfq_memcpy(void *dst, void const *src, size_t nmemb);
// Fill memory `dst` with `c`.
void kbelfq_memset(void *dst, uint8_t c, size_t nmemb);
// Compare memory `a` to `b`.
bool kbelfq_memeq(void const *a, void const *b, size_t nmemb) __attribute__((pure));
