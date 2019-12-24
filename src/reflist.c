#include "reflist.h"


RefList *ref_list_init(void) {
    const size_t item_size = sizeof(void *);
    const size_t cap = DEFAULT_LIST_CAPACITY;
    void **array = malloc(cap * item_size);
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
    if (list->length >= list->cap) {
        const size_t next_cap = list->cap * 2;
        void **next = realloc(list->array, next_cap * list->item_size);
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
        void **next = realloc(list->array, next_cap * list->item_size);
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

bool ref_list_insert(RefList *list, int n, void *item) {
    if (n < 0 || n > list->length) {
        return false;
    }

    if (n == list->length) {
        return ref_list_set(list, n, item);
    }

    if (list->length + 1 >= list->cap) {
        const size_t next_cap = list->cap * 2;
        void **next = realloc(list->array, next_cap * list->item_size);
        if (next == NULL) {
            return false;
        }
        list->array = next;
        list->cap = next_cap;
    }

    memcpy(list->array + n + 1, list->array + n,
           (list->length - n) * list->item_size);
    list->array[n] = item;

    list->length += 1;
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

void *ref_list_del(RefList *list, int n) {
    if (n < 0 || n >= list->length) {
        return NULL;
    }

    if (n == 0 && list->length == 1) {
        void *item = list->array[n];
        list->array[0] = NULL;
        list->length = 0;
        return item;
    }

    void *item = list->array[n];
    memcpy(list->array + n, list->array + n + 1,
           (list->length - n - 1) * list->item_size);
    list->array[list->length - 1] = NULL;
    list->length -= 1;
    return item;
}

void ref_list_clear(RefList *list) {
    list->length = 0;
}

void ref_list_free(RefList *list) {
    free(list->array);
    free(list);
}
