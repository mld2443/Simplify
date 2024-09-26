#pragma once

#include "simd.h"

#include <vector>
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
    Halfedge *he;
    v3f pos;
    bool valid;

    std::vector<Vertex*> neighbors() const;

    void update(Vertex* v);

    void markEdges();

    void draw() const;
};

struct Edge {
    Halfedge *he;
    bool dirty, unsafe, valid;

    size_t collapse();

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
