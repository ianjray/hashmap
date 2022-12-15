#include "hashmap.h"

#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(void)
{
    const char *filename = "/usr/share/dict/words";

    FILE *fp = fopen(filename, "r");
    if (!fp) {
        perror(filename);
        exit(1);
    }

    struct hashmap *h = hashmap_new(0);

    char buffer[80];
    while (fgets(buffer, sizeof(buffer), fp)) {
        char *p = strchr(buffer, '\n');
        if (!p) {
            abort();
        }

        *p = 0;

        // Exclude proper nouns.
        if (!isupper(buffer[0])) {
            struct hashmap_insert_ret ret = hashmap_insert(h, buffer);
            assert(ret.ok);
        }
    }

    fclose(fp);

    printf("size %zu\n", hashmap_size(h));
    printf("buckets %zu\n", hashmap_bucket_count(h));
    printf("load factor %f\n", hashmap_load_factor(h));
    hashmap_delete(h);
}
