/*
 * posix.h - Portable POSIX-like Functions for C99
 * 
 * This header provides C99-compatible implementations of commonly-used POSIX
 * functions that are not part of the C standard library. All functions are
 * prefixed with 'c_' to avoid naming conflicts.
 *
 * DESIGN PRINCIPLES:
 * - Pure C99 with no platform-specific code
 * - Drop-in replacements for POSIX functions
 * - Consistent error handling and behavior
 * - Header-only for ease of use
 *
 * Copyright (c) 2025 CGIOS Project
 * SPDX-License-Identifier: MIT
 *
 * ============================================================================
 * NAME
 *     c_strdup, c_strndup, c_strcasecmp, c_strncasecmp, c_strsep - portable
 *     string utilities
 *
 * SYNOPSIS
 *     #include "posix.h"
 *
 *     char *c_strdup(const char *s);
 *     char *c_strndup(const char *s, size_t n);
 *     int c_strcasecmp(const char *s1, const char *s2);
 *     int c_strncasecmp(const char *s1, const char *s2, size_t n);
 *     char *c_strsep(char **stringp, const char *delim);
 *     void *c_reallocarray(void *ptr, size_t nmemb, size_t size);
 *     int c_asprintf(char **strp, const char *fmt, ...);
 *     ssize_t c_getline(char **lineptr, size_t *n, FILE *stream);
 *
 * DESCRIPTION
 *     These functions provide portable C99 implementations of common POSIX
 *     string and memory utilities.
 *
 *     c_strdup() returns a pointer to a new string which is a duplicate of
 *     the string s. Memory for the new string is obtained with malloc(3),
 *     and can be freed with free(3).
 *
 *     c_strndup() is similar, but copies at most n bytes. If s is longer
 *     than n, only n bytes are copied, and a terminating null byte is added.
 *
 *     c_strcasecmp() compares the two strings s1 and s2, ignoring the case
 *     of the characters. It returns an integer less than, equal to, or
 *     greater than zero if s1 is found to be less than, to match, or be
 *     greater than s2.
 *
 *     c_strncasecmp() is similar, except it compares only the first n bytes.
 *
 *     c_strsep() locates the first occurrence in *stringp of any character
 *     in delim and replaces it with '\0'. The location of the next character
 *     after the delimiter is stored in *stringp. Returns original *stringp.
 *
 *     c_reallocarray() changes the size of the memory block pointed to by
 *     ptr to be large enough for an array of nmemb elements of size bytes.
 *     Unlike realloc, it checks for overflow in the multiplication.
 *
 *     c_asprintf() formats and allocates a string, storing the pointer in
 *     *strp. The caller must free the string with free(3).
 *
 *     c_getline() reads an entire line from stream, storing the address of
 *     the buffer in *lineptr and the size in *n. The buffer is automatically
 *     resized if needed.
 *
 * RETURN VALUE
 *     c_strdup() and c_strndup() return a pointer to the duplicated string,
 *     or NULL if insufficient memory was available.
 *
 *     c_strcasecmp() and c_strncasecmp() return an integer less than, equal
 *     to, or greater than zero if s1 is found to be less than, to match, or
 *     be greater than s2.
 *
 *     c_strsep() returns a pointer to the token, or NULL if no token found.
 *
 *     c_reallocarray() returns a pointer to the newly allocated memory, or
 *     NULL on failure.
 *
 *     c_asprintf() returns the number of characters printed (excluding null
 *     byte), or -1 on error.
 *
 *     c_getline() returns the number of characters read (including newline
 *     but excluding null byte), or -1 on error or EOF.
 *
 * NOTES
 *     All allocation functions use malloc() and can be freed with free().
 *     These functions are thread-safe if the underlying C library functions
 *     (malloc, strcpy, etc.) are thread-safe.
 *
 * EXAMPLES
 *     Duplicate a string:
 *         char *copy = c_strdup("hello");
 *         if (copy) {
 *             printf("%s\n", copy);
 *             free(copy);
 *         }
 *
 *     Case-insensitive comparison:
 *         if (c_strcasecmp("Hello", "HELLO") == 0) {
 *             printf("Strings match!\n");
 *         }
 *
 *     Parse a string:
 *         char *str = c_strdup("one,two,three");
 *         char *token;
 *         while ((token = c_strsep(&str, ",")) != NULL) {
 *             printf("Token: %s\n", token);
 *         }
 *         free(str);
 *
 * SEE ALSO
 *     strdup(3), strcasecmp(3), strsep(3), getline(3)
 * ============================================================================
 */

#ifndef POSIX_COMPAT_H
#define POSIX_COMPAT_H

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdint.h>
#include <limits.h>

/* Define ssize_t if not available (Windows) */
#if defined(_WIN32) && !defined(__CYGWIN__)
    #include <BaseTsd.h>
    typedef SSIZE_T ssize_t;
#elif !defined(_SSIZE_T_DEFINED) && !defined(_SSIZE_T)
    #if defined(__LP64__) || defined(_WIN64)
        typedef long ssize_t;
    #else
        typedef int ssize_t;
    #endif
#endif

/* ============================================================================
 * String Duplication
 * ============================================================================ */

/**
 * c_strdup - Duplicate a string
 * @s: String to duplicate
 *
 * Returns: Pointer to duplicated string, or NULL on failure
 */
static inline char* c_strdup(const char* s) {
    if (!s) return NULL;
    
    size_t len = strlen(s) + 1;
    char* new_str = malloc(len);
    if (!new_str) return NULL;
    
    memcpy(new_str, s, len);
    return new_str;
}

/**
 * c_strndup - Duplicate at most n bytes of a string
 * @s: String to duplicate
 * @n: Maximum number of bytes to copy
 *
 * Returns: Pointer to duplicated string, or NULL on failure
 */
static inline char* c_strndup(const char* s, size_t n) {
    if (!s) return NULL;
    
    size_t len = strlen(s);
    if (len > n) len = n;
    
    char* new_str = malloc(len + 1);
    if (!new_str) return NULL;
    
    memcpy(new_str, s, len);
    new_str[len] = '\0';
    return new_str;
}

/* ============================================================================
 * Case-Insensitive String Comparison
 * ============================================================================ */

/**
 * c_strcasecmp - Compare two strings ignoring case
 * @s1: First string
 * @s2: Second string
 *
 * Returns: <0 if s1 < s2, 0 if s1 == s2, >0 if s1 > s2
 */
static inline int c_strcasecmp(const char* s1, const char* s2) {
    if (!s1 || !s2) {
        if (s1 == s2) return 0;
        return s1 ? 1 : -1;
    }
    
    while (*s1 && *s2) {
        int c1 = tolower((unsigned char)*s1);
        int c2 = tolower((unsigned char)*s2);
        if (c1 != c2) return c1 - c2;
        s1++;
        s2++;
    }
    
    return tolower((unsigned char)*s1) - tolower((unsigned char)*s2);
}

/**
 * c_strncasecmp - Compare at most n bytes of two strings ignoring case
 * @s1: First string
 * @s2: Second string
 * @n: Maximum number of bytes to compare
 *
 * Returns: <0 if s1 < s2, 0 if s1 == s2, >0 if s1 > s2
 */
static inline int c_strncasecmp(const char* s1, const char* s2, size_t n) {
    if (!s1 || !s2 || n == 0) {
        if (s1 == s2 || n == 0) return 0;
        return s1 ? 1 : -1;
    }
    
    while (n > 0 && *s1 && *s2) {
        int c1 = tolower((unsigned char)*s1);
        int c2 = tolower((unsigned char)*s2);
        if (c1 != c2) return c1 - c2;
        s1++;
        s2++;
        n--;
    }
    
    if (n == 0) return 0;
    return tolower((unsigned char)*s1) - tolower((unsigned char)*s2);
}

/* ============================================================================
 * String Tokenization
 * ============================================================================ */

/**
 * c_strsep - Extract token from string
 * @stringp: Pointer to string pointer
 * @delim: Delimiter characters
 *
 * Returns: Pointer to token, or NULL if no more tokens
 */
static inline char* c_strsep(char** stringp, const char* delim) {
    if (!stringp || !*stringp) return NULL;
    
    char* start = *stringp;
    char* end = start + strcspn(start, delim);
    
    if (*end) {
        *end = '\0';
        *stringp = end + 1;
    } else {
        *stringp = NULL;
    }
    
    return start;
}

/* ============================================================================
 * Safe Memory Allocation
 * ============================================================================ */

/**
 * c_reallocarray - Reallocate array with overflow checking
 * @ptr: Pointer to existing memory or NULL
 * @nmemb: Number of elements
 * @size: Size of each element
 *
 * Returns: Pointer to reallocated memory, or NULL on failure
 */
static inline void* c_reallocarray(void* ptr, size_t nmemb, size_t size) {
    /* Check for multiplication overflow */
    if (nmemb > 0 && size > SIZE_MAX / nmemb) {
        return NULL;
    }
    
    return realloc(ptr, nmemb * size);
}

/* ============================================================================
 * Formatted String Allocation
 * ============================================================================ */

/**
 * c_asprintf - Allocate and format a string
 * @strp: Pointer to store allocated string
 * @fmt: Format string (printf-style)
 * @...: Format arguments
 *
 * Returns: Number of characters printed (excluding null), or -1 on error
 */
static inline int c_asprintf(char** strp, const char* fmt, ...) {
    if (!strp || !fmt) return -1;
    
    va_list ap, ap_copy;
    va_start(ap, fmt);
    va_copy(ap_copy, ap);
    
    /* Calculate required size */
    int size = vsnprintf(NULL, 0, fmt, ap);
    va_end(ap);
    
    if (size < 0) {
        va_end(ap_copy);
        return -1;
    }
    
    /* Allocate buffer */
    *strp = malloc(size + 1);
    if (!*strp) {
        va_end(ap_copy);
        return -1;
    }
    
    /* Format string */
    int result = vsnprintf(*strp, size + 1, fmt, ap_copy);
    va_end(ap_copy);
    
    return result;
}

/* ============================================================================
 * Line-Oriented Input
 * ============================================================================ */

/**
 * c_getline - Read a line from a stream
 * @lineptr: Pointer to buffer pointer
 * @n: Pointer to buffer size
 * @stream: Input stream
 *
 * Returns: Number of characters read (including newline), or -1 on error/EOF
 */
static inline ssize_t c_getline(char** lineptr, size_t* n, FILE* stream) {
    if (!lineptr || !n || !stream) return -1;
    
    /* Allocate initial buffer if needed */
    if (*lineptr == NULL || *n == 0) {
        *n = 128;
        *lineptr = malloc(*n);
        if (!*lineptr) return -1;
    }
    
    size_t pos = 0;
    int c;
    
    while ((c = fgetc(stream)) != EOF) {
        /* Resize buffer if needed */
        if (pos + 1 >= *n) {
            size_t new_size = *n * 2;
            char* new_ptr = realloc(*lineptr, new_size);
            if (!new_ptr) return -1;
            *lineptr = new_ptr;
            *n = new_size;
        }
        
        (*lineptr)[pos++] = c;
        
        if (c == '\n') break;
    }
    
    if (pos == 0 && c == EOF) return -1;
    
    (*lineptr)[pos] = '\0';
    return pos;
}

/* ============================================================================
 * Additional Utilities
 * ============================================================================ */

/**
 * c_strnlen - Get length of string with maximum
 * @s: String to measure
 * @maxlen: Maximum length to check
 *
 * Returns: Length of string, or maxlen if no null byte found
 */
static inline size_t c_strnlen(const char* s, size_t maxlen) {
    if (!s) return 0;
    
    const char* end = memchr(s, '\0', maxlen);
    return end ? (size_t)(end - s) : maxlen;
}

/**
 * c_strlcpy - Size-bounded string copy
 * @dst: Destination buffer
 * @src: Source string
 * @size: Size of destination buffer
 *
 * Returns: Total length of string attempted to create (strlen(src))
 */
static inline size_t c_strlcpy(char* dst, const char* src, size_t size) {
    if (!dst || !src) return 0;
    
    size_t src_len = strlen(src);
    
    if (size > 0) {
        size_t copy_len = (src_len >= size) ? size - 1 : src_len;
        memcpy(dst, src, copy_len);
        dst[copy_len] = '\0';
    }
    
    return src_len;
}

/**
 * c_strlcat - Size-bounded string concatenation
 * @dst: Destination buffer
 * @src: Source string
 * @size: Size of destination buffer
 *
 * Returns: Total length of string attempted to create
 */
static inline size_t c_strlcat(char* dst, const char* src, size_t size) {
    if (!dst || !src) return 0;
    
    size_t dst_len = c_strnlen(dst, size);
    size_t src_len = strlen(src);
    
    if (dst_len == size) return size + src_len;
    
    size_t copy_len = size - dst_len - 1;
    if (src_len < copy_len) {
        copy_len = src_len;
    }
    
    memcpy(dst + dst_len, src, copy_len);
    dst[dst_len + copy_len] = '\0';
    
    return dst_len + src_len;
}

#endif /* POSIX_COMPAT_H */
