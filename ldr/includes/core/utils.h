#pragma once
#include <windows.h>

static inline char *LdrStrchr(const char *str, int c) {
    volatile char *s = (volatile char *)str;
    char ch = (char)c;
    while (*s != ch) {
        if (*s == '\0')
            return NULL;
        s++;
    }
    return (char *)s;
}

static inline SIZE_T LdrStrlen(const char *str) {
    const volatile char *s = (const volatile char *)str;
    SIZE_T n = 0;
    while (*s++) n++;
    return n;
}

static inline int LdrStrncmp(const char *s1, const char *s2, SIZE_T n) {
    volatile unsigned char *p1 = (volatile unsigned char *)s1;
    volatile unsigned char *p2 = (volatile unsigned char *)s2;
    for (SIZE_T i = 0; i < n; i++) {
        if (p1[i] != p2[i])
            return p1[i] - p2[i];
        if (p1[i] == '\0')
            return 0;
    }
    return 0;
}

 static inline void *LdrMemcpy(void *dst, const void *src, SIZE_T n) {
    volatile unsigned char *d = (volatile unsigned char *)dst;
    const unsigned char *s = (const unsigned char *)src;
    while (n--) *d++ = *s++;
    return dst;
}

static inline int LdrMemcmp(const void *s1, const void *s2, SIZE_T n) {
    volatile unsigned char *p1 = (volatile unsigned char *)s1;
    volatile unsigned char *p2 = (volatile unsigned char *)s2;
    for (SIZE_T i = 0; i < n; i++) {
        if (p1[i] != p2[i]) {
            return p1[i] - p2[i];  // was missing semicolon
        }
    }
    return 0;
}

 static inline void *LdrMemset(void *dst, int val, SIZE_T n) {
    volatile unsigned char *d = (volatile unsigned char *)dst;
    while (n--) *d++ = (unsigned char)val;
    return dst;
}