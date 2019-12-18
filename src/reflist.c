#include "reflist.h"


RefList *ref_list_init(void) {
    const size_t item_size = sizeof(void *);
    const size_t cap = item_size * DEFAULT_LIST_CAPACITY;
    void **array = malloc(cap);
    if (array == NULL) {
        return NULL;
    }

    RefList *list = malloc(sizeof(RefList));
    if (list == NULL) {
        free(array);
        return NULL;
    }

    *list = (RefList){
        .item_size = item_size,
        .cap = cap,
        .length = 0,
        .array = array};

    return list;
}

bool ref_list_add(RefList *list, void *item) {
    if (list->length == list->cap) {
        const size_t next_cap = list->cap * 2;
        void **next = realloc(list->array, next_cap);
        if (next == NULL) {
            return false;
        }
        list->array = next;
        list->cap = next_cap;
    }

    list->array[list->length] = item;
    list->length += 1;
    return true;
}

bool ref_list_set(RefList *list, int n, void *item) {
    if (n < 0 || n > list->length) { // allowed to add last
        return false;
    }

    if (n >= list->cap) {
        const size_t next_cap = list->cap * 2;
        void **next = realloc(list->array, next_cap);
        if (next == NULL) {
            return false;
        }
        list->array = next;
        list->cap = next_cap;
    }

    list->array[n] = item;
    if (n >= list->length) {
        list->length = n + 1;
    }
    return true;
}

bool ref_list_has(RefList *list, int n) {
    return n >= 0 && n < list->length;
}

void *ref_list_get(RefList *list, int n) {
    if (n < 0 || n >= list->length) {
        return NULL;
    }

    return list->array[n];
}

void *ref_list_last(RefList *list) {
    return ref_list_get(list, 0);
}

void *ref_list_pop(RefList *list) {
    if (list->length == 0) {
        return NULL;
    }

    int i = list->length - 1;
    void *item = list->array[i];
    list->array[i] = NULL;
    list->length -= 1;

    return item;
}

void ref_list_free(RefList *list) {
    free(list->array);
    free(list);
}
