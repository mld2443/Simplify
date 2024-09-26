#pragma once

#include "simd.h"

#include <GL/glut.h>
#include <vector>
#include <cmath>
#include <list>


//struct QuadraticErrorFunction;
struct Vertex;
struct Edge;
struct Face;

struct Halfedge {
    Halfedge *next, *prev, *flip;
    Vertex *v;
    Edge *e;
    Face *f;
    bool valid;

    unsigned int collapse();
};

struct Vertex {
    v3f pos;
    Halfedge *he;
    bool valid;

    std::vector<Vertex*> neighbors() const {
        std::vector<Vertex*> neighborhood;

        Halfedge *trav = he;
        do {
            neighborhood.push_back(trav->flip->v);
            trav = trav->flip->next;
        } while (trav != he);

        return neighborhood;
    }

    void update(Vertex* v) {
        valid = false;

        Halfedge *trav = he;
        do {
            trav->v = v;
            trav = trav->flip->next;
        } while(trav != he);
    }

    void markEdges();

    Vertex* operator=(const Vertex& v) { pos = v.pos; he = v.he; return this; }
};

struct Edge {
    Halfedge *he;
    bool dirty, unsafe, valid;

    v3f midpoint() const { return (he->v->pos + he->flip->v->pos)/2; }
    unsigned long collapse() { valid = false; return he->collapse(); }

    void draw() const {
        GLfloat white[] = { 1.0, 1.0, 1.0 };
        glMaterialfv(GL_FRONT, GL_AMBIENT, white);
        glBegin(GL_LINES); {
            glVertex3d(he->v->pos.x, he->v->pos.y, he->v->pos.z);
            glVertex3d(he->flip->v->pos.x, he->flip->v->pos.y, he->flip->v->pos.z);
        } glEnd();
    }
};

struct Face {
    Halfedge *he;
    bool valid;

    v3f normal() const {
        return (he->next->next->v->pos - he->v->pos).cross(he->next->next->next->v->pos - he->next->v->pos).normalize();
    }

    v3f centroid() const {
        unsigned int val = 0;
        v3f r = {0,0,0};

        Halfedge *trav = he;
        do {
            ++val;
            r += trav->v->pos;
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
                glVertex3d(trav->v->pos.x, trav->v->pos.y, trav->v->pos.z);
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
