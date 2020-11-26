// SPDX-License-Identifier: BSL-1.0
#ifndef TERMPAINT_TERMPAINT_HASH_INCLUDED
#define TERMPAINT_TERMPAINT_HASH_INCLUDED

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

// internal header, not api or abi stable

// pointers to this are stable as long as the item is kept in the hash
typedef struct termpaint_hash_item_ {
    unsigned char* text;
    bool unused;
    struct termpaint_hash_item_ *next;
} termpaint_hash_item;

typedef struct termpaint_hash_ {
    int count;
    int allocated;
    termpaint_hash_item** buckets;
    int item_size;
    void (*gc_mark_cb)(struct termpaint_hash_*);
    void (*destroy_cb)(struct termpaint_hash_item_*);
} termpaint_hash;


static uint32_t termpaintp_hash_fnv1a(const unsigned char* text) {
    uint32_t hash = 2166136261;
    for (; *text; ++text) {
            hash = hash ^ *text;
            hash = hash * 16777619;
    }
    return hash;
}

static bool termpaintp_hash_grow(termpaint_hash* p) {
    int old_allocated = p->allocated;
    termpaint_hash_item** old_buckets = p->buckets;
    p->allocated *= 2;
    p->buckets = (termpaint_hash_item**)calloc(p->allocated, sizeof(*p->buckets));
    if (!p->buckets) {
        p->allocated = old_allocated;
        p->buckets = old_buckets;
        return false;
    }

    for (int i = 0; i < old_allocated; i++) {
        termpaint_hash_item* item_it = old_buckets[i];
        while (item_it) {
            termpaint_hash_item* item = item_it;
            item_it = item->next;

            uint32_t bucket = termpaintp_hash_fnv1a(item->text) % p->allocated;
            item->next = p->buckets[bucket];
            p->buckets[bucket] = item;
        }
    }
    free(old_buckets);
    return true;
}

static int termpaintp_hash_gc(termpaint_hash* p) {
    if (!p->gc_mark_cb) {
        return 0;
    }

    int items_removed = 0;

    for (int i = 0; i < p->allocated; i++) {
        termpaint_hash_item* item_it = p->buckets[i];
        while (item_it) {
            item_it->unused = true;
            item_it = item_it->next;
        }
    }

    p->gc_mark_cb(p);

    for (int i = 0; i < p->allocated; i++) {
        termpaint_hash_item** prev_ptr = &p->buckets[i];
        termpaint_hash_item* item = *prev_ptr;
        while (item) {
            termpaint_hash_item* old = item;
            if (item->unused) {
                *prev_ptr = item->next;
            } else {
                prev_ptr = &item->next;
            }
            item = item->next;
            if (old->unused) {
                --p->count;
                if (p->destroy_cb) {
                    p->destroy_cb(old);
                }
                free(old->text);
                free(old);
                ++items_removed;
            }
        }
    }
    return items_removed;
}

static void* termpaintp_hash_ensure(termpaint_hash* p, const unsigned char* text) {
    if (!p->allocated) {
        p->allocated = 32;
        p->buckets = (termpaint_hash_item**)calloc(p->allocated, sizeof(termpaint_hash_item*));
        if (!p->buckets) {
            p->allocated = 0;
            return NULL;
        }
    }
    uint32_t bucket = termpaintp_hash_fnv1a(text) % p->allocated;

    if (p->buckets[bucket]) {
        termpaint_hash_item* item = p->buckets[bucket];
        termpaint_hash_item* prev = item;
        while (item) {
            prev = item;
            if (strcmp((const char*)text, (char*)item->text) == 0) {
                return item;
            }
            item = item->next;
        }
        if (p->allocated / 2 <= p->count) {
            if (termpaintp_hash_gc(p) == 0) {
                if (!termpaintp_hash_grow(p)) {
                    return NULL;
                }
            }
            // either termpaintp_hash_gc or termpaintp_hash_grow have invalidated `prev` but now capacity is free
            return termpaintp_hash_ensure(p, text);
        } else {
            item = (termpaint_hash_item*)calloc(1, p->item_size);
            if (!item) {
                return NULL;
            }
            item->text = (unsigned char*)strdup((const char*)text);
            if (!item->text) {
                free(item);
                return NULL;
            }
            prev->next = item;
            p->count++;
            return item;
        }
    } else {
        if (p->allocated / 2 <= p->count && termpaintp_hash_gc(p) == 0) {
            if (!termpaintp_hash_grow(p)) {
                return NULL;
            }
            return termpaintp_hash_ensure(p, text);
        } else {
            termpaint_hash_item* item = (termpaint_hash_item*)calloc(1, p->item_size);
            if (!item) {
                return NULL;
            }
            item->text = (unsigned char*)strdup((const char*)text);
            if (!item->text) {
                free(item);
                return NULL;
            }
            p->count++;
            p->buckets[bucket] = item;
            return item;
        }
    }
}

static void* termpaintp_hash_get(termpaint_hash* p, const unsigned char* text) {
    if (!p->allocated) {
        return NULL;
    }
    uint32_t bucket = termpaintp_hash_fnv1a(text) % p->allocated;

    if (p->buckets[bucket]) {
        termpaint_hash_item* item = p->buckets[bucket];
        while (item) {
            if (strcmp((const char*)text, (char*)item->text) == 0) {
                return item;
            }
            item = item->next;
        }
    }
    return NULL;
}

static void termpaintp_hash_destroy(termpaint_hash* p) {
    for (int i = 0; i < p->allocated; i++) {
        termpaint_hash_item* item = p->buckets[i];
        while (item) {
            termpaint_hash_item* old = item;
            item = item->next;
            if (p->destroy_cb) {
                p->destroy_cb(old);
            }
            free(old->text);
            free(old);
        }
    }
    free(p->buckets);
    p->buckets = (termpaint_hash_item**)0;
    p->allocated = 0;
    p->count = 0;
}


#endif
