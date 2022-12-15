#include "hashmap.h"

#include "memory_shim.h"

#include <assert.h>
#include <errno.h>
#include <float.h>
#include <math.h>
#include <string.h>

static void test_hashmap_new(void)
{
    g_calloc_fail = true;
    assert(NULL == hashmap_new(sizeof(int)));
    g_calloc_fail = false;

    g_realloc_fail = true;
    assert(NULL == hashmap_new(sizeof(int)));
    g_realloc_fail = false;
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

    assert(-EFAULT == hashmap_reserve(NULL, 42));

    h = hashmap_new(sizeof(int));

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
    assert(-EFAULT == hashmap_clear(NULL));
}

static void test_hashmap_bucket_count(void)
{
    assert(0 == hashmap_bucket_count(NULL));
}

static void test_hashmap_bucket_size(void)
{
    struct hashmap *h;

    assert(0 == hashmap_bucket_size(NULL, 0));
    assert(0 == hashmap_bucket_size(NULL, 1));

    h = hashmap_new(sizeof(int));

    assert(0 == hashmap_bucket_size(h, 0));
    assert(0 == hashmap_bucket_size(h, 1));
    assert(0 == hashmap_bucket_size(h, 5));
    assert(0 == hashmap_bucket_size(h, 6));

    hashmap_insert(h, "Au");
    hashmap_insert(h, "Ag");
    assert(1 == hashmap_bucket_size(h, hashmap_bucket(h, "Au")));

    hashmap_delete(h);
}

static void test_hashmap_bucket(void)
{
    struct hashmap *h;

    assert(0 == hashmap_bucket(NULL, "foo"));
    assert(0 == hashmap_bucket(NULL, NULL));

    h = hashmap_new(sizeof(int));

    assert(0 == hashmap_bucket(h, NULL));

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

    assert(2 == hashmap_bucket(h, "Au"));
    assert(3 == hashmap_bucket(h, "Ag"));
    assert(3 == hashmap_bucket(h, "Cu"));
    assert(1 == hashmap_bucket(h, "Pt"));
    assert(4 == hashmap_size(h));
    assert(5 == hashmap_bucket_count(h));

    f = fabsf(0.8f - hashmap_load_factor(h));
    assert(f <= FLT_MIN);

    hashmap_max_load_factor_set(h, hashmap_max_load_factor(h) / 2.f);

    f = fabsf(0.5f - hashmap_max_load_factor(h));
    assert(f <= 0.000001f);

    assert(8 == hashmap_bucket(h, "Au"));
    assert(5 == hashmap_bucket(h, "Ag"));
    assert(8 == hashmap_bucket(h, "Cu"));
    assert(7 == hashmap_bucket(h, "Pt"));
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

    assert(-EFAULT == hashmap_max_load_factor_set(NULL, 1));

    h = hashmap_new(sizeof(int));

    f = fabsf(1.0f - hashmap_max_load_factor(h));
    assert(f <= FLT_MIN);

    assert(0 == hashmap_max_load_factor_set(h, 4));

    f = fabsf(4 - hashmap_max_load_factor(h));
    assert(f <= FLT_MIN);

    hashmap_max_load_factor_set(h, 0);
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

    h = hashmap_new(sizeof(int));

    ret = hashmap_insert(h, "a");
    *(int *)ret.pair->userdata = 0;

    ret = hashmap_insert(h, "b");
    *(int *)ret.pair->userdata = 1;

    ret = hashmap_insert(h, "c");
    *(int *)ret.pair->userdata = 2;

    calls = 0;
    for (struct hashmap_iter *it = hashmap_begin(h); it != hashmap_end(h); it = hashmap_advance(it, 1)) {
        struct hashmap_pair *pair = hashmap_at(it);
        int i = *(int *)pair->userdata;
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

    h = hashmap_new(sizeof(int));

    ret = hashmap_insert(h, "a");
    *(int *)ret.pair->userdata = 0;

    ret = hashmap_insert(h, "b");
    *(int *)ret.pair->userdata = 1;

    ret = hashmap_insert(h, "c");
    *(int *)ret.pair->userdata = 2;

    ch = h;

    calls = 0;
    for (const struct hashmap_iter *it = hashmap_cbegin(ch); it != hashmap_cend(ch); it = hashmap_advance_const(it, 1)) {
        const struct hashmap_pair *pair = hashmap_at_const(it);
        const int i = *(const int *)pair->userdata;
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

    h = hashmap_new(sizeof(int));

    hashmap_insert(h, "a");
    hashmap_insert(h, "b");
    hashmap_insert(h, "c");
    hashmap_insert(h, "d");

    ret = hashmap_insert(h, "e");
    assert(ret.inserted);

    ret = hashmap_insert(h, "e");
    assert(!ret.inserted);

    g_realloc_fail = true;
    ret = hashmap_insert(h, "f");
    g_realloc_fail = false;
    assert(!ret.inserted);
    assert(NULL == ret.pair);

    g_strdup_fail = true;
    ret = hashmap_insert(h, "f");
    g_strdup_fail = false;
    assert(!ret.inserted);
    assert(NULL == ret.pair);

    g_calloc_fail = true;
    ret = hashmap_insert(h, "f");
    g_calloc_fail = false;
    assert(!ret.inserted);
    assert(NULL == ret.pair);

    ret = hashmap_insert(h, "f");
    assert(ret.inserted);

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

static void test_hashmap_erase(void)
{
    struct hashmap *h;
    //struct hashmap_iter *it;

    assert(-EFAULT == hashmap_erase(NULL, NULL));

    h = hashmap_new(sizeof(int));

    assert(-EINVAL == hashmap_erase(h, hashmap_begin(h)));
    assert(-EINVAL == hashmap_erase(h, hashmap_end(h)));

    hashmap_insert(h, "a");
    assert(0 == hashmap_erase(h, hashmap_begin(h)));
    assert(-EINVAL == hashmap_erase(h, hashmap_begin(h)));

    hashmap_insert(h, "a");
    hashmap_insert(h, "f");
    assert(0 == hashmap_erase(h, hashmap_begin(h)));

    hashmap_delete(h);
}

static void test_hashmap_rehash(void)
{
    struct hashmap *h;

    assert(-EFAULT == hashmap_rehash(NULL, 42));

    h = hashmap_new(sizeof(int));

    g_realloc_fail = true;
    assert(-ENOMEM == hashmap_rehash(h, 42));
    g_realloc_fail = false;

    assert(5 == hashmap_bucket_count(h));

    assert(0 == hashmap_rehash(h, 3));
    assert(5 == hashmap_bucket_count(h));

    assert(0 == hashmap_rehash(h, 42));
    assert(42 == hashmap_bucket_count(h));

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
    test_hashmap_erase();
    test_hashmap_rehash();
    return 0;
}
