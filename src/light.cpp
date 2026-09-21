
#include "light.h"
#include "camera.h"
#include "shape.h"
#include "triangle.h"

Light::Light(const Vector& cente, unsigned char* colo) : center(cente) {
    color = colo;
}

unsigned char* Light::getColor(unsigned char a, unsigned char b, unsigned char c) {
    unsigned char* r = (unsigned char*)malloc(sizeof(unsigned char) * 3);
    r[0] = a;
    r[1] = b;
    r[2] = c;
    return r;
}

Autonoma::Autonoma(const Camera& c) : camera(c) {
    listStart = NULL;
    listEnd = NULL;
    lightStart = NULL;
    lightEnd = NULL;
    groupStart = NULL;
    depth = 10;
    skybox = BLACK;
}

Autonoma::Autonoma(const Camera& c, Texture* tex) : camera(c) {
    listStart = NULL;
    listEnd = NULL;
    lightStart = NULL;
    lightEnd = NULL;
    groupStart = NULL;
    depth = 10;
    skybox = tex;
}

bool rayHitsBox(const Ray& ray, const Vector& minB, const Vector& maxB) {
    double t1 = (minB.x - ray.point.x) / ray.vector.x;
    double t2 = (maxB.x - ray.point.x) / ray.vector.x;
    double tlo = (t1 < t2) ? t1 : t2;
    double thi = (t1 < t2) ? t2 : t1;
    t1 = (minB.y - ray.point.y) / ray.vector.y;
    t2 = (maxB.y - ray.point.y) / ray.vector.y;
    if (t1 > t2) {
        double tmp = t1;
        t1 = t2;
        t2 = tmp;
    }
    if (t1 > tlo)
        tlo = t1;
    if (t2 < thi)
        thi = t2;
    t1 = (minB.z - ray.point.z) / ray.vector.z;
    t2 = (maxB.z - ray.point.z) / ray.vector.z;
    if (t1 > t2) {
        double tmp = t1;
        t1 = t2;
        t2 = tmp;
    }
    if (t1 > tlo)
        tlo = t1;
    if (t2 < thi)
        thi = t2;
    return tlo <= thi && thi >= 0.;
}

void Autonoma::addShape(Shape* r) {
    ShapeNode* hi = (ShapeNode*)malloc(sizeof(ShapeNode));
    hi->data = r;
    hi->next = hi->prev = NULL;
    if (listStart == NULL) {
        listStart = listEnd = hi;
    } else {
        listEnd->next = hi;
        hi->prev = listEnd;
        listEnd = hi;
    }
}

void Autonoma::removeShape(ShapeNode* s) {
    if (s == listStart) {
        if (s == listEnd) {
            listStart = listStart = NULL;
        } else {
            listStart = s->next;
            listStart->prev = NULL;
        }
    } else if (s == listEnd) {
        listEnd = s->prev;
        listEnd->next = NULL;
    } else {
        ShapeNode *b4 = s->prev, *aft = s->next;
        b4->next = aft;
        aft->prev = b4;
    }
    free(s);
}

void Autonoma::addLight(Light* r) {
    LightNode* hi = (LightNode*)malloc(sizeof(LightNode));
    hi->data = r;
    hi->next = hi->prev = NULL;
    if (lightStart == NULL) {
        lightStart = lightEnd = hi;
    } else {
        lightEnd->next = hi;
        hi->prev = lightEnd;
        lightEnd = hi;
    }
}

void Autonoma::removeLight(LightNode* s) {
    if (s == lightStart) {
        if (s == lightEnd) {
            lightStart = lightStart = NULL;
        } else {
            lightStart = s->next;
            lightStart->prev = NULL;
        }
    } else if (s == lightEnd) {
        lightEnd = s->prev;
        lightEnd->next = NULL;
    } else {
        LightNode *b4 = s->prev, *aft = s->next;
        b4->next = aft;
        aft->prev = b4;
    }
    free(s);
}

void getLight(double* tColor, Autonoma* aut, Vector point, Vector norm, unsigned char flip) {
    tColor[0] = tColor[1] = tColor[2] = 0.;
    LightNode* t = aut->lightStart;
    while (t != NULL) {
        double lightColor[3];
        lightColor[0] = t->data->color[0] / 255.;
        lightColor[1] = t->data->color[1] / 255.;
        lightColor[2] = t->data->color[2] / 255.;
        Vector ra = t->data->center - point;
        ShapeNode* shapeIter = aut->listStart;
        bool hit = false;
        Ray shadowRay(point + ra * .01, ra);
        while (!hit && shapeIter != NULL) {
            hit = shapeIter->data->getLightIntersection(shadowRay, lightColor);
            shapeIter = shapeIter->next;
        }
        TriangleGroup* g = aut->groupStart;
        while (!hit && g != NULL) {
            if (rayHitsBox(shadowRay, g->minB, g->maxB)) {
                for (int i = 0; !hit && i < g->count; i++)
                    hit = g->tris[i]->getLightIntersection(shadowRay, lightColor);
            }
            g = g->next;
        }
        double perc = (norm.dot(ra) / (ra.mag() * norm.mag()));
        if (!hit) {
            if (flip && perc < 0)
                perc = -perc;
            if (perc > 0) {

                tColor[0] += perc * (lightColor[0]);
                tColor[1] += perc * (lightColor[0]);
                tColor[2] += perc * (lightColor[0]);
                if (tColor[0] > 1.)
                    tColor[0] = 1.;
                if (tColor[1] > 1.)
                    tColor[1] = 1.;
                if (tColor[2] > 1.)
                    tColor[2] = 1.;
            }
        }
        t = t->next;
    }
}
