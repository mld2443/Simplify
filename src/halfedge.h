#pragma once

#include "simd.h"

#include <GL/glut.h>
#include <vector>
#include <cmath>
#include <list>


struct QuadraticErrorFunction;
struct Vertex;
struct Edge;
struct Face;

struct Halfedge {
    Halfedge *next, *prev, *flip;
    Face *f;
    Vertex *o;
    Edge *e;
    bool valid;

    unsigned int collapse();
};

struct QuadraticErrorFunction {
    float n;
    v3f Sv;
    float vtv;

    QuadraticErrorFunction() = default;
    QuadraticErrorFunction(float n, const v3f& Sv, float vtv): n(n), Sv(Sv), vtv(vtv) {}
    QuadraticErrorFunction(Halfedge* he);

    float eval(const v3f& v) const { return n * v.dot(v) - 2 * v.dot(Sv) + vtv; }

    QuadraticErrorFunction operator+(const QuadraticErrorFunction& q) const { return {n + q.n, Sv + q.Sv, vtv + q.vtv}; }
    QuadraticErrorFunction* operator=(const QuadraticErrorFunction& q) { n = q.n; Sv = q.Sv; vtv = q.vtv; return this; }
};

struct Vertex {
    v3f pos;
    QuadraticErrorFunction qef;
    Halfedge *he;
    bool valid;

    ~Vertex() {}

    void calcQEF() { qef = { he }; }

    std::vector<Vertex*> neighbors() const {
        std::vector<Vertex*> neighborhood;

        Halfedge *trav = he;
        do {
            neighborhood.push_back(trav->flip->o);
            trav = trav->flip->next;
        } while(trav != he);

        return neighborhood;
    }

    void update(Vertex* v) {
        valid = false;

        Halfedge *trav = he;
        do {
            trav->o = v;
            trav = trav->flip->next;
        } while(trav != he);
    }

    void markEdges();

    Vertex* operator=(const Vertex& v) { pos = v.pos; qef = v.qef; he = v.he; return this; }
};

struct Edge {
    Halfedge *he;
    bool dirty, unsafe, valid;

    v3f midpoint() const { return (he->o->pos + he->flip->o->pos)/2; }
    v3f getNewPt() const { return (he->o->qef.Sv + he->flip->o->qef.Sv)/(he->o->qef.n + he->flip->o->qef.n); }
    float getCombinedError() const { return (he->o->qef + he->flip->o->qef).eval(getNewPt()); }

    unsigned int collapse() { valid = false; return he->collapse(); }

    void draw() const {
        GLfloat white[] = { 1.0, 1.0, 1.0 };
        glMaterialfv(GL_FRONT, GL_AMBIENT, white);
        glBegin(GL_LINES); {
            glVertex3d(he->o->pos.x, he->o->pos.y, he->o->pos.z);
            glVertex3d(he->flip->o->pos.x, he->flip->o->pos.y, he->flip->o->pos.z);
        } glEnd();
    }
};

struct Face {
    Halfedge *he;
    bool valid;

    v3f normal() const {
        return (he->next->next->o->pos - he->o->pos).cross(he->next->next->next->o->pos - he->next->o->pos).normalize();
    }

    v3f centroid() const {
        unsigned int val = 0;
        v3f r = {0,0,0};

        Halfedge *trav = he;
        do {
            ++val;
            r += trav->o->pos;
            trav = trav->next;
        } while(trav != he);

        return r / val;
    }

    void draw() const {
        glBegin(GL_POLYGON); {
            v3f n = normal();
            glNormal3d(n.x, n.y, n.z);

            Halfedge *trav = he;
            do {
                glVertex3d(trav->o->pos.x, trav->o->pos.y, trav->o->pos.z);
                trav = trav->next;
            } while(trav != he);
        } glEnd();
    }
};

struct invalid {
    bool operator() (const Halfedge& h) { return !h.valid; }
    bool operator() (const Vertex& v) { return !v.valid; }
    bool operator() (const Edge& e) { return !e.valid; }
    bool operator() (const Face& f) { return !f.valid; }
};
