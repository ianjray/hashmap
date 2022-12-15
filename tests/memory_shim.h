#include <stdbool.h>

/// True if @c malloc should fail.
extern bool g_malloc_fail;

/// True if @c calloc should fail.
extern bool g_calloc_fail;

/// True if @c strdup should fail.
extern bool g_strdup_fail;

/// True if @c strndup should fail.
extern bool g_strndup_fail;

/// True if @c realloc should fail.
extern bool g_realloc_fail;
