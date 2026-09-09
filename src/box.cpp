#include "box.h"

Box::Box(const Vector& c, Texture* t, double ya, double pi, double ro, double tx, double ty)
    : Plane(c, t, ya, pi, ro, tx, ty) {}
Box::Box(const Vector& c, Texture* t, double ya, double pi, double ro, double tx)
    : Plane(c, t, ya, pi, ro, tx, tx) {}

double Box::getIntersection(Ray ray) {
    double time = Plane::getIntersection(ray);
    Vector C = ray.point + ray.vector * time - center;
Vector dist(cache.ax * C.x + cache.ay * C.y + cache.az * C.z,
            cache.bx * C.x + cache.by * C.y + cache.bz * C.z,
            cache.cx * C.x + cache.cy * C.y + cache.cz * C.z);
    if (time == inf)
        return time;
    return (((dist.x >= 0) ? dist.x : -dist.x) > textureX / 2 ||
            ((dist.y >= 0) ? dist.y : -dist.y) > textureY / 2)
               ? inf
               : time;
}

bool Box::getLightIntersection(Ray ray, double* fill) {
    const double t = ray.vector.dot(vect);
    const double norm = vect.dot(ray.point) + d;
    const double r = -norm / t;
    if (r <= 0. || r >= 1.)
        return false;
    Vector C = ray.point + ray.vector * r - center;
    Vector dist(cache.ax * C.x + cache.ay * C.y + cache.az * C.z,
                cache.bx * C.x + cache.by * C.y + cache.bz * C.z,
                cache.cx * C.x + cache.cy * C.y + cache.cz * C.z);
    if (((dist.x >= 0) ? dist.x : -dist.x) > textureX / 2 ||
        ((dist.y >= 0) ? dist.y : -dist.y) > textureY / 2)
        return false;

    if (texture->opacity > 1 - 1E-6)
        return true;
    unsigned char temp[4];
    double amb, op, ref;
    texture->getColor(temp, &amb, &op, &ref, fix(dist.x / textureX - .5),
                      fix(dist.y / textureY - .5));
    if (op > 1 - 1E-6)
        return true;
    fill[0] *= temp[0] / 255.;
    fill[1] *= temp[1] / 255.;
    fill[2] *= temp[2] / 255.;
    return false;
}