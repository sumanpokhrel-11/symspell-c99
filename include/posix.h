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

//#define DEBUG

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdint.h>
#include <limits.h>

#define GETLINE_BUFFER_SIZE 4096

/* ============================================================================
 * Platform Detection
 * ============================================================================ */

#if defined(__aarch64__) || defined(__arm64__)
    #define PLATFORM_ARM64
#elif defined(__x86_64__) || defined(__i386__)
    #define PLATFORM_X86
#endif

/* Include POSIX strings.h early if available */
#if defined(_POSIX_C_SOURCE)
    #include <strings.h>
#endif

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
 * Case-Insensitive String Comparison (Platform-Optimized)
 * ============================================================================ */

// #if defined(PLATFORM_ARM64) && !defined(DEBUG)
#if defined(PLATFORM_ARM64) && !defined(DEBUG)


/**
 * c_strcasecmp - Compare two strings ignoring case (ARM64 optimized)
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

    int result;

    __asm__ volatile (
        "mov w3, #0x20\n\t"              // Lowercase mask in w3

        "1:\n\t"                         // Main loop
        "ldrb w4, [%[s1]], #1\n\t"       // Load *s1++
        "ldrb w5, [%[s2]], #1\n\t"       // Load *s2++

        // Fast path: equal check
        "cmp w4, w5\n\t"
        "b.eq 3f\n\t"                    // If equal, check for null

        // Lowercase s1 if in [A-Z]
        "sub w6, w4, #'A'\n\t"           // c - 'A'
        "cmp w6, #25\n\t"                // Is it <= 25?
        "b.hi 2f\n\t"                    // No: skip
        "orr w4, w4, w3\n\t"             // Yes: c |= 0x20

        "2:\n\t"
        // Lowercase s2 if in [A-Z]
        "sub w6, w5, #'A'\n\t"
        "cmp w6, #25\n\t"
        "b.hi 3f\n\t"
        "orr w5, w5, w3\n\t"

        "3:\n\t"
        "cmp w4, w5\n\t"                 // Compare lowercased chars
        "b.ne 4f\n\t"                    // Not equal? Exit
        "cbnz w4, 1b\n\t"                // Not null? Continue loop

        // Equal strings
        "mov %w0, #0\n\t"
        "b 5f\n\t"

        "4:\n\t"                         // Not equal
        "sub %w0, w4, w5\n\t"             // Return difference

        "5:\n\t"
        : "=r" (result), [s1] "+r" (s1), [s2] "+r" (s2)
        :
        : "w3", "w4", "w5", "w6", "cc", "memory"
    );

    return result;
}

/**
 * c_strncasecmp - Compare at most n bytes ignoring case (ARM64 optimized)
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

    int result;
    size_t count = n;

    __asm__ volatile (
        "mov w3, #0x20\n\t"              // Lowercase mask

        "1:\n\t"                         // Main loop
        "cbz %[count], 6f\n\t"           // If count==0, equal
        "ldrb w4, [%[s1]], #1\n\t"       // Load *s1++
        "ldrb w5, [%[s2]], #1\n\t"       // Load *s2++
        "sub %[count], %[count], #1\n\t" // count--

        // Fast path: equal check
        "cmp w4, w5\n\t"
        "b.eq 3f\n\t"

        // Lowercase s1 if in [A-Z]
        "sub w6, w4, #'A'\n\t"
        "cmp w6, #25\n\t"
        "b.hi 2f\n\t"
        "orr w4, w4, w3\n\t"

        "2:\n\t"
        // Lowercase s2 if in [A-Z]
        "sub w6, w5, #'A'\n\t"
        "cmp w6, #25\n\t"
        "b.hi 3f\n\t"
        "orr w5, w5, w3\n\t"

        "3:\n\t"
        "cmp w4, w5\n\t"                 // Compare lowercased
        "b.ne 4f\n\t"                    // Not equal? Exit
        "cbnz w4, 1b\n\t"                // Not null? Continue

        // Equal strings (or both null)
        "6:\n\t"
        "mov %w0, #0\n\t"
        "b 5f\n\t"

        "4:\n\t"                         // Not equal
        "sub %w0, w4, w5\n\t"

        "5:\n\t"
        : "=r" (result), [s1] "+r" (s1), [s2] "+r" (s2), [count] "+r" (count)
        :
        : "w3", "w4", "w5", "w6", "cc", "memory"
    );

    return result;
}

#elif defined(PLATFORM_X86) && defined(_POSIX_C_SOURCE) && !defined(DEBUG)

/**
 * c_strcasecmp - Use native glibc implementation (x86 POSIX passthrough)
 * @s1: First string
 * @s2: Second string
 *
 * Returns: <0 if s1 < s2, 0 if s1 == s2, >0 if s1 > s2
 */
static inline int c_strcasecmp(const char* s1, const char* s2) {
    return strcasecmp(s1, s2);
}

/**
 * c_strncasecmp - Use native glibc implementation (x86 POSIX passthrough)
 * @s1: First string
 * @s2: Second string
 * @n: Maximum number of bytes to compare
 *
 * Returns: <0 if s1 < s2, 0 if s1 == s2, >0 if s1 > s2
 */
static inline int c_strncasecmp(const char* s1, const char* s2, size_t n) {
    return strncasecmp(s1, s2, n);
}

#else

/**
 * c_strcasecmp - Compare two strings ignoring case (portable C99)
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
 * c_strncasecmp - Compare at most n bytes ignoring case (portable C99)
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

#endif

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
#endif
}

/* ============================================================================
 * Line-Oriented Input
 * ============================================================================ */

static inline ssize_t c_getline(char** lineptr, size_t* n, FILE* stream) {
#if defined(_POSIX_C_SOURCE)
    return getline(lineptr, n, stream);
#else
    if (!lineptr || !n || !stream) return -1;
    
    if (*lineptr == NULL || *n == 0) {
        *n = 128;
        *lineptr = malloc(*n);
        if (!*lineptr) return -1;
    }
    
    /* Static buffers allocated once, reused forever */
    static char* buffer1 = NULL;
    static char* buffer2 = NULL;
    
    /* Per-stream state - need to track which stream we're reading */
    static FILE* last_stream = NULL;
    static char* current_buf = NULL;
    static size_t buf_pos = 0;
    static size_t buf_len = 0;
    static int current_buffer = 0;
    
    /* Allocate buffers on first use */
    if (!buffer1) {
        buffer1 = malloc(GETLINE_BUFFER_SIZE);
        buffer2 = malloc(GETLINE_BUFFER_SIZE);
        if (!buffer1 || !buffer2) return -1;
    }
    
    /* Stream changed? Reset state */
    if (stream != last_stream) {
        last_stream = stream;
        buf_pos = 0;
        buf_len = 0;
    }
    
    size_t line_pos = 0;
    
    while (1) {
        if (buf_pos >= buf_len) {
            current_buffer = !current_buffer;
            current_buf = current_buffer ? buffer2 : buffer1;
            buf_len = fread(current_buf, 1, GETLINE_BUFFER_SIZE, stream);
            buf_pos = 0;
            
            if (buf_len == 0) {
                if (line_pos == 0) return -1;
                (*lineptr)[line_pos] = '\0';
                return line_pos;
            }
        }
        
        while (buf_pos < buf_len) {
            char c = current_buf[buf_pos++];
            
            if (line_pos + 1 >= *n) {
                size_t new_size = *n * 2;
                char* new_ptr = realloc(*lineptr, new_size);
                if (!new_ptr) return -1;
                *lineptr = new_ptr;
                *n = new_size;
            }
            
            (*lineptr)[line_pos++] = c;
            
            if (c == '\n') {
                (*lineptr)[line_pos] = '\0';
                return line_pos;
            }
        }
    }
#endif
}

/* ============================================================================
 * Additional Utilities
 * ============================================================================ */

/**
 * c_strnlen - Get length of string with maximum
 * @s: String to measure
 * @maxlen: Maximum length to check
 *
 * Returns: Length of string or maxlen, whichever is less
 */
//static inline size_t c_strnlen(const char* s, size_t maxlen) {
//    if (!s) return 0;
//    const char* start = s;
//    const char* max = s + maxlen;
//    while (s < max && *s != '\0') {
//        s++;
//    }
//    return s - start;
//}

static inline size_t c_strnlen(const char* s, size_t maxlen) {
    if (!s) return 0;  // Add this line for NULL safety
    size_t count = 0;
    // Iterate byte by byte until null terminator or maxlen is reached
    while (count < maxlen && s[count] != '\0') {
        count++;
    }
    return count;
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

/* POSIX_COMPAT_H */
