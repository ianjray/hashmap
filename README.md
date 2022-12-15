# hashmap
Hash Map Implementation in C

The API is modelled on that of C++ std::unordered_map, as far as this is possible in C.

Requires https://github.com/ianjray/llist.

## Example

```c
#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <libhashmap/hashmap.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int readline(FILE *fp, char *buf, size_t len)
{
    if (fgets(buf, (int)len, fp)) {
        char *p = strchr(buf, '\n');
        if (!p) {
            abort();
        }
        *p = 0;
        return 0;
    }

    if (errno) {
        return -errno;
    }

    return EOF;
}

static int readdict(const char *filename, struct hashmap *h)
{
    char buffer[80];
    FILE *fp;

    fp = fopen(filename, "r");
    if (!fp) {
        int r = errno;
        perror(filename);
        return -r;
    }

    while (readline(fp, buffer, sizeof(buffer)) == 0) {
        if (isupper(buffer[0])) {
            // Exclude proper nouns.
        } else {
            struct hashmap_insert_ret ret = hashmap_insert(h, buffer);
            assert(ret.ok);
        }
    }

    fclose(fp);

    return 0;
}

int main(void)
{
    struct hashmap *h;
    int r;

    h = hashmap_new(0);

    r = readdict("/usr/share/dict/words", h);
    if (r == 0) {
        printf("size %zu\n", hashmap_size(h));
        printf("buckets %zu\n", hashmap_bucket_count(h));
        printf("load factor %f\n", (double)hashmap_load_factor(h));
    }

    hashmap_delete(h);
}
```

### Output

```
$ words
size 210773
buckets 393241
load factor 0.535989
```
