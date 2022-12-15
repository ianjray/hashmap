#include "memory_shim.h"

#include <dlfcn.h>
#include <errno.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

bool g_malloc_fail;
bool g_calloc_fail;
bool g_strdup_fail;
bool g_strndup_fail;
bool g_realloc_fail;

static void * (*g_libc_malloc)(size_t);
static void * (*g_libc_calloc)(size_t, size_t);
static char * (*g_libc_strdup)(const char *);
static char * (*g_libc_strndup)(const char *, size_t);
static void * (*g_libc_realloc)(void *, size_t);

/// Shim for simulating out-of-memory.
void *malloc(size_t size)
{
    if (!g_libc_malloc) {
        g_libc_malloc = (void * (*)(size_t))dlsym(RTLD_NEXT, "malloc");
    }

    if (!g_malloc_fail) {
        return g_libc_malloc(size);
    }

    errno = ENOMEM;
    return NULL;
}

/// Shim for simulating out-of-memory.
void *calloc(size_t count, size_t size)
{
    if (!g_libc_calloc) {
        g_libc_calloc = (void * (*)(size_t, size_t))dlsym(RTLD_NEXT, "calloc");
    }

    if (!g_calloc_fail) {
        return g_libc_calloc(count, size);
    }

    errno = ENOMEM;
    return NULL;
}

/// Shim for simulating out-of-memory.
char *strdup(const char *str)
{
    if (!g_libc_strdup) {
        g_libc_strdup = (char * (*)(const char *))dlsym(RTLD_NEXT, "strdup");
    }

    if (!g_strdup_fail) {
        return g_libc_strdup(str);
    }

    errno = ENOMEM;
    return NULL;
}

/// Shim for simulating out-of-memory.
char *strndup(const char *str, size_t n)
{
    if (!g_libc_strndup) {
        g_libc_strndup = (char * (*)(const char *, size_t))dlsym(RTLD_NEXT, "strndup");
    }

    if (!g_strndup_fail) {
        return g_libc_strndup(str, n);
    }

    errno = ENOMEM;
    return NULL;
}

/// Shim for simulating out-of-memory.
void *realloc(void *ptr, size_t size)
{
    if (!g_libc_realloc) {
        g_libc_realloc = (void * (*)(void *, size_t))dlsym(RTLD_NEXT, "realloc");
    }

    if (!g_realloc_fail) {
        return g_libc_realloc(ptr, size);
    }

    errno = ENOMEM;
    return NULL;
}

#if 0

/// Shim for simulating out-of-memory.
char *strdup(const char *str)
{
    char *p = malloc(strlen(str) + 1);
    if (p) {
        strcpy(p, str);
    }
    return p;
}

#endif
