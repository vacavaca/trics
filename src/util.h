#ifndef UTIL_H
#define UTIL_H

#include <stdbool.h> // bool
#include <math.h> // sqrt

#define MAX(a, b) (a >= b ? a : b)
#define MIN(a, b) (a <= b ? a : b)
#define CLAMP(a, n, x) MIN(MAX(a, n), x)

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

double length(Point const *point);

bool rect_intersects_ray(Rect const *rect, Point const *origin, Point const *ray);

bool rect_ray_intersection(Rect const *rect, Point const *origin,
                           Point const *ray, Point *inter);

double rect_distance_to(Rect const * rect, Point const * point);

#endif //UTIL_H
