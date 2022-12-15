#include "hashmap.h"

#include <liblist/llist.h>

#include <stdlib.h>
#include <string.h>

struct hashmap_node {
    struct list_node node;
    size_t hash;
    size_t bucket;
    char *key;
    void *userdata;
};

static struct hashmap_node *make(size_t element_size)
{
    return malloc(offsetof(struct hashmap_node, userdata) + element_size);
}

static void node_destructor(void *n)
{
    struct hashmap_node *node = (struct hashmap_node *)n;
    free(node->key);
}

/* Iterator has the same layout as the list node.
 * Clients only ever see a pointer-to-iterator, thus the implementation is opaque. */
struct hashmap_iter {
    int unused;
};

struct hashmap {
    float max_load_factor;
    size_t buckets;
    size_t element_size;
    struct list *list;
    struct list_iter **map;
};

/* Arbitrary subset of prime numbers.
 * Trade-off memory overhead vs rehash cost. */
static size_t primes[] =
{
    5, 11, 23, 47, 53, 97,
    193, 389, 769,
    1543, 3079, 6151,
    12289, 24593, 49157, 98317,
    196613, 393241, 786433,
    1572869, 3145739, 6291469,
    12582917, 25165843, 50331653,
    100663319, 201326611, 402653189, 805306457,
    1610612741
};

static size_t impl_bucket_count_ideal(size_t n)
{
    size_t i;
    size_t ret = 5;

    for (i = 0; i < sizeof(primes)/sizeof(*primes) && ret < n; ++i) {
        ret = primes[i];
    }

    return ret;
}

/// @return size_t Bucket count that supports @c elements at maximum load factor.
static size_t impl_bucket_count_calculate(struct hashmap *h, size_t elements)
{
    return impl_bucket_count_ideal((size_t)((float)(elements / hashmap_max_load_factor(h))));
}

static void impl_alloc_buckets(struct hashmap *h, size_t n)
{
    size_t size = n * sizeof(struct list_iter *);

    h->buckets = n;
    h->map = (struct list_iter **)realloc(h->map, size);
    memset(h->map, 0, size);
}

struct hashmap *hashmap_new(size_t element_size)
{
    struct hashmap *h = (struct hashmap *)malloc(sizeof(struct hashmap));

    element_size += offsetof(struct hashmap_node, userdata);

    h->max_load_factor = 1.0;
    h->buckets = 0;
    h->element_size = element_size;
    h->list = list_new(offsetof(struct hashmap_node, node));
    h->map = NULL;
    impl_alloc_buckets(h, impl_bucket_count_calculate(h, hashmap_size(h)));
    return h;
}

void hashmap_delete(struct hashmap *h)
{
    if (!h) {
        return;
    }

    hashmap_clear(h);
    list_delete(h->list);
    free(h->map);
    free(h);
}

bool hashmap_empty(const struct hashmap *h)
{
    if (!h) {
        return true;
    }

    return list_size(h->list) == 0;
}

size_t hashmap_size(const struct hashmap *h)
{
    if (!h) {
        return 0;
    }

    return list_size(h->list);
}

struct hashmap_iter *hashmap_begin(struct hashmap *h)
{
    return (struct hashmap_iter *)list_begin(h->list);
}

struct hashmap_iter *hashmap_end(struct hashmap *h)
{
    return (struct hashmap_iter *)list_end(h->list);
}

struct hashmap_iter *hashmap_iter_inc(struct hashmap_iter *iter)
{
    return (struct hashmap_iter *)list_next((struct list_iter *)iter);
}

struct hashmap_pair hashmap_iter_deref(struct hashmap_iter *iter)
{
    struct hashmap_node *node = list_at((struct list_iter *)iter);
    struct hashmap_pair pair;

    if (node) {
        pair.key = node->key;
        pair.userdata = &node->userdata;

    } else {
        pair.key = NULL;
        pair.userdata = NULL;
    }

    return pair;
}

static size_t hashof(const char *key)
{
    // http://www.cse.yorku.ca/~oz/hash.html
    // djb2
    size_t hash = 5381;

    while (*key) {
        hash = ((hash << 5) + hash) + (size_t)(*key++); /* hash * 33 + c */
    }

    return hash;
}

struct hashmap_iter *hashmap_find(struct hashmap *h, const char *key)
{
    size_t hash = hashof(key);
    size_t bucket = hash % h->buckets;
    struct list_iter *iter;

    for (iter = h->map[bucket]; iter; iter = list_next(iter)) {
        struct hashmap_node *node = list_at((struct list_iter *)iter);

        if (!node) {
            break;
        }

        if (node->bucket != bucket) {
            break;
        }

        if (hash == node->hash && !strcmp(key, node->key)) {
            return (struct hashmap_iter *)iter;
        }
    }

    return hashmap_end(h);
}

struct hashmap_insert_ret hashmap_insert(struct hashmap *h, const char *key)
{
    struct list_iter *iter;
    struct hashmap_insert_ret ret;

    hashmap_rehash(h, impl_bucket_count_calculate(h, hashmap_size(h)));

    iter = (struct list_iter *)hashmap_find(h, key);
    if (iter != list_end(h->list)) {
        ret.ok = false;
        ret.pair = hashmap_iter_deref((struct hashmap_iter *)iter);

    } else {
        size_t hash = hashof(key);
        size_t bucket = hash % h->buckets;
        struct hashmap_node *node;

        node = list_insert(list_end(h->list), make(h->element_size));
        node->hash = hash;
        node->bucket = bucket;
        node->key = strdup(key);

        iter = list_prev(list_end(h->list));
        list_splice(h->map[node->bucket], iter);
        h->map[node->bucket] = iter;

        ret.ok = true;
        ret.pair.key = node->key;
        ret.pair.userdata = &node->userdata;
    }

    return ret;
}

void hashmap_erase(struct hashmap *h, struct hashmap_iter *iter)
{
    if (!h) {
        return;

    } else if (iter == hashmap_end(h)) {
        return;

    } else {
        struct hashmap_node *node = list_at((struct list_iter *)iter);
        size_t bucket = hashmap_bucket(h, node->key);

        if (h->map[bucket] == (struct list_iter *)iter) {
            struct list_iter *next = list_next((struct list_iter *)iter);

            if (next != list_end(h->list) && ((struct hashmap_node *)list_at((struct list_iter *)next))->bucket == bucket) {
                h->map[bucket] = next;

            } else {
                h->map[bucket] = NULL;
            }
        }

        node_destructor(list_unlink((struct list_iter *)iter));
    }
}

void hashmap_clear(struct hashmap *h)
{
    if (!h) {
        return;
    }

    list_erase_all(h->list, node_destructor);
    memset(h->map, 0, h->buckets * sizeof(struct hashmap_node *));
}

size_t hashmap_bucket_count(struct hashmap *h)
{
    return h->buckets;
}

size_t hashmap_bucket_size(struct hashmap *h, size_t bucket)
{
    if (!h) {
        return 0;

    } else if (bucket >= h->buckets) {
        return 0;

    } else {
        struct list_iter *iter;
        size_t count = 0;

        for (iter = h->map[bucket]; iter; iter = list_next(iter)) {
            struct hashmap_node *node = list_at((struct list_iter *)iter);

            if (!node || node->bucket != bucket) {
                break;
            }

            count++;
        }

        return count;
    }
}

size_t hashmap_bucket(const struct hashmap *h, const char *key)
{
    if (!h) {
        return 0;
    }

    return hashof(key) % h->buckets;
}

float hashmap_load_factor(struct hashmap *h)
{
    if (!h) {
        return 0;
    }

    return hashmap_size(h) / (float)h->buckets;
}

float hashmap_max_load_factor(struct hashmap *h)
{
    return h->max_load_factor;
}

void hashmap_max_load_factor_set(struct hashmap *h, float z)
{
    if (z < 0.25f) {
        z = 0.25f;
    }

    h->max_load_factor = z;

    hashmap_rehash(h, impl_bucket_count_calculate(h, hashmap_size(h)));
}

void hashmap_rehash(struct hashmap *h, size_t n)
{
    struct list_iter *iter;

    if (n <= h->buckets) {
        return;
    }

    impl_alloc_buckets(h, n);

    // Assign a new bucket for all elements after number of buckets was changed.
    for (iter = list_begin(h->list); iter != list_end(h->list); ) {
        struct list_iter *next = list_next(iter);
        struct hashmap_node *node;

        node = (struct hashmap_node *)list_at(iter);
        node->bucket = hashmap_bucket(h, node->key);

        list_splice(h->map[node->bucket], iter);
        h->map[node->bucket] = iter;

        iter = next;
    }
}

void hashmap_reserve(struct hashmap *h, size_t elements)
{
    size_t capacity = (size_t)(h->buckets * h->max_load_factor);

    if (elements > capacity) {
        hashmap_rehash(h, impl_bucket_count_calculate(h, elements));
    }
}
