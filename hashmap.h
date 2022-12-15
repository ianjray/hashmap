#include <stdbool.h>
#include <stddef.h>

/// Constructor.
struct hashmap *hashmap_new(size_t element_size);

/// Destructor.
void hashmap_delete(struct hashmap *h);

/// @return bool True if the map is empty.
bool hashmap_empty(const struct hashmap *h);

/// @return size_t The number of elements in the map.
size_t hashmap_size(const struct hashmap *h);

struct hashmap_iter;

/// @return Iterator Returns an iterator to the beginning.
struct hashmap_iter *hashmap_begin(struct hashmap *h);

/// @return Iterator Returns an iterator to the end.
struct hashmap_iter *hashmap_end(struct hashmap *h);

/// Increment iterator position.
/// @return Iterator
struct hashmap_iter *hashmap_iter_inc(struct hashmap_iter *iter);

struct hashmap_pair {
    const char *key;
    void *userdata;
};

/// Dereference iterator.
/// @return void* Pointer to element at iterator position.
struct hashmap_pair hashmap_iter_deref(struct hashmap_iter *iter);

/// Find element with specific key.
struct hashmap_iter *hashmap_find(struct hashmap *h, const char *key);

struct hashmap_insert_ret {
    bool ok;
    struct hashmap_pair pair;
};

/// Insert element.
struct hashmap_insert_ret hashmap_insert(struct hashmap *h, const char *key);

/// Erase element.
void hashmap_erase(struct hashmap *h, struct hashmap_iter *iter);

/// Clears the contents of the map.
void hashmap_clear(struct hashmap *h);

/// @return size_t Number of buckets.
size_t hashmap_bucket_count(struct hashmap *h);

/// @return size_t The number of elements in the bucket @c n.
size_t hashmap_bucket_size(struct hashmap *h, size_t n);

/// @return size_t The bucket number when element @c key is located.
size_t hashmap_bucket(const struct hashmap *h, const char *key);

/// @return float Load factor.
float hashmap_load_factor(struct hashmap *h);

/// @return float Maximum load factor.
float hashmap_max_load_factor(struct hashmap *h);

/// Set maximum load factor.
void hashmap_max_load_factor_set(struct hashmap *h, float z);

/// Rehash
/// @param n Number of buckets.
void hashmap_rehash(struct hashmap *h, size_t n);

/// Request a capacity change.
void hashmap_reserve(struct hashmap *h, size_t elements);
