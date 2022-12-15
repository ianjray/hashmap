#ifndef LIBHASHMAP_HASHMAP_H_
#define LIBHASHMAP_HASHMAP_H_

#include <stdbool.h>
#include <stddef.h>
#include <sys/types.h>

#ifdef __has_attribute
# define PUBLIC __attribute__ ((visibility("default")))
#else
# define PUBLIC /*NOTHING*/
#endif

/// Hash Map object.
///
/// Provides an associative container containing key-value pairs with unique keys.
/// Search, insertion, and removal of elements have average constant-time complexity.
///
/// This library is **not** thread-safe.
/// Caller must synchronize access to list objects.
/// Multiple readers are safe if no writers are active.
struct hashmap;

/// Constructor.
/// @param element_size Size of mapped type.
/// @note If @c element_size is zero, then userdata cannot be used.
/// @return Pointer to map on success.
/// @return NULL on failure, and errno is set to:
///   - ENOMEM: Insufficient memory.
/// @note Memory ownership: Caller must hashmap_delete() the returned pointer.
struct hashmap *hashmap_new(size_t element_size) PUBLIC;

/// Destructor.
/// @note Memory ownership: Takes ownership of the pointer.
void hashmap_delete(struct hashmap *) PUBLIC;

/// Test if map is empty.
/// @return Non-zero if map is empty (or NULL), zero otherwise.
bool hashmap_empty(const struct hashmap *) PUBLIC;

/// Get number of elements in map.
/// @return The number of elements in the map, or zero if empty or NULL.
/// @note Complexity: O(1).
size_t hashmap_size(const struct hashmap *) PUBLIC;

/// Erases all elements from the container.
/// @return 0 on success.
/// @return -1 on failure, and errno is set to:
///   - EINVAL: List invalid.
/// @note Invalidates all iterators.
int hashmap_clear(struct hashmap *) PUBLIC;

/// @return The number of buckets in the map, or zero if empty or NULL.
size_t hashmap_bucket_count(const struct hashmap *) PUBLIC;

/// @return The maximum number of buckets the container can hold, or zero if NULL.
size_t hashmap_max_bucket_count(const struct hashmap *) PUBLIC;

/// @return The number of elements in bucket @c n, or zero if NULL or invalid.
/// @note Complexity: O(bucket_size).
size_t hashmap_bucket_size(const struct hashmap *, size_t n) PUBLIC;

/// @return The bucket index where element @c key is located.
/// @return -1 on failure, and errno is set to:
///   - EINVAL: Map or key invalid.
/// @note The hashing algorithm is an implementation detail.
ssize_t hashmap_bucket(const struct hashmap *, const char *key) PUBLIC;

/// Returns the average number of elements per bucket, that is, @c hashmap_size() divided by @c hashmap_bucket_count().
/// @return Load factor, or zero on error.
float hashmap_load_factor(const struct hashmap *) PUBLIC;

/// Get maximum load factor (number of elements per bucket).
/// The container automatically increases the number of buckets if the load factor exceeds this threshold.
/// @return Maximum load factor, or zero on error.
float hashmap_max_load_factor(const struct hashmap *) PUBLIC;

/// Set maximum load factor.
/// @return Zero on success.
/// @return -1 on failure, and errno is set to:
///   - EINVAL: List invalid.
///   - ENOMEM: Insufficient memory.
/// @note Values of @c z less than 0.25 are promoted to 0.25.
/// @note May trigger rehash on success.
int hashmap_max_load_factor_set(struct hashmap *, float z) PUBLIC;

/// Iterator object.
struct hashmap_iter;

/// @return Iterator to the beginning, or NULL on error.
/// @note hashmap_begin(l) == hashmap_end(l) when the map is empty.
/// @note Memory ownership: The pointer is valid until element is erased or map deleted.
struct hashmap_iter *hashmap_begin(struct hashmap *) PUBLIC;

/// Constant variant of @c hashmap_begin.
/// @see hashmap_begin.
const struct hashmap_iter *hashmap_cbegin(const struct hashmap *) PUBLIC;

/// @return Iterator to the end, or NULL on error.
/// @note Memory ownership: The pointer is valid until element is erased or map deleted.
struct hashmap_iter *hashmap_end(struct hashmap *) PUBLIC;

/// Constant variant of @c hashmap_end.
/// @see hashmap_end.
const struct hashmap_iter *hashmap_cend(const struct hashmap *) PUBLIC;

/// @return Iterator advanced by @c offset (within bounds @c hashmap_begin to @c hashmap_end), or NULL on error.
/// @note Offsets are O(n).
/// @note Memory ownership: The pointer is valid until element is erased or map deleted.
struct hashmap_iter *hashmap_advance(struct hashmap_iter *, ssize_t offset) PUBLIC;

/// Constant variant of @c hashmap_advance.
/// @see hashmap_advance.
const struct hashmap_iter *hashmap_advance_const(const struct hashmap_iter *, ssize_t offset) PUBLIC;

struct hashmap_pair {
    const char *key;
    /// Dynamically sized storage for the value (mapped type).
    /// The size is specified as the @c element_size argument to the constructor.
    char userdata[1];
};

/// Get pair at iterator.
/// @return Pointer to pair, or NULL on error or if iterator == end().
/// @note Complexity: O(1).
/// @note The returned pointer is invalidated on erase(), clear(), or delete().
struct hashmap_pair *hashmap_at(struct hashmap_iter *) PUBLIC;

/// Const variant of hashmap_at().
/// @see hashmap_at.
const struct hashmap_pair *hashmap_at_const(const struct hashmap_iter *) PUBLIC;

/// Insert result.
/// inserted == true  => new element was created and pair points to it.
/// inserted == false && pair.key != NULL => element already existed; pair points to it.
/// inserted == false && pair.key == NULL => allocation failure or error; no element inserted.
struct hashmap_insert_result {
    bool inserted;
    struct hashmap_pair *pair;
};

/// Insert key into the map.
/// @return On success (either found or inserted) @c pair points to key and mapped type.
/// @note This function makes an internal copy of @c key using strdup().
/// @note Keys are case sensitive.
/// @note Memory ownership: Caller retains ownership of the given pointer.
struct hashmap_insert_result hashmap_insert(struct hashmap *, const char *key) PUBLIC;

/// Find element with specific key.
/// @return Iterator.
/// @note The iterator is hashmap_end() if arguments are NULL or @c key is not found.
struct hashmap_iter *hashmap_find(struct hashmap *, const char *key) PUBLIC;

/// Const variant of hashmap_find().
/// @see hashmap_find.
const struct hashmap_iter *hashmap_cfind(const struct hashmap *, const char *key) PUBLIC;

/// Erase element.
/// Invalidates iterator immediately.
/// @return 0 on success.
/// @return -1 on failure, and errno is set to:
///   - EINVAL: Map or iterator invalid.
int hashmap_erase(struct hashmap *, struct hashmap_iter *) PUBLIC;

/// Rehash.
/// Changes the number of buckets to a value @c n that is not less than @c count.
/// @return 0 on success.
/// @return -1 on failure, and errno is set to:
///   - EINVAL: Map invalid.
///   - ENOMEM: Insufficient memory.
int hashmap_rehash(struct hashmap *, size_t buckets) PUBLIC;

/// Request a capacity change.
/// Ensures capacity for at least @c number_of_elements at current @c max_load_factor.
/// @return 0 on success.
/// @return -1 on failure, and errno is set to:
///   - EINVAL: Map invalid.
///   - ENOMEM: Insufficient memory.
/// @note May trigger rehash on success.
int hashmap_reserve(struct hashmap *, size_t number_of_elements) PUBLIC;

#endif
