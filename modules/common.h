#pragma once

static inline int pStrcmp(const char* s1, const char* s2) {
    while(*s1 && (*s1 == *s2)) {
    	s1++;
    	s2++;
    }
    return (unsigned char)*s1 - (unsigned char)*s2;
}

static inline SIZE_T mStrLen(const char *str) {
    const volatile char *s = (const volatile char *)str;
    SIZE_T n = 0;
    while (*s++) n++;
    return n;
}