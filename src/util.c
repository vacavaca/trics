#include "util.h"

typedef struct {
    double x;
    double y;
} DPoint;

int sign(int a) {
    return a > 0 ? 1 : (a < 0 ? -1 : 0);
}

bool rect_contains(Rect const *rect, Point const *point) {
    return point->x >= rect->x && point->x - rect->x <= rect->width &&
           point->y >= rect->y && point->y - rect->y <= rect->height;
}

bool rect_contains_rect(Rect const *rect, Rect const *other) {
    Point a = (Point){ .x = other->x, .y = other->y };
    Point b = (Point){ .x = a.x + other->width, .y = a.y };
    Point c = (Point){ .x = b.x, .y = a.y + other->height };
    Point d = (Point){ .x = a.x, .y = c.y };

    return rect_contains(rect, &a) && rect_contains(rect, &b) &&
           rect_contains(rect, &c) && rect_contains(rect, &d);
}

bool rect_eq(Rect const *rect, Rect const *other) {
    return rect->x == other->x && rect->y == other->y &&
           rect->width == other->width && rect->height == other->height;
}

Point orthogonal(Point const *point) {
    return (Point){
        .x = point->y,
        .y = -point->x,
    };
}

Point neg(Point const *point) {
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

Point mul(Point const *point, double f) {
    return (Point){
        .x = point->x * f,
        .y = point->y * f,
    };
}

int dot(Point const *point, Point const *other) {
    return point->x * other->x + point->y * other->y;
}

int cross(Point const *point, Point const *other) {
    return point->x * other->y - point->y * other->x;
}

double length(Point const *point) {
    return sqrt((double)dot(point, point));
}

bool ray_intersects_segment(Point const *ray,
                            Point const *start, Point const *end) {
    Point o = orthogonal(ray);

    return sign(dot(start, &o)) != sign(dot(end, &o));
}

bool ray_segment_intersection(Point const *ray, Point const *start,
                              Point const *end, DPoint *inter) {
    Point nstart = neg(start);
    Point s = add(end, &nstart);

    double dv = ((double)cross(ray, &s));
    if (dv == 0) {
        return false; // parallel
    }

    double t = ((double)cross(start, &s)) / dv;
    if (t <= 0) {
        return false; // no intersection
    }

    *inter = (DPoint){
        .x = t * ray->x,
        .y = t * ray->y,
    };

    return true;
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

    return ray_intersects_segment(ray, &a, &b) ||
           ray_intersects_segment(ray, &b, &c) ||
           ray_intersects_segment(ray, &c, &d) ||
           ray_intersects_segment(ray, &d, &a);
}

bool rect_ray_intersection(Rect const *rect, Point const *origin,
                           Point const *ray, Point *inter) {
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

    DPoint di;
    if (ray_segment_intersection(ray, &a, &b, &di) ||
        ray_segment_intersection(ray, &b, &c, &di) ||
        ray_segment_intersection(ray, &c, &d, &di) ||
        ray_segment_intersection(ray, &d, &a, &di)) {
        *inter = (Point){
            .x = (int)(round(di.x + origin->x)),
            .y = (int)(round(di.y + origin->y))};
        return true;
    }

    return false;
}

double segment_distance_to(Point *const start, Point *const end,
                           Point const *point) {
    Point nstart = neg(start);
    Point nend = neg(end);
    Point s = add(end, &nstart);

    Point stp = add(point, &nstart);
    Point etp = add(point, &nend);
    Point o = orthogonal(&s);

    if (sign(dot(&stp, &s)) != sign(dot(&etp, &s))) {
        double l = (double)dot(&stp, &o) / length(&o);
        return l * sign(l);
    } else {
        double sl = length(&stp);
        double el = length(&etp);
        return sl < el ? sl : el;
    }
}

double rect_distance_to(Rect const *rect, Point const *point) {
    Point a = (Point){
        .x = rect->x,
        .y = rect->y,
    };

    Point b = (Point){
        .x = rect->x + rect->width,
        .y = rect->y,
    };

    Point c = (Point){
        .x = rect->x + rect->width,
        .y = rect->y + rect->height,
    };

    Point d = (Point){
        .x = rect->x,
        .y = rect->y + rect->height,
    };

    double min_d = segment_distance_to(&a, &b, point);
    double ds;

    ds = segment_distance_to(&b, &c, point);
    if (ds < min_d) {
        min_d = ds;
    }

    ds = segment_distance_to(&c, &d, point);
    if (ds < min_d) {
        min_d = ds;
    }

    ds = segment_distance_to(&d, &a, point);
    if (ds < min_d) {
        min_d = ds;
    }

    return min_d;
}
