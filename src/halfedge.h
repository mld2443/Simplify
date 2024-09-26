#pragma once

#include "simd.h"

#include <GL/glut.h>
#include <vector>
#include <cmath>
#include <list>
#include <functional>


struct Vertex;
struct Edge;
struct Face;

struct Halfedge {
    Halfedge *next, *prev, *flip;
    Vertex *v;
    Edge *e;
    Face *f;
    bool valid;

    size_t collapse();

    void traverseVertex(std::function<void(Halfedge*)> op);
    void traverseFace(std::function<void(Halfedge*)> op);
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

    void draw() const;

    Vertex* operator=(const Vertex& v) { pos = v.pos; he = v.he; return this; }
};

struct Edge {
    Halfedge *he;
    bool dirty, unsafe, valid;

    size_t collapse() { valid = false; return he->collapse(); }

    v3f midpoint() const;

    void draw() const;
};

struct Face {
    Halfedge *he;
    bool valid;

    v3f normal() const;
    v3f centroid() const;

    void draw() const;
};

struct invalid {
    bool operator() (const Halfedge& h) { return !h.valid; }
    bool operator() (const Vertex& v) { return !v.valid; }
    bool operator() (const Edge& e) { return !e.valid; }
    bool operator() (const Face& f) { return !f.valid; }
};
