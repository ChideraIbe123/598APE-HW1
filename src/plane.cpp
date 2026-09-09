#include "plane.h"

Plane::Plane(const Vector& c, Texture* t, double ya, double pi, double ro, double tx, double ty)
    : Shape(c, t, ya, pi, ro), vect(c), right(c), up(c) {
    textureX = tx;
    textureY = ty;
    setAngles(yaw, pitch, roll);
    normalMap = NULL;
    mapX = textureX;
    mapY = textureY;
}

void Plane::setAngles(double a, double b, double c) {
    yaw = a;
    pitch = b;
    roll = c;
    xcos = cos(yaw);
    xsin = sin(yaw);
    ycos = cos(pitch);
    ysin = sin(pitch);
    zcos = cos(roll);
    zsin = sin(roll);
    useCache();
}

void Plane::setYaw(double a) {
    yaw = a;
    xcos = cos(yaw);
    xsin = sin(yaw);
    useCache();
}

void Plane::setPitch(double b) {
    pitch = b;
    ycos = cos(pitch);
    ysin = sin(pitch);
    useCache();
}

void Plane::useCache() {
    vect.x = xsin * ycos * zcos + ysin * zsin;
    vect.y = ysin * zcos - xsin * ycos * zsin;
    vect.z = xcos * ycos;
    up.x = -xsin * ysin * zcos + ycos * zsin;
    up.y = ycos * zcos + xsin * ysin * zsin;
    up.z = -xcos * ysin;
    right.x = xcos * zcos;
    right.y = -xcos * zsin;
    right.z = -xsin;
    d = -vect.dot(center);
    fillCache();
}

void Plane::fillCache() {
    double denom = right.z * up.y * vect.x - right.y * up.z * vect.x - right.z * up.x * vect.y + right.x * up.z * vect.y + right.y * up.x * vect.z - right.x * up.y * vect.z;
    cache.ax = (up.z * vect.y - up.y * vect.z) / denom;
    cache.ay = (up.x * vect.z - up.z * vect.x) / denom;
    cache.az = (up.y * vect.x - up.x * vect.y) / denom;

    cache.bx = (right.y * vect.z - right.z * vect.y) / denom;
    cache.by = (right.z * vect.x - right.x * vect.z) / denom;
    cache.bz = (right.x * vect.y - right.y * vect.x) / denom;

    cache.cx = (right.z * up.y - right.y * up.z) / denom;
    cache.cy = (right.x * up.z - right.z * up.x) / denom;
    cache.cz = (right.y * up.x - right.x * up.y) / denom;
}
void Plane::setRoll(double c) {
    roll = c;
    zcos = cos(roll);
    zsin = sin(roll);
    useCache();
}

double Plane::getIntersection(Ray ray) {
    const double t = ray.vector.dot(vect);
    const double norm = vect.dot(ray.point) + d;
    const double r = -norm / t;
    return (r > 0) ? r : inf;
}

bool Plane::getLightIntersection(Ray ray, double* fill) {
    const double t = ray.vector.dot(vect);
    const double norm = vect.dot(ray.point) + d;
    const double r = -norm / t;
    if (r <= 0. || r >= 1.)
        return false;

    if (texture->opacity > 1 - 1E-6)
        return true;
    Vector C = ray.point - center;
    Vector dist(cache.ax * C.x + cache.ay * C.y + cache.az * C.z,
                cache.bx * C.x + cache.by * C.y + cache.bz * C.z,
                cache.cx * C.x + cache.cy * C.y + cache.cz * C.z);
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

void Plane::move() {
    d = -vect.dot(center);
}
void Plane::getColor(unsigned char* toFill, double* am, double* op, double* ref, Autonoma* r,
                     Ray ray, unsigned int depth) {
    Vector C = ray.point - center;
    Vector dist(cache.ax * C.x + cache.ay * C.y + cache.az * C.z,
                cache.bx * C.x + cache.by * C.y + cache.bz * C.z,
                cache.cx * C.x + cache.cy * C.y + cache.cz * C.z);
    texture->getColor(toFill, am, op, ref, fix(dist.x / textureX - .5),
                      fix(dist.y / textureY - .5));
}
unsigned char Plane::reversible() {
    return 1;
}

Vector Plane::getNormal(Vector point) {
    if (normalMap == NULL)
        return vect;
    else {
        Vector C = point - center;
        Vector dist(cache.ax * C.x + cache.ay * C.y + cache.az * C.z,
                    cache.bx * C.x + cache.by * C.y + cache.bz * C.z,
                    cache.cx * C.x + cache.cy * C.y + cache.cz * C.z);
        double am, ref, op;
        unsigned char norm[3];
        normalMap->getColor(norm, &am, &op, &ref, fix(dist.x / mapX - .5 + mapOffX),
                            fix(dist.y / mapY - .5 + mapOffY));
        Vector ret = ((norm[0] - 128) * right + (norm[1] - 128) * up + norm[2] * vect).normalize();
        return ret;
    }
}