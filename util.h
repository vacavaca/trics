#ifndef UTIL_H
#define UTIL_H

#include <stdbool.h> // bool

int sign(int a);

typedef struct {
    int x;
    int y;
} Point;

typedef struct {
    int x;
    int y;
    int width;
    int height;
} Rect;

bool rect_contains(Rect const *rect, Point const *point);

bool rect_intersects_ray(Rect const *rect, Point const *origin, Point const *ray);

bool rect_ray_intersection(Rect const *rect, Point const *origin,
                           Point const *ray, Point *inter);

#endif //UTIL_H