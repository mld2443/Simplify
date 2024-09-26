//
//  halfedge.h
//  Simplify
//
//  Created by Matthew Dillard on 11/9/15.
//
#pragma once

#include "simd.h"

#include <GL/glut.h>
#include <vector>
#include <cmath>
#include <list>

struct vertex;
struct edge;
struct face;

struct halfedge {
    halfedge *next, *prev, *flip;
    face *f;
    vertex *o;
    edge *e;
    bool valid;

    unsigned int collapse();
};

struct QuadraticErrorFunction {
    float n;
    v3f Sv;
    float vtv;

    QuadraticErrorFunction() = default;
    QuadraticErrorFunction(float n, const v3f& Sv, float vtv): n(n), Sv(Sv), vtv(vtv) {}
    QuadraticErrorFunction(halfedge* he);

    float eval(const v3f& v) const { return n * v.dot(v) - 2 * v.dot(Sv) + vtv; }

    QuadraticErrorFunction operator+(const QuadraticErrorFunction& q) const { return {n + q.n, Sv + q.Sv, vtv + q.vtv}; }
    QuadraticErrorFunction* operator=(const QuadraticErrorFunction& q) { n = q.n; Sv = q.Sv; vtv = q.vtv; return this; }
};

struct vertex {
    v3f pos;
    QuadraticErrorFunction qef;
    halfedge *he;
    bool valid;

    ~vertex() {}

    void calcQEF() { qef = { he }; }

    std::vector<vertex*> neighbors() const {
        std::vector<vertex*> neighborhood;

        halfedge *trav = he;
        do {
            neighborhood.push_back(trav->flip->o);
            trav = trav->flip->next;
        } while(trav != he);

        return neighborhood;
    }

    void update(vertex* v) {
        valid = false;

        halfedge *trav = he;
        do {
            trav->o = v;
            trav = trav->flip->next;
        } while(trav != he);
    }

    void markEdges();

    vertex* operator=(const vertex& v) { pos = v.pos; qef = v.qef; he = v.he; return this; }
};

struct edge {
    halfedge *he;
    bool dirty, unsafe, valid;

    v3f midpoint() const { return (he->o->pos + he->flip->o->pos)/2; }
    v3f getNewPt() const { return (he->o->qef.Sv + he->flip->o->qef.Sv)/(he->o->qef.n + he->flip->o->qef.n); }
    float getCombinedError() const { return (he->o->qef + he->flip->o->qef).eval(getNewPt()); }

    unsigned int collapse() { valid = false; return he->collapse(); }

    void draw() const {
        GLfloat white[] = {1.0,1.0,1.0};
        glMaterialfv(GL_FRONT, GL_AMBIENT, white);
        glBegin(GL_LINES); {
            glVertex3d(he->o->pos.x, he->o->pos.y, he->o->pos.z);
            glVertex3d(he->flip->o->pos.x, he->flip->o->pos.y, he->flip->o->pos.z);
        } glEnd();
    }
};

struct face {
    halfedge *he;
    bool valid;

    v3f normal() const {
        return (he->next->next->o->pos - he->o->pos).cross(he->next->next->next->o->pos - he->next->o->pos).normalize();
    }

    v3f centroid() const {
        unsigned int val = 0;
        v3f r = {0,0,0};

        halfedge *trav = he;
        do {
            ++val;
            r += trav->o->pos;
            trav = trav->next;
        } while(trav != he);

        return r/val;
    }

    void draw() const {
        glBegin(GL_POLYGON); {
            v3f n = normal();
            glNormal3d(n.x, n.y, n.z);

            halfedge *trav = he;
            do {
                glVertex3d(trav->o->pos.x, trav->o->pos.y, trav->o->pos.z);
                trav = trav->next;
            } while(trav != he);
        } glEnd();
    }
};

struct invalid {
    bool operator() (const halfedge& h) { return !h.valid; }
    bool operator() (const vertex& v) { return !v.valid; }
    bool operator() (const edge& e) { return !e.valid; }
    bool operator() (const face& f) { return !f.valid; }
};
