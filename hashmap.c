#include "hashmap.h"

#include <liblist/llist.h>

#include <assert.h>
#include <errno.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

struct hashmap {
    /// Configurable maximum load factor.
    /// A value of 1.0 means each bucket contains one value.
    float max_load_factor;
    /// Size of mapped type.
    size_t element_size;
    /// The values (key, userdata pairs) stored in the hashmap.
    /// Ordered such that they may be indexed by the iterators in buckets[].
    /// @see @c value.
    struct list *entries;
    /// Dynamically sized array of iterators to the start of each bucket.
    /// Keys with the same hash code appear in the same bucket.
    /// This allows fast access to individual elements, since once the hash is computed, it refers to the bucket containing the element.
    struct list_iter **buckets;
    /// Number of buckets in array @c buckets.
    size_t num_buckets;
};

/// Models a {key, mapped type} value.
struct entry {
    /// Linked list management.
    LIST_NODE(node);
    /// Key hash.
    /// Derived from @c key, and cached as an optimization.
    size_t hash;
    /// Key.
    char *key;
    /// Mapped type.
    void *userdata;
};

/// @return Hash of given @c key.
/// @see http://www.cse.yorku.ca/~oz/hash.html
static size_t hash_of(const char *key)
{
    // djb2
    size_t hash = 5381;

    while (*key) {
        // hash * 33 + c.
        hash = ((hash << 5) + hash) + (size_t)(*key++);
    }

    return hash;
}

/// Constructor.
/// @return Pointer to entry, or NULL on error.
/// @note Memory ownership: Caller must entry_delete() the returned pointer.
/// @note This function makes an internal copy of @c key using strdup().
static struct entry *entry_new(size_t element_size, const char *key)
{
    char *key_copy;
    struct entry *entry;

    key_copy = strdup(key);
    if (!key_copy) {
        return NULL;
    }

    entry = calloc(1, offsetof(struct entry, userdata) + element_size);
    if (!entry) {
        free(key_copy);
        return NULL;
    }

    entry->hash = hash_of(key_copy);
    entry->key  = key_copy;
    return entry;
}

/// Destructor.
/// @note Memory ownership: Takes ownership of the pointer.
static void entry_delete(void *ptr)
{
    struct entry *entry = (struct entry *)ptr;
    free(entry->key);
}

/// Arbitrary subset of prime numbers.
/// Trade-off memory overhead vs rehash cost.
static size_t primes[] =
{
    5, 11, 23,
    47, 53, 97,
    193, 389, 769,
    1543, 3079, 6151,
    12289, 24593, 49157,
    98317, 196613, 393241,
    786433, 1572869, 3145739,
    6291469, 12582917, 25165843,
    50331653, 100663319, 201326611,
    402653189, 805306457, 1610612741
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

/// @return Number of buckets required to support @c number_of_elements at maximum load factor.
static size_t impl_bucket_count_calculate(struct hashmap *h, size_t number_of_elements)
{
    return impl_bucket_count_ideal((size_t)((float)number_of_elements / hashmap_max_load_factor(h)));
}

static int impl_alloc_buckets(struct hashmap *h, size_t n)
{
    size_t size;
    void *buckets;

    size = n * sizeof(struct list_iter *);

    buckets = realloc(h->buckets, size);
    if (!buckets) {
        // Leave existing state unchanged on allocation failure.
        return -ENOMEM;
    }

    h->num_buckets = n;
    h->buckets = buckets;
    memset(h->buckets, 0, size);
    return 0;
}

struct hashmap *hashmap_new(size_t element_size)
{
    struct hashmap *h = calloc(1, sizeof(struct hashmap));
    if (!h) {
        return NULL;
    }

    h->max_load_factor = 1.0;
    h->element_size = element_size;
    h->entries = list_new(offsetof(struct entry, node));

    if (impl_alloc_buckets(h, impl_bucket_count_calculate(h, hashmap_size(h))) < 0) {
        hashmap_delete(h);
        h = NULL;
    }

    return h;
}

void hashmap_delete(struct hashmap *h)
{
    if (!h) {
        return;
    }

    list_delete(h->entries, entry_delete);
    free(h->buckets);
    free(h);
}

bool hashmap_empty(const struct hashmap *h)
{
    return hashmap_size(h) == 0;
}

size_t hashmap_size(const struct hashmap *h)
{
    if (!h) {
        return 0;
    }

    return list_size(h->entries);
}

/// Rehash if there is an insufficient number of buckets for the @c number_of_elements.
/// @return Zero on success, negative errno otherwise.
static int impl_rehash_if_needed(struct hashmap *h, size_t number_of_elements)
{
    size_t required;

    required = impl_bucket_count_calculate(h, number_of_elements);
    if (required <= h->num_buckets) {
        return 0;
    }

    return hashmap_rehash(h, required);
}

int hashmap_clear(struct hashmap *h)
{
    if (!h) {
        return -EFAULT;
    }

    list_clear(h->entries, entry_delete);
    memset(h->buckets, 0, h->num_buckets * sizeof(struct list_iter *));
    return 0;
}

size_t hashmap_bucket_count(const struct hashmap *h)
{
    if (!h) {
        return 0;
    }

    return h->num_buckets;
}

/// @return Bucket of given @c hash.
/// @note The bucket is 1-indexed.
static size_t bucket_of(const struct hashmap *h, size_t hash)
{
    // Divide-by-zero is theoretical, but not possible.
    assert(h->num_buckets);
    return (hash % h->num_buckets) + 1;
}

size_t hashmap_bucket_size(const struct hashmap *h, size_t bucket)
{
    size_t count = 0;

    if (!h) {
        return 0;
    }

    if (bucket == 0) {
        // Reject invalid bucket.
        return 0;
    }

    if (bucket > h->num_buckets) {
        // No such bucket.
        return 0;
    }

    for (struct list_iter *it = h->buckets[bucket - 1]; ; it = list_next(it)) {
        struct entry *entry = list_at(it);
        if (!entry) {
            // Bucket empty or list_end() reached.
            break;
        }

        if (bucket_of(h, entry->hash) != bucket) {
            // Iterated to next bucket.
            break;
        }

        count++;
    }

    return count;
}

size_t hashmap_bucket(const struct hashmap *h, const char *key)
{
    if (!h || !key) {
        return 0;
    }

    return bucket_of(h, hash_of(key));
}

float hashmap_load_factor(const struct hashmap *h)
{
    size_t sz;
    size_t n;

    if (!h) {
        return 0;
    }

    sz = hashmap_size(h);
    n = hashmap_bucket_count(h);
    return (float)sz / (float)n;
}

float hashmap_max_load_factor(const struct hashmap *h)
{
    if (!h) {
        return 0;
    }

    return h->max_load_factor;
}

int hashmap_max_load_factor_set(struct hashmap *h, float z)
{
    const float lower_bound = 0.25;

    if (!h) {
        return -EFAULT;
    }

    if (z < lower_bound) {
        z = lower_bound;
    }

    h->max_load_factor = z;

    return impl_rehash_if_needed(h, hashmap_size(h));
}

/// Iterator has the same layout as the list node.
/// Clients only ever see a pointer-to-iterator, thus the implementation is opaque.
struct hashmap_iter {
    struct list_node node;
};

struct hashmap_iter *hashmap_begin(struct hashmap *h)
{
    // Const cast away is safe because @c h is non-const.
    return (struct hashmap_iter *)hashmap_cbegin(h);
}

const struct hashmap_iter *hashmap_cbegin(const struct hashmap *h)
{
    if (!h) {
        return NULL;
    }

    return (const struct hashmap_iter *)list_begin(h->entries);
}

struct hashmap_iter *hashmap_end(struct hashmap *h)
{
    // Const cast away is safe because @c h is non-const.
    return (struct hashmap_iter *)hashmap_cend(h);
}

const struct hashmap_iter *hashmap_cend(const struct hashmap *h)
{
    if (!h) {
        return NULL;
    }

    return (const struct hashmap_iter *)list_end(h->entries);
}

struct hashmap_iter *hashmap_advance(struct hashmap_iter *it, ssize_t offset)
{
    return (struct hashmap_iter *)list_advance((struct list_iter *)it, offset);
}

const struct hashmap_iter *hashmap_advance_const(const struct hashmap_iter *it, ssize_t offset)
{
    return (const struct hashmap_iter *)list_advance_const((const struct list_iter *)it, offset);
}

/// @return Pointer overlay to internal entry.key / entry.userdata fields.
static struct hashmap_pair *impl_pair_of(const struct hashmap_iter *it)
{
    struct entry *entry = list_at((struct list_iter *)it);

    if (!entry) {
        return NULL;
    }

    // Memory layout guarantee:
    //   struct entry { ...; char *key; void *userdata; }
    //
    // hashmap_pair has identical tail layout (key, userdata).
    // We safely alias the address of entry->key.

    return (struct hashmap_pair *)&entry->key;
}

struct hashmap_pair *hashmap_at(struct hashmap_iter *it)
{
    return impl_pair_of(it);
}

const struct hashmap_pair *hashmap_at_const(const struct hashmap_iter *it)
{
    return impl_pair_of(it);
}

struct hashmap_insert_result hashmap_insert(struct hashmap *h, const char *key)
{
    struct hashmap_iter *it;
    struct hashmap_insert_result ret;
    struct entry *entry;
    size_t bucket;
    int r;

    ret.inserted = false;
    ret.pair = NULL;

    if (!h || !key) {
        return ret;
    }

    it = hashmap_find(h, key);
    if (it != hashmap_end(h)) {
        // Already present, return existing pair.
        ret.inserted = false;
        ret.pair = hashmap_at(it);
        return ret;
    }

    r = impl_rehash_if_needed(h, hashmap_size(h) + 1 /* For the element to be added. */);
    if (r < 0) {
        return ret;
    }

    entry = entry_new(h->element_size, key);
    if (!entry) {
        return ret;
    }

    // list_insert() temporarily places the entry at list_end().
    // No search or visible operation occurs before list_splice(),
    // so this brief intermediate state is safe.

    it = (struct hashmap_iter *)list_insert(list_end(h->entries), entry);
    if (!it) {
        entry_delete(entry); // UNREACHABLE
        return ret; // UNREACHABLE
    }

    bucket = bucket_of(h, entry->hash);

    // Insert the new entry at the *front* of the bucket run.
    // This guarantees O(1) insertion.
    // Each bucket is a implemented as contiguous run in the entries list.
    list_splice(h->buckets[bucket - 1], (struct list_iter *)it);
    h->buckets[bucket - 1] = (struct list_iter *)it;

    ret.inserted = true;
    ret.pair = impl_pair_of(it);

    return ret;
}

struct hashmap_iter *hashmap_find(struct hashmap *h, const char *key)
{
    size_t hash;
    size_t bucket;

    if (!h || !key) {
        return hashmap_end(h);
    }

    hash = hash_of(key);
    bucket = bucket_of(h, hash);

    for (struct list_iter *it = h->buckets[bucket - 1]; ; it = list_next(it)) {
        struct entry *entry = list_at(it);
        if (!entry) {
            // Bucket empty or list_end() reached.
            break;
        }

        if (bucket_of(h, entry->hash) != bucket) {
            // Iterated to next bucket.
            break;
        }

        if (hash == entry->hash && !strcmp(key, entry->key)) {
            // Found @c key.
            return (struct hashmap_iter *)it;
        }
    }

    return hashmap_end(h);
}

int hashmap_erase(struct hashmap *h, struct hashmap_iter *it)
{
    struct entry *entry;
    size_t bucket;

    if (!h) {
        return -EFAULT;
    }

    entry = (struct entry *)list_at((struct list_iter *)it);
    if (!entry) {
        return -EINVAL;
    }

    bucket = hashmap_bucket(h, entry->key);

    if (h->buckets[bucket - 1] == (struct list_iter *)it) {
        struct list_iter *next = list_next((struct list_iter *)it);

        if (next != list_end(h->entries) && bucket_of(h, ((struct entry *)list_at(next))->hash) == bucket) {
            h->buckets[bucket - 1] = next;

        } else {
            h->buckets[bucket - 1] = NULL;
        }
    }

    return list_erase((struct list_iter *)it, entry_delete);
}

int hashmap_rehash(struct hashmap *h, size_t buckets)
{
    struct list_iter *it;
    int r;

    if (!h) {
        return -EFAULT;
    }

    if (buckets <= h->num_buckets) {
        // Early return if we have sufficient buckets.
        return 0;
    }

    r = impl_alloc_buckets(h, buckets);
    if (r < 0) {
        return r;
    }

    // Assign a new bucket for all elements after number of buckets was changed.
    for (it = list_begin(h->entries); it != list_end(h->entries); ) {
        struct list_iter *next = list_next(it);
        struct entry *entry;
        size_t bucket;

        entry = list_at(it);

        bucket = hashmap_bucket(h, entry->key);

        list_splice(h->buckets[bucket - 1], it);
        h->buckets[bucket - 1] = it;

        it = next;
    }

    return 0;
}

int hashmap_reserve(struct hashmap *h, size_t number_of_elements)
{
    float required;
    size_t capacity;

    if (!h) {
        return -EFAULT;
    }

    required = ceilf(h->num_buckets * h->max_load_factor);
    capacity = (size_t)required;

    if (number_of_elements <= capacity) {
        // reserve() only expands, never errors.
        return 0;
    }

    return impl_rehash_if_needed(h, number_of_elements);
}
