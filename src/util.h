#ifndef UTIL_H
#define UTIL_H

#include <stdbool.h> // bool
#include <math.h> // sqrt

#define MAX(a, b) (a >= b ? a : b)
#define MIN(a, b) (a <= b ? a : b)
#define CLAMP(a, n, x) MIN(MAX(a, n), x)
#define NORM(a, n, x) ((CLAMP((float)a, n, x) - n) / (x - n))

// error handling
#define CHECK_N(e, l) if (e == NULL) { goto l; }
#define CHECK(e, l) if (!e) { goto l; }
#define FREE_N(f, e) if (e != NULL) { f(e); }

#define PI 3.14159265359

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

bool rect_contains_rect(Rect const *rect, Rect const *other);

bool rect_eq(Rect const *rect, Rect const *other);

double length(Point const *point);

bool rect_intersects_ray(Rect const *rect, Point const *origin, Point const *ray);

bool rect_ray_intersection(Rect const *rect, Point const *origin,
                           Point const *ray, Point *inter);

double rect_distance_to(Rect const * rect, Point const * point);

#endif //UTIL_H
