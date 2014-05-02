#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <assert.h>
#include "quadratic_probing_hashmap.h"
#define SPACERATIO 0.5
static int __tombstone;
typedef struct hash_node_s hash_node_t;
struct hash_node_s {
    void * key;
    void * val;
};
static void __ensurecapacity (hashmapq_t * h);

static int is_power_of_two (unsigned int x) {
    return ((x != 0) && !(x & (x - 1)));
}

hashmapq_t * hashmapq_new (func_longhash_f hash, func_longcmp_f cmp, unsigned int initial_capacity) {
    hashmapq_t * h;
    assert (is_power_of_two (initial_capacity));
    h = calloc (1, sizeof (hashmapq_t));
    h->size = initial_capacity;
    h->array = calloc (h->size, sizeof (hash_node_t));
    h->hash = hash;
    h->compare = cmp;
    return h;
}

int hashmapq_count (const hashmapq_t * h) {
    return h->count;
}

int hashmapq_size (hashmapq_t * h) {
    return h->size;
}

void hashmapq_clear (hashmapq_t * h) {
    int ii;
    for (ii = 0; ii < h->size; ii++) {
        hash_node_t * n;
        n = &((hash_node_t *) h->array)[ii];
        if (NULL == n->key)
            continue;
        if (n->key == (void *) &__tombstone) {
            n->key = NULL;
            continue;
        }
        n->key = NULL;
        h->count--;
        assert (0 <= h -> count);
    }
    assert (0 == hashmapq_count (h));
}

void hashmapq_free (hashmapq_t * h) {
    assert (h);
    hashmapq_clear (h);
}

void hashmapq_freeall (hashmapq_t * h) {
    assert (h);
    hashmapq_free (h);
    free (h);
}

void * hashmapq_get (hashmapq_t * h, const void * key) {
    unsigned int slot;
    hash_node_t * n;
    if (0 == hashmapq_count (h) || !key)
        return NULL;
    slot = h->hash (key);
    n = &((hash_node_t *) h->array)[slot % h->size];
    int new_slot;
    int i;
    for (i = 0;; i++) {
        new_slot = (slot + (i / 2) + (i * i) / 2) % h->size;
        n = &((hash_node_t *) h->array)[new_slot];
        if (!n->key)
            break;
        if (n->key == (void *) &__tombstone)
            continue;
        if (0 == h->compare (key, n->key)) {
            return (void *) n->val;
        }
    }
    return NULL;
}

int hashmapq_contains_key (hashmapq_t * h, const void * key) {
    return (NULL != hashmapq_get (h, key));
}

void hashmapq_remove_entry (hashmapq_t * h, hash_entry_t * entry, const void * k) {
    hash_node_t * n;
    int slot;
    int i;
    slot = h->hash (k);
    n = &((hash_node_t *) h->array)[slot % h->size];
    if (!n->key)
        goto notfound;
    for (i = 0;; i++) {
        int new_slot;
        new_slot = (slot + (i / 2) + (i * i) / 2) % h->size;
        n = &((hash_node_t *) h->array)[new_slot];
        if (!n->key)
            goto notfound;
        if (n->key == (void *) &__tombstone)
            continue;
        if (0 == h->compare (k, n->key)) {
            n->key = &__tombstone;
            h->count--;
            entry->key = n->key;
            entry->val = n->val;
            return;
        }
    }
notfound :
    entry->key = NULL;
    entry->val = NULL;
}

void * hashmapq_remove (hashmapq_t * h, const void * key) {
    hash_entry_t entry;
    hashmapq_remove_entry (h, & entry, key);
    return (void *) entry.val;
}

void * hashmapq_put (hashmapq_t * h, void * k, void * v) {
    hash_node_t * n;
    int slot;
    if (!k || !v)
        return NULL;
    __ensurecapacity (h);
    slot = h->hash (k);
    int new_slot;
    int i;
    for (i = 0;; i++) {
        new_slot = (slot + (i / 2) + (i * i) / 2) % h->size;
        n = &((hash_node_t *) h->array)[new_slot];
        if (!n->key || n->key == &__tombstone) {
            if (!n->key) {
                h->slots_used += 1;
                h->count++;
            }
            n->key = k;
            n->val = v;
            return NULL;
        }
        else if (0 == h->compare (k, n->key)) {
            void * old;
            old = n->val;
            n->val = v;
            return old;
        }
    }
}

void hashmapq_put_entry (hashmapq_t * h, hash_entry_t * entry) {
    hashmapq_put (h, entry -> key, entry -> val);
}

void hashmapq_increase_capacity (hashmapq_t * h) {
    hash_node_t * array_old;
    int ii, asize_old;
    array_old = h->array;
    asize_old = h->size;
    h->count = 0;
    h->slots_used = 0;
    h->size <<= 1;
    h->array = alloca ((h->size) * (sizeof (hash_node_t)));
    for (ii = 0; ii < asize_old; ii++) {
        hash_node_t * n;
        n = &((hash_node_t *) array_old)[ii];
        if (!n->key || n->key == &__tombstone)
            continue;
        hashmapq_put (h, n -> key, n -> val);
    }
    free (array_old);
}

static void __ensurecapacity (hashmapq_t * h) {
    if ((float) h->slots_used / h->size < SPACERATIO) {
        return;
    }
    else {
        hashmapq_increase_capacity (h);
    }
}

void * hashmapq_iterator_peek (hashmapq_t * h, hashmapq_iterator_t * iter) {
    for (; iter->cur < h->size; iter->cur++) {
        hash_node_t * n;
        n = &((hash_node_t *) h->array)[iter->cur];
        if (n->key && n->key != &__tombstone)
            return n->key;
    }
    return NULL;
}

void * hashmapq_iterator_peek_value (hashmapq_t * h, hashmapq_iterator_t * iter) {
    return hashmapq_get (h, hashmapq_iterator_peek (h, iter));
}

int hashmapq_iterator_has_next (hashmapq_t * h, hashmapq_iterator_t * iter) {
    return NULL != hashmapq_iterator_peek (h, iter);
}

void * hashmapq_iterator_next_value (hashmapq_t * h, hashmapq_iterator_t * iter) {
    void * k;
    k = hashmapq_iterator_next (h, iter);
    if (!k)
        return NULL;
    return hashmapq_get (h, k);
}

void * hashmapq_iterator_next (hashmapq_t * h, hashmapq_iterator_t * iter) {
    hash_node_t * n;
    assert (iter);
    for (; iter->cur < h->size; iter->cur++) {
        n = &((hash_node_t *) h->array)[iter->cur];
        if (!n->key || n->key == &__tombstone)
            continue;
        iter->cur++;
        return n->key;
    }
    return NULL;
}

void hashmapq_iterator (hashmapq_t * h __attribute__ ((__unused__)), hashmapq_iterator_t * iter) {
    iter->cur = 0;
}

