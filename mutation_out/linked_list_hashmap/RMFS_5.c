#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <assert.h>
#include "linked_list_hashmap.h"
#define SPACERATIO 0.5
typedef struct hash_node_s hash_node_t;
struct hash_node_s {
    hash_entry_t ety;
    hash_node_t * next;
};
static void __ensurecapacity (hashmap_t * h);

static hash_node_t * __allocnodes (unsigned int count) {
    return calloc (count, sizeof (hash_node_t));
}

hashmap_t * hashmap_new (func_longhash_f hash, func_longcmp_f cmp, unsigned int initial_capacity) {
    hashmap_t * h;
    h = calloc (1, sizeof (hashmap_t));
    h->arraySize = initial_capacity;
    h->array = __allocnodes (h->arraySize);
    h->hash = hash;
    h->compare = cmp;
    return h;
}

int hashmap_count (const hashmap_t * h) {
    return h->count;
}

int hashmap_size (hashmap_t * h) {
    return h->arraySize;
}

static void __node_empty (hashmap_t * h, hash_node_t * node) {
    if (NULL == node) {
        return;
    }
    else {
        __node_empty (h, node -> next);
        free (node);
        h->count--;
    }
}

void hashmap_clear (hashmap_t * h) {
    int ii;
    for (ii = 0; ii < h->arraySize; ii++) {
        hash_node_t * node;
        node = &((hash_node_t *) h->array)[ii];
        if (NULL == node->ety.key)
            continue;
        node->ety.key = NULL;
        __node_empty (h, node -> next);
        node->next = NULL;
        h->count--;
        assert (0 <= h -> count);
    }
    assert (0 == hashmap_count (h));
}

void hashmap_free (hashmap_t * h) {
    assert (h);
    hashmap_clear (h);
}

void hashmap_freeall (hashmap_t * h) {
    assert (h);
    hashmap_free (h);
    free (h);
}

inline static unsigned int __doProbe (hashmap_t * h, const void * key) {
    return h->hash (key) % h->arraySize;
}

void * hashmap_get (hashmap_t * h, const void * key) {
    unsigned int probe;
    hash_node_t * node;
    if (0 == hashmap_count (h) || !key)
        return NULL;
    probe = __doProbe (h, key);
    node = &((hash_node_t *) h->array)[probe];
    if (NULL == node->ety.key) {
        return NULL;
    }
    else {
        do {
            if (0 == h->compare (key, node->ety.key)) {
                return (void *) node->ety.val;
            }
        }
        while ((node = node->next));
    }
    return NULL;
}

int hashmap_contains_key (hashmap_t * h, const void * key) {
    return (NULL != hashmap_get (h, key));
}

void hashmap_remove_entry (hashmap_t * h, hash_entry_t * entry, const void * key) {
    hash_node_t * n, * n_parent;
    n = &((hash_node_t *) h->array)[__doProbe (h, key)];
    if (!n->ety.key)
        goto notfound;
    n_parent = NULL;
    do {
        if (0 != h->compare (key, n->ety.key)) {
            n_parent = n;
            n = n->next;
            continue;
        }
        memcpy (entry, & n -> ety, sizeof (hash_entry_t));
        if (!n_parent) {
            if (n->next) {
                hash_node_t * tmp;
                tmp = n->next;
                memcpy (& n -> ety, & tmp -> ety, sizeof (hash_entry_t));
                n->next = tmp->next;
                free (tmp);
            }
            else {
                n->ety.key = NULL;
            }
        }
        else {
            n_parent->next = n->next;
            free (n);
        }
        h->count--;
        return;
    }
    while (n);
notfound :
    entry->key = NULL;
    entry->val = NULL;
}

void * hashmap_remove (hashmap_t * h, const void * key) {
    hash_entry_t entry;
    hashmap_remove_entry (h, & entry, key);
    return (void *) entry.val;
}

inline static void __nodeassign (hashmap_t * h, hash_node_t * node, void * key, void * val) {
    if (!node->ety.key)
        h->count++;
    assert (h -> count < 32768);
    node->ety.key = key;
    node->ety.val = val;
}

void * hashmap_put (hashmap_t * h, void * key, void * val_new) {
    hash_node_t * node;
    if (!key || !val_new)
        return NULL;
    assert (key);
    assert (val_new);
    assert (h -> array);
    __ensurecapacity (h);
    node = &((hash_node_t *) h->array)[__doProbe (h, key)];
    assert (node);
    if (NULL == node->ety.key) {
        __nodeassign (h, node, key, val_new);
    }
    else {
        do {
            if (0 == h->compare (key, node->ety.key)) {
                void * val_prev;
                val_prev = node->ety.val;
                node->ety.val = val_new;
                return val_prev;
            }
        }
        while (node->next && (node = node->next));
        node->next = __allocnodes (1);
        __nodeassign (h, node -> next, key, val_new);
    }
    return NULL;
}

void hashmap_put_entry (hashmap_t * h, hash_entry_t * entry) {
    hashmap_put (h, entry -> key, entry -> val);
}

void hashmap_increase_capacity (hashmap_t * h, unsigned int factor) {
    hash_node_t * array_old;
    int ii, asize_old;
    array_old = h->array;
    asize_old = h->arraySize;
    h->arraySize *= factor;
    h->array = __allocnodes (h->arraySize);
    h->count = 0;
    for (ii = 0; ii < asize_old; ii++) {
        hash_node_t * node;
        node = &((hash_node_t *) array_old)[ii];
        if (NULL == node->ety.key)
            continue;
        hashmap_put (h, node -> ety.key, node -> ety.val);
        node = node->next;
        while (node) {
            hash_node_t * next;
            next = node->next;
            hashmap_put (h, node -> ety.key, node -> ety.val);
            assert (NULL != node -> ety.key);
            free (node);
            node = next;
        }
    }
}

static void __ensurecapacity (hashmap_t * h) {
    if ((float) h->count / h->arraySize < SPACERATIO) {
        return;
    }
    else {
        hashmap_increase_capacity (h, 2);
    }
}

void * hashmap_iterator_peek (hashmap_t * h, hashmap_iterator_t * iter) {
    if (NULL == iter->cur_linked) {
        for (; iter->cur < h->arraySize; iter->cur++) {
            hash_node_t * node;
            node = &((hash_node_t *) h->array)[iter->cur];
            if (node->ety.key)
                return node->ety.key;
        }
        return NULL;
    }
    else {
        hash_node_t * node;
        node = iter->cur_linked;
        return node->ety.key;
    }
}

void * hashmap_iterator_peek_value (hashmap_t * h, hashmap_iterator_t * iter) {
    return hashmap_get (h, hashmap_iterator_peek (h, iter));
}

int hashmap_iterator_has_next (hashmap_t * h, hashmap_iterator_t * iter) {
    return NULL != hashmap_iterator_peek (h, iter);
}

void * hashmap_iterator_next_value (hashmap_t * h, hashmap_iterator_t * iter) {
    void * k;
    k = hashmap_iterator_next (h, iter);
    if (!k)
        return NULL;
    return hashmap_get (h, k);
}

void * hashmap_iterator_next (hashmap_t * h, hashmap_iterator_t * iter) {
    hash_node_t * n;
    assert (iter);
    if ((n = iter->cur_linked)) {
        hash_node_t * n_parent;
        n_parent = &((hash_node_t *) h->array)[iter->cur];
        if (!n_parent->next) {
            n = n_parent;
            iter->cur_linked = NULL;
            iter->cur++;
        }
        else {
            if (NULL == n->next)
                iter->cur++;
            iter->cur_linked = n->next;
        }
        return n->ety.key;
    }
    else {
        for (; iter->cur < h->arraySize; iter->cur++) {
            n = &((hash_node_t *) h->array)[iter->cur];
            if (n->ety.key)
                break;
        }
        if (h->arraySize == iter->cur) {
            return NULL;
        }
        n = &((hash_node_t *) h->array)[iter->cur];
        if (n->next) {
            iter->cur_linked = n->next;
        }
        else {
            iter->cur += 1;
        }
        return n->ety.key;
    }
}

void hashmap_iterator (hashmap_t * h __attribute__ ((__unused__)), hashmap_iterator_t * iter) {
    iter->cur = 0;
    iter->cur_linked = NULL;
}

