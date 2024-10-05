#pragma once

#include "simd.h"

#include <functional>
#include <cstdint>


struct Vertex;
struct Edge;
struct Face;

struct Halfedge {
    Halfedge *next, *prev, *flip;
    Vertex *v;
    Edge *e;
    Face *f;

    uint64_t collapse();
};

struct Vertex {
    Halfedge *he;
    f32v3 pos;

    uint64_t degree() const;

    void traverseEdges(std::function<void(Halfedge*)> op) const;

    void draw() const;
};

struct Edge {
    Halfedge *he;
    bool dirty, unsafe;

    f32v3 midpoint() const;

    void draw() const;
};

struct Face {
    Halfedge *he;

    f32v3 normal() const;
    f32v3 centroid() const;
    uint64_t degree() const;

    void traversePerimeter(std::function<void(Halfedge*)> op) const;

    void draw() const;
};

struct invalid {
    bool operator() (const Halfedge& h) { return h.f==nullptr; }
    bool operator() (const Vertex& v) { return v.he==nullptr; }
    bool operator() (const Edge& e) { return e.he==nullptr; }
    bool operator() (const Face& f) { return f.he==nullptr; }
};
