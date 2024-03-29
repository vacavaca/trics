#ifndef REFLIST_H
#define REFLIST_H

#include <stdlib.h>  // malloc
#include <stdbool.h> // bool
#include <string.h> // memcpy

#define DEFAULT_LIST_CAPACITY 256

typedef struct {
    size_t item_size;
    int cap;
    int length;
    void **array;
} RefList;

RefList *ref_list_init(void);

RefList *ref_list_init_cap(int cap);

bool ref_list_add(RefList *list, void *item);

bool ref_list_set(RefList *list, int n, void *item);

bool ref_list_insert(RefList *list, int n, void *item);

bool ref_list_has(RefList *list, int n);

int ref_list_bin_search(RefList *list, void *item,
                         signed char (*cmp)(void *a, void *b));

void *ref_list_get(RefList *list, int n);

void *ref_list_last(RefList *list);

void *ref_list_pop(RefList *list);

void *ref_list_del(RefList *list, int n);

void ref_list_clear(RefList *list);

void ref_list_free(RefList *list);

#endif // REFLIST_H
