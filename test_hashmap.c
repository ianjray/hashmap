#include "hashmap.h"

#include <assert.h>
#include <float.h>
#include <math.h>
#include <string.h>

struct bucket {
    int value;
    char data[16];
};

static void insert(struct hashmap *h, const char *key, int value)
{
    struct hashmap_insert_ret ret;
    struct bucket *b;

    ret = hashmap_insert(h, key);
    assert(ret.ok == true);

    b = (struct bucket *)ret.pair.userdata;
    b->value = value;
    memset(b->data, 0xCC, sizeof(b->data));
}

static void check_element(struct hashmap *h, size_t bucket, const char *key, int value)
{
    struct hashmap_iter *iter;
    struct hashmap_pair pair;

    iter = hashmap_find(h, key);
    assert(iter != hashmap_end(h));

    pair = hashmap_iter_deref(iter);
    assert(!strcmp(key, pair.key));

    assert(bucket == hashmap_bucket(h, key));
    assert(value == ((struct bucket *)pair.userdata)->value);
}

int main(void)
{
    struct hashmap *h;
    struct hashmap_iter *iter;
    struct hashmap_insert_ret ret;
    struct bucket *b;
    struct hashmap_pair pair;
    float d;


    assert(hashmap_bucket(NULL, "foo") == 0);
    assert(hashmap_empty(NULL));
    assert(hashmap_size(NULL) == 0);


    h = hashmap_new(sizeof(struct bucket));
    assert(hashmap_empty(h));
    assert(hashmap_size(h) == 0);
    assert(hashmap_bucket_count(h) == 5);


    hashmap_reserve(h, 50);
    assert(hashmap_bucket_count(h) == 53);

    // NOP
    hashmap_rehash(h, 50);


    iter = hashmap_begin(h);
    assert(iter == hashmap_end(h));
    iter = hashmap_iter_inc(iter);
    assert(iter == hashmap_end(h));


    insert(h, "bacteria", 10);
    assert(hashmap_size(h) == 1);
    assert(!hashmap_empty(h));

    ret = hashmap_insert(h, "bacteria");
    assert(ret.ok == false);
    b = (struct bucket *)ret.pair.userdata;
    assert(b->value == 10);
    b->value = 1;
    assert(hashmap_size(h) == 1);

    insert(h, "adept", 2);
    assert(hashmap_size(h) == 2);
    insert(h, "entitlement", 3);
    insert(h, "choir", 4);
    insert(h, "geodesic", 5);
    insert(h, "impermeable", 6);
    insert(h, "adjuster", 7);
    insert(h, "entourage", 8);
    insert(h, "aerofoil", 9);
    insert(h, "bandage", 10);
    insert(h, "germless", 11);
    insert(h, "envelop", 12);
    assert(hashmap_size(h) == 12);

    ret = hashmap_insert(h, "bacteria");
    assert(ret.ok == false);
    b = (struct bucket *)ret.pair.userdata;
    assert(b->value == 1);
    assert(hashmap_size(h) == 12);

    iter = hashmap_find(h, "impermeable");
    hashmap_erase(h, iter);
    assert(hashmap_size(h) == 11);
    insert(h, "impermeable", 6);
    assert(hashmap_size(h) == 12);


    assert(0 == hashmap_bucket_size(NULL, 0));
    assert(0 == hashmap_bucket_size(h, hashmap_bucket_count(h)));

    assert(3 == hashmap_bucket_size(h, 0));  /* adept, adjuster, aerofoil */
    assert(2 == hashmap_bucket_size(h, 1));  /* bacteria, bandage */
    assert(1 == hashmap_bucket_size(h, 2));  /* choir */
    assert(0 == hashmap_bucket_size(h, 3));
    assert(3 == hashmap_bucket_size(h, 4));  /* entitlement, entourage, envelop */
    assert(0 == hashmap_bucket_size(h, 5));
    assert(2 == hashmap_bucket_size(h, 6));  /* geodesic, germless */
    assert(0 == hashmap_bucket_size(h, 7));
    assert(1 == hashmap_bucket_size(h, 8));  /* impermeable */
    assert(0 == hashmap_bucket_size(h, 9));

    check_element(h, 0, "adept", 2);
    check_element(h, 0, "adjuster", 7);
    check_element(h, 0, "aerofoil", 9);
    check_element(h, 1, "bacteria", 1);
    check_element(h, 1, "bandage", 10);
    check_element(h, 2, "choir", 4);
    check_element(h, 4, "entitlement", 3);
    check_element(h, 4, "entourage", 8);
    check_element(h, 4, "envelop", 12);
    check_element(h, 6, "geodesic", 5);
    check_element(h, 6, "germless", 11);
    check_element(h, 8, "impermeable", 6);

    assert(hashmap_find(h, "alumnal") == hashmap_end(h));


    pair = hashmap_iter_deref(hashmap_end(h));
    assert(NULL == pair.key);
    assert(NULL == pair.userdata);

    iter = hashmap_begin(h);
    assert(iter != hashmap_end(h));
    pair = hashmap_iter_deref(iter);
    assert(!strcmp("adept", pair.key));

    iter = hashmap_iter_inc(iter);
    assert(iter != hashmap_begin(h));
    assert(iter != hashmap_end(h));
    pair = hashmap_iter_deref(iter);
    assert(!strcmp("adjuster", pair.key));

    iter = hashmap_find(h, "germless");
    assert(iter != hashmap_begin(h));
    assert(iter != hashmap_end(h));

    iter = hashmap_iter_inc(iter);
    assert(iter != hashmap_begin(h));
    assert(iter != hashmap_end(h));
    pair = hashmap_iter_deref(iter);
    assert(!strcmp("impermeable", pair.key));

    iter = hashmap_iter_inc(iter);
    assert(iter != hashmap_begin(h));
    assert(iter == hashmap_end(h));


    hashmap_erase(NULL, iter);
    assert(hashmap_size(h) == 12);

    hashmap_erase(h, iter);
    assert(hashmap_size(h) == 12);


    iter = hashmap_find(h, "adjuster");
    hashmap_erase(h, iter);
    assert(hashmap_size(h) == 11);
    assert(2 == hashmap_bucket_size(h, 0));
    assert(2 == hashmap_bucket_size(h, 1));
    assert(1 == hashmap_bucket_size(h, 2));
    assert(0 == hashmap_bucket_size(h, 3));
    assert(3 == hashmap_bucket_size(h, 4));
    assert(0 == hashmap_bucket_size(h, 5));
    assert(2 == hashmap_bucket_size(h, 6));
    assert(0 == hashmap_bucket_size(h, 7));
    assert(1 == hashmap_bucket_size(h, 8));

    iter = hashmap_find(h, "bacteria");
    hashmap_erase(h, iter);
    assert(hashmap_size(h) == 10);
    assert(2 == hashmap_bucket_size(h, 0));
    assert(1 == hashmap_bucket_size(h, 1));
    assert(1 == hashmap_bucket_size(h, 2));
    assert(0 == hashmap_bucket_size(h, 3));
    assert(3 == hashmap_bucket_size(h, 4));
    assert(0 == hashmap_bucket_size(h, 5));
    assert(2 == hashmap_bucket_size(h, 6));
    assert(0 == hashmap_bucket_size(h, 7));
    assert(1 == hashmap_bucket_size(h, 8));

    iter = hashmap_find(h, "choir");
    hashmap_erase(h, iter);
    assert(hashmap_size(h) == 9);
    assert(2 == hashmap_bucket_size(h, 0));
    assert(1 == hashmap_bucket_size(h, 1));
    assert(0 == hashmap_bucket_size(h, 2));
    assert(0 == hashmap_bucket_size(h, 3));
    assert(3 == hashmap_bucket_size(h, 4));
    assert(0 == hashmap_bucket_size(h, 5));
    assert(2 == hashmap_bucket_size(h, 6));
    assert(0 == hashmap_bucket_size(h, 7));
    assert(1 == hashmap_bucket_size(h, 8));


    d = fabsf(0.f - hashmap_load_factor(NULL));
    assert(d <= FLT_MIN);
    d = fabsf(0.169811f - hashmap_load_factor(h));
    assert(d <= 0.000001f);

    d = fabsf(1.0f - hashmap_max_load_factor(h));
    assert(d <= FLT_MIN);

    hashmap_max_load_factor_set(h, 0);
    d = fabsf(0.25f - hashmap_max_load_factor(h));
    assert(d <= FLT_MIN);


    hashmap_clear(NULL);
    assert(hashmap_size(h) == 9);
    assert(!hashmap_empty(h));

    hashmap_clear(h);
    assert(hashmap_size(h) == 0);
    assert(hashmap_empty(h));


    hashmap_delete(NULL);
    hashmap_delete(h);


    // https://cplusplus.com/reference/unordered_map/unordered_map/max_load_factor/
    h = hashmap_new(sizeof(struct bucket));

    ret = hashmap_insert(h, "Au");
    ((struct bucket *)ret.pair.userdata)->value = 10;
    ret = hashmap_insert(h, "Ag");
    ((struct bucket *)ret.pair.userdata)->value = 20;
    ret = hashmap_insert(h, "Cu");
    ((struct bucket *)ret.pair.userdata)->value = 30;
    ret = hashmap_insert(h, "Pt");
    ((struct bucket *)ret.pair.userdata)->value = 40;

    assert(4 == hashmap_size(h));
    assert(5 == hashmap_bucket_count(h));

    d = fabsf(0.8f - hashmap_load_factor(h));
    assert(d <= FLT_MIN);

    hashmap_max_load_factor_set(h, hashmap_max_load_factor(h) / 2.f);
    assert(4 == hashmap_size(h));
    assert(11 == hashmap_bucket_count(h));

    d = fabsf(0.5f - hashmap_max_load_factor(h));
    assert(d <= 0.000001f);

    d = fabsf(0.363636f - hashmap_load_factor(h));
    assert(d <= 0.000001f);

    check_element(h, 7, "Au", 10);
    check_element(h, 4, "Ag", 20);
    check_element(h, 7, "Cu", 30);
    check_element(h, 6, "Pt", 40);

    hashmap_delete(h);
}
