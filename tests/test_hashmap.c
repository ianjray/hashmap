#include "hashmap.h"

#include "memory_shim.h"

#include <assert.h>
#include <errno.h>
#include <float.h>
#include <math.h>
#include <stdint.h>
#include <string.h>

static void test_hashmap_new(void)
{
    for (unsigned n = 1; n <= 3; n++) {
        memory_shim_fail_at(n);
        assert(NULL == hashmap_new(sizeof(int)));
    }

    memory_shim_reset();
}

static void test_hashmap_delete(void)
{
    hashmap_delete(NULL);
}

static void test_hashmap_empty(void)
{
    struct hashmap *h;

    assert(hashmap_empty(NULL));

    h = hashmap_new(sizeof(int));

    assert(hashmap_empty(h));

    hashmap_insert(h, "key");
    assert(!hashmap_empty(h));

    hashmap_clear(h);
    assert(hashmap_empty(h));

    hashmap_delete(h);
}

static void test_hashmap_size(void)
{
    struct hashmap *h;

    assert(0 == hashmap_size(NULL));

    h = hashmap_new(sizeof(int));

    assert(0 == hashmap_size(h));

    hashmap_insert(h, "a");
    assert(1 == hashmap_size(h));
    hashmap_insert(h, "aa");
    assert(2 == hashmap_size(h));

    hashmap_clear(h);
    assert(0 == hashmap_size(h));

    hashmap_delete(h);
}

static void test_hashmap_reserve(void)
{
    struct hashmap *h;
    float f;

    errno = 0;
    assert(-1 == hashmap_reserve(NULL, 42));
    assert(EINVAL == errno);

    h = hashmap_new(sizeof(int));

    errno = 0;
    assert(-1 == hashmap_reserve(h, SIZE_MAX));
    assert(EOVERFLOW == errno);

    errno = 0;
    assert(-1 == hashmap_reserve(h, BUCKET_COUNT_MAXIMUM));
    assert(EOVERFLOW == errno);

    assert(5 == hashmap_bucket_count(h));
    f = fabsf(1 - hashmap_max_load_factor(h));
    assert(f <= FLT_MIN);

    assert(0 == hashmap_reserve(h, 42));
    // 47 is the next prime higher than 42.
    assert(47 == hashmap_bucket_count(h));

    assert(0 == hashmap_reserve(h, 5));
    // Reserve only expands.
    assert(47 == hashmap_bucket_count(h));

    hashmap_delete(h);
}

static void test_hashmap_clear(void)
{
    errno = 0;
    assert(-1 == hashmap_clear(NULL));
    assert(EINVAL == errno);
}

static void test_hashmap_bucket_count(void)
{
    assert(0 == hashmap_bucket_count(NULL));
}

static void test_hashmap_max_bucket_count(void)
{
    assert(0 == hashmap_max_bucket_count(NULL));
}

static void test_hashmap_bucket_size(void)
{
    struct hashmap *h;

    assert(0 == hashmap_bucket_size(NULL, 0));
    assert(0 == hashmap_bucket_size(NULL, 1));

    h = hashmap_new(sizeof(int));

    assert(0 == hashmap_bucket_size(h, SIZE_MAX));

    assert(0 == hashmap_bucket_size(h, 0));
    assert(0 == hashmap_bucket_size(h, 1));
    assert(0 == hashmap_bucket_size(h, 5));
    assert(0 == hashmap_bucket_size(h, 6));

    hashmap_insert(h, "Au");
    hashmap_insert(h, "Ag");
    assert(1 == hashmap_bucket_size(h, (size_t)hashmap_bucket(h, "Au")));

    hashmap_delete(h);
}

static void test_hashmap_bucket(void)
{
    struct hashmap *h;

    errno = 0;
    assert(-1 == hashmap_bucket(NULL, "foo"));
    assert(EINVAL == errno);

    errno = 0;
    assert(-1 == hashmap_bucket(NULL, NULL));
    assert(EINVAL == errno);

    h = hashmap_new(sizeof(int));

    errno = 0;
    assert(-1 == hashmap_bucket(h, NULL));
    assert(EINVAL == errno);

    hashmap_delete(h);
}

static void test_hashmap_load_factor(void)
{
    struct hashmap *h;
    float f;

    f = fabsf(0 - hashmap_load_factor(NULL));
    assert(f <= FLT_MIN);

    h = hashmap_new(sizeof(int));

    hashmap_insert(h, "Au");
    hashmap_insert(h, "Ag");
    hashmap_insert(h, "Cu");
    hashmap_insert(h, "Pt");

    assert(1 == hashmap_bucket(h, "Au"));
    assert(2 == hashmap_bucket(h, "Ag"));
    assert(2 == hashmap_bucket(h, "Cu"));
    assert(0 == hashmap_bucket(h, "Pt"));
    assert(4 == hashmap_size(h));
    assert(5 == hashmap_bucket_count(h));

    f = fabsf(0.8f - hashmap_load_factor(h));
    assert(f <= FLT_MIN);

    hashmap_max_load_factor_set(h, hashmap_max_load_factor(h) / 2.f);

    f = fabsf(0.5f - hashmap_max_load_factor(h));
    assert(f <= 0.000001f);

    assert(7 == hashmap_bucket(h, "Au"));
    assert(4 == hashmap_bucket(h, "Ag"));
    assert(7 == hashmap_bucket(h, "Cu"));
    assert(6 == hashmap_bucket(h, "Pt"));
    assert(4 == hashmap_size(h));
    assert(11 == hashmap_bucket_count(h));

    f = fabsf(0.363636f - hashmap_load_factor(h));
    assert(f <= 0.000001f);
    hashmap_delete(h);
}

static void test_hashmap_max_load_factor(void)
{
    float f;
    f = fabsf(0 - hashmap_max_load_factor(NULL));
    assert(f <= FLT_MIN);
}

static void test_hashmap_max_load_factor_set(void)
{
    struct hashmap *h;
    float f;

    errno = 0;
    assert(-1 == hashmap_max_load_factor_set(NULL, 1));
    assert(EINVAL == errno);

    h = hashmap_new(sizeof(int));

    f = fabsf(1.0f - hashmap_max_load_factor(h));
    assert(f <= FLT_MIN);

    assert(0 == hashmap_max_load_factor_set(h, 4));

    f = fabsf(4 - hashmap_max_load_factor(h));
    assert(f <= FLT_MIN);

    assert(0 == hashmap_max_load_factor_set(h, 0));
    f = fabsf(0.25f - hashmap_max_load_factor(h));
    assert(f <= FLT_MIN);

    hashmap_delete(h);
}

static void test_hashmap_begin(void)
{
    assert(NULL == hashmap_begin(NULL));
}

static void test_hashmap_iterator(void)
{
    struct hashmap *h;
    struct hashmap_insert_result ret;
    int calls;

    h = hashmap_new(sizeof(char));

    ret = hashmap_insert(h, "a");
    *(char *)ret.pair->userdata = 0;

    ret = hashmap_insert(h, "b");
    *(char *)ret.pair->userdata = 1;

    ret = hashmap_insert(h, "c");
    *(char *)ret.pair->userdata = 2;

    calls = 0;
    for (struct hashmap_iter *it = hashmap_begin(h); it != hashmap_end(h); it = hashmap_advance(it, 1)) {
        struct hashmap_pair *pair = hashmap_at(it);
        int i = *(char *)pair->userdata;
        assert(i == calls);
        calls++;
    }
    assert(calls == 3);

    hashmap_delete(h);
}

static void test_hashmap_iterator_const(void)
{
    struct hashmap *h;
    struct hashmap_insert_result ret;
    const struct hashmap *ch;
    int calls;

    h = hashmap_new(sizeof(char));

    ret = hashmap_insert(h, "a");
    *(char *)ret.pair->userdata = 0;

    ret = hashmap_insert(h, "b");
    *(char *)ret.pair->userdata = 1;

    ret = hashmap_insert(h, "c");
    *(char *)ret.pair->userdata = 2;

    ch = h;

    calls = 0;
    for (const struct hashmap_iter *it = hashmap_cbegin(ch); it != hashmap_cend(ch); it = hashmap_advance_const(it, 1)) {
        const struct hashmap_pair *pair = hashmap_at_const(it);
        const int i = *(const char *)pair->userdata;
        assert(i == calls);
        calls++;
    }
    assert(calls == 3);

    hashmap_delete(h);
}

static void test_hashmap_at(void)
{
    assert(NULL == hashmap_at(NULL));
}

static void test_hashmap_insert(void)
{
    struct hashmap *h;
    struct hashmap_insert_result ret;

    ret = hashmap_insert(NULL, "e");
    assert(!ret.inserted);
    assert(NULL == ret.pair);

    h = hashmap_new(SIZE_MAX);
    assert(h);

    errno = 0;
    ret = hashmap_insert(h, "a");
    assert(!ret.inserted);
    assert(EOVERFLOW == errno);

    errno = 0;
    assert(-1 == hashmap_rehash(NULL, 42));
    assert(EINVAL == errno);

    hashmap_delete(h);

    for (unsigned n = 1; n <= 3; ++n) {
        h = hashmap_new(sizeof(int));
        hashmap_insert(h, "a");
        hashmap_insert(h, "b");
        hashmap_insert(h, "c");
        hashmap_insert(h, "d");
        hashmap_insert(h, "e");

        memory_shim_fail_at(n);
        ret = hashmap_insert(h, "f");
        assert(!ret.inserted);
        assert(NULL == ret.pair);

        memory_shim_reset();
        hashmap_delete(h);
    }

    h = hashmap_new(sizeof(int));

    hashmap_insert(h, "a");
    hashmap_insert(h, "b");
    hashmap_insert(h, "c");
    hashmap_insert(h, "d");

    ret = hashmap_insert(h, "e");
    assert(ret.inserted);

    ret = hashmap_insert(h, "e");
    assert(!ret.inserted);

    ret = hashmap_insert(h, "f");
    assert(ret.inserted);

    ret = hashmap_insert(h, "g");
    assert(ret.inserted);
    ret = hashmap_insert(h, "h");
    assert(ret.inserted);

    errno = 0;
    ret = hashmap_insert(h, "i");
    assert(!ret.inserted);
    assert(EOVERFLOW == errno);

    hashmap_delete(h);
}

static void test_hashmap_find(void)
{
    struct hashmap *h;

    assert(hashmap_end(NULL) == hashmap_find(NULL, NULL));
    assert(hashmap_end(NULL) == hashmap_find(NULL, "e"));

    h = hashmap_new(sizeof(int));

    assert(hashmap_end(h) == hashmap_find(h, NULL));
    assert(hashmap_end(h) == hashmap_find(h, "e"));

    hashmap_insert(h, "e");

    assert(hashmap_end(h) != hashmap_find(h, "e"));
    // 'j' has the same hash as 'e', but is not in the map and should not be found.
    assert(hashmap_end(h) == hashmap_find(h, "j"));

    hashmap_delete(h);
}

static void test_hashmap_cfind(void)
{
    struct hashmap *h;
    const struct hashmap *ch;

    assert(hashmap_cend(NULL) == hashmap_cfind(NULL, NULL));
    assert(hashmap_cend(NULL) == hashmap_cfind(NULL, "e"));

    h = hashmap_new(sizeof(int));
    ch = h;

    assert(hashmap_cend(ch) == hashmap_cfind(ch, NULL));
    assert(hashmap_cend(ch) == hashmap_cfind(ch, "e"));

    hashmap_delete(h);
}

static void test_hashmap_erase(void)
{
    struct hashmap *h;

    errno = 0;
    assert(-1 == hashmap_erase(NULL, NULL));
    assert(EINVAL == errno);

    h = hashmap_new(sizeof(int));

    errno = 0;
    assert(-1 == hashmap_erase(h, hashmap_begin(h)));
    assert(EINVAL == errno);

    errno = 0;
    assert(-1 == hashmap_erase(h, hashmap_end(h)));
    assert(EINVAL == errno);

    hashmap_insert(h, "a");
    assert(0 == hashmap_erase(h, hashmap_begin(h)));

    errno = 0;
    assert(-1 == hashmap_erase(h, hashmap_begin(h)));
    assert(EINVAL == errno);

    hashmap_insert(h, "a");
    hashmap_insert(h, "f");
    assert(0 == hashmap_erase(h, hashmap_begin(h)));

    hashmap_delete(h);
}

static void test_hashmap_rehash(void)
{
    struct hashmap *h;

    errno = 0;
    assert(-1 == hashmap_rehash(NULL, 42));
    assert(EINVAL == errno);

    h = hashmap_new(sizeof(int));

    memory_shim_fail_at(1);
    errno = 0;
    assert(-1 == hashmap_rehash(h, 42));
    assert(ENOMEM == errno);
    memory_shim_reset();

    assert(5 == hashmap_bucket_count(h));

    assert(0 == hashmap_rehash(h, 3));
    assert(5 == hashmap_bucket_count(h));

    assert(0 == hashmap_rehash(h, 42));
    assert(42 == hashmap_bucket_count(h));

    hashmap_delete(h);
}

static void test_overflow(void)
{
    struct hashmap *h;
    struct hashmap_insert_result ret;

    // Insufficient space for 'struct entry' header.
    h = hashmap_new(SIZE_MAX);
    assert(h);

    errno = 0;
    ret = hashmap_insert(h, "a");
    assert(!ret.inserted);
    assert(EOVERFLOW == errno);

    errno = 0;
    // Insufficient space to store SIZE_MAX/3 buckets.
    assert(-1 == hashmap_rehash(h, SIZE_MAX / 3));
    assert(EOVERFLOW == errno);

    hashmap_delete(h);
}

int main(void)
{
    test_hashmap_new();
    test_hashmap_delete();
    test_hashmap_empty();
    test_hashmap_size();
    test_hashmap_reserve();
    test_hashmap_clear();
    test_hashmap_bucket_count();
    test_hashmap_max_bucket_count();
    test_hashmap_bucket_size();
    test_hashmap_bucket();
    test_hashmap_load_factor();
    test_hashmap_max_load_factor();
    test_hashmap_max_load_factor_set();
    test_hashmap_begin();
    test_hashmap_iterator();
    test_hashmap_iterator_const();
    test_hashmap_at();
    test_hashmap_insert();
    test_hashmap_find();
    test_hashmap_cfind();
    test_hashmap_erase();
    test_hashmap_rehash();
    test_overflow();
    return 0;
}
