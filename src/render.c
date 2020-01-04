#include "render.h"

inline static signed char wy_compare(Widget *a, Widget *b) {
    return a->rect.y - b->rect.y;
}

signed char y_compare(void *a, void *b) {
    return wy_compare(a, b);
}


Container *container_init(void) {
    Container *container = malloc(sizeof(Container));
    if (container == NULL) {
        return NULL;
    }

    RefList *children = ref_list_init();
    if (children == NULL) {
        goto cleanup;
    }

    *container = (Container){
        .scroll = 0,
        .rscroll = 0,
        .children = children,
        .rendered = false
    };

    return container;

cleanup:
    free(container);
    return NULL;
}

bool container_add(Container *container, Widget *widget) {
    if (widget->parent != NULL) {
        container_remove(widget);
    }

    int i = ref_list_bin_search(container->children, widget, y_compare);
    int n = i >= 0 ? i : (-i - 1);
    return ref_list_insert(container->children, n, widget);
}

void container_remove(Widget *widget) {
    if (widget->parent == NULL) {
        return;
    }

    Container *container = widget->parent->container;
    int ndx = ref_list_bin_search(container->children, widget, y_compare);
    assert(ndx >= 0);

    ref_list_del(container->children, ndx);
    widget->parent = NULL;
}

void container_reset_scroll(Container *container) {
    container_scroll(container, -container->scroll);
}

void container_scroll(Container *container, int n) {
    container->scroll += n;
    for (int i = 0; i < container->children->length; i ++) {
        Widget *widget = ref_list_get(container->children, i);
        int ny = widget->rect.y - n;
        int x = widget->rect.x;
        widget_move(widget, x, ny);
    }
}

void container_scroll_to_last(Container *container) {
    int len = container->children->length;
    if (len == 0) {
        container_reset_scroll(container);
    }
    Widget *last = ref_list_get(container->children, len - 1);
    int bottom = last->rect.y + last->rect.height - container->rect.y;
    int ns = bottom - container->rect.height;
    container_scroll(container, ns - container->scroll);
}

void container_scroll_to_position(Container *container, int y) {
    int rel = y - container->rect.y;

    if (rel < 0) {
        container_scroll(container, rel);
    } else if (rel > container->rect.height) {
        container_scroll(container, rel + container->rect.height);
    }
}

void container_move(Container *container, int x, int y) {
    for (int i = 0; i < container->children->length; i ++) {
        Widget *widget = ref_list_get(container->children, i);
        widget_move(widget, x, y);
    }
}

void container_refresh(Container *container, Rect const *rect) {
    int ds = container->scroll - container->rscroll;
    container->rscroll = container->scroll;

    if (container->rendered && ds == 0) {
        return;
    }

    container->rendered = true;

    int len = container->children->length;
    for (int i = ds > 0 ? 0 : len - 1;
         ds > 0 ? i < len : i >= 0;
         ds > 0 ? i ++ : i --) {
        Widget *widget = ref_list_get(container->children, i);

        widget_clear(widget);

        if (rect_contains_rect(rect, &widget->rect)) {
            widget_refresh(widget);
        }
    }
}

void container_free(Container *container) {
    for (int i = 0; i < container->children->length; i ++) {
        Widget *widget = ref_list_get(container->children, i);
        widget_free(widget);
    }
    ref_list_free(container->children);
    free(container);
}

Widget *widget_init_text(RendererParams *params, Widget *parent,
                         Rect rect, char *txt) {
    Widget *widget = malloc(sizeof(Widget));
    if (widget == NULL) {
        return NULL;
    }

    Renderer *renderer = renderer_init(params, rect);
    if (renderer == NULL) {
        goto cleanup_widget;
    }

    Text text = (Text){
        .text = txt,
        .color = UI_COLOR_WHITE,
        .bold = false
    };

    *widget = (Widget){
        .renderer = renderer,
        .parent = NULL,
        .type = WIDGET_TYPE_TEXT,
        .rect = rect,
        .text = text,
        .refreshed_at = -1
    };

    if (parent != NULL) {
        assert(parent->type == WIDGET_TYPE_CONTAINER);
        if (!container_add(parent->container, widget)) {
            goto cleanup;
        }

        widget->parent = parent;
    }

    return widget;

cleanup:
    renderer_free(renderer);
cleanup_widget:
    free(widget);
    return NULL;
}

Widget *widget_init_container(RendererParams *params, Widget *parent,
                              Rect rect) {
    Widget *widget = malloc(sizeof(Widget));
    if (widget == NULL) {
        return NULL;
    }

    Renderer *renderer = renderer_init(params, rect);
    if (renderer == NULL) {
        goto cleanup_widget;
    }


    Container *container = container_init();
    if (container == NULL) {
        goto cleanup_renderer;
    }

    *widget = (Widget){
        .renderer = renderer,
        .parent = NULL,
        .type = WIDGET_TYPE_CONTAINER,
        .rect = rect,
        .container = container,
        .refreshed_at = -1
    };

    if (parent != NULL) {
        assert(parent->type == WIDGET_TYPE_CONTAINER);
        if (!container_add(parent->container, widget)) {
            goto cleanup;
        }

        widget->parent = parent;
    }

    return widget;

cleanup:
    container_free(container);
cleanup_renderer:
    renderer_free(renderer);
cleanup_widget:
    free(widget);
    return NULL;
}

void widget_move(Widget *widget, int x, int y) {
    widget->rect.x = x;
    widget->rect.y = y;

    if (widget->type == WIDGET_TYPE_CONTAINER) {
        container_move(widget->container, x, y);
    }

    renderer_move(widget->renderer, x, y);
}

void widget_clear(Widget *widget) {
    renderer_clear(widget->renderer);
}

void widget_refresh(Widget *widget) {
    if (widget->type == WIDGET_TYPE_TEXT) {
        renderer_render(widget->renderer, &widget->text);
    } else if (widget->type == WIDGET_TYPE_CONTAINER) {
        container_refresh(widget->container, &widget->rect);
    }
}

void widget_free(Widget *widget) {
    renderer_free(widget->renderer);
    if (widget->type == WIDGET_TYPE_CONTAINER) {
        container_free(widget->container);
    }

    free(widget);
}
