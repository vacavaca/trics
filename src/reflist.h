#ifndef REFLIST_H
#define REFLIST_H

#include <stdlib.h>  // malloc
#include <stdbool.h> // bol

#define DEFAULT_LIST_CAPACITY 64

typedef struct {
    size_t item_size;
    int cap;
    int length;
    void **array;
} RefList;

RefList *ref_list_init();

bool ref_list_add(RefList *list, void *item);

bool ref_list_set(RefList *list, int n, void *item);

bool ref_list_has(RefList *list, int n);

void *ref_list_get(RefList *list, int n);

void *ref_list_last(RefList *list);

void *ref_list_pop(RefList *list);

void ref_list_free(RefList *list);

#endif // REFLIST_H
