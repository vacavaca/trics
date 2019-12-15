#include "util.h"

int sign(int a) {
    return a > 0 ? 1 : (a < 0 ? -1 : 0);
}

bool rect_contains(Rect const *rect, Point const *point) {
    return point->x >= rect->x && point->x - rect->x <= rect->width &&
           point->y >= rect->y && point->y - rect->y <= rect->height;
}

Point orthogonal(Point const *point) {
    return (Point){
        .x = point->y,
        .y = -point->x,
    };
}

Point negate(Point const *point) {
    return (Point){
        .x = -point->x,
        .y = -point->y,
    };
}

Point add(Point const *point, Point const *other) {
    return (Point){
        .x = point->x + other->x,
        .y = point->y + other->y,
    };
}

int dot(Point const *point, Point const *other) {
    return point->x * other->x + point->y * other->y;
}

bool ray_intersects_segment(Point const *ray,
                            Point const *start, Point const *end) {
    Point o = orthogonal(&ray);

    return sign(dot(&start, &o)) != sign(dot(&end, &o));
}

bool rect_intersects_ray(Rect const *rect,
                         Point const *origin, Point const *ray) {
    if (rect_contains(rect, origin)) {
        return true;
    }

    Rect local_rect = (Rect){
        .x = rect->x - origin->x,
        .y = rect->y - origin->y,
        .width = rect->width,
        .height = rect->height};

    Point a = (Point){
        .x = local_rect.x,
        .y = local_rect.y,
    };

    Point b = (Point){
        .x = local_rect.x + local_rect.width,
        .y = local_rect.y,
    };

    Point c = (Point){
        .x = local_rect.x + local_rect.width,
        .y = local_rect.y + local_rect.height,
    };

    Point d = (Point){
        .x = local_rect.x,
        .y = local_rect.y + local_rect.height,
    };

    return ray_intersects_segment(&ray, &a, &b) ||
           ray_intersects_segment(&ray, &b, &c) ||
           ray_intersects_segment(&ray, &c, &d) ||
           ray_intersects_segment(&ray, &d, &a);
}

bool ray_segment_intersection(Point const *ray, Point const *start,
                              Point const *end, Point *inter) {
    Point o = orthogonal(&ray);

    if (sign(dot(&start, &o)) == sign(dot(&end, &o))) {
        return false;
    }

    
}

bool rect_ray_intersection(Rect const *rect, Point const *origin,
                           Point const *ray, Point *inter) {
    if (!rect_intersects_ray(rect, origin, ray)) {
        return false;
    }

    Rect local_rect = (Rect){
        .x = rect->x - origin->x,
        .y = rect->y - origin->y,
        .width = rect->width,
        .height = rect->height};

    Point a = (Point){
        .x = local_rect.x,
        .y = local_rect.y,
    };

    Point b = (Point){
        .x = local_rect.x + local_rect.width,
        .y = local_rect.y,
    };

    Point c = (Point){
        .x = local_rect.x + local_rect.width,
        .y = local_rect.y + local_rect.height,
    };

    Point d = (Point){
        .x = local_rect.x,
        .y = local_rect.y + local_rect.height,
    };
}