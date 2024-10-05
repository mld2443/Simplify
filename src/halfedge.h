#pragma once

#include "simd.h"

#include <vector>
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

    void traverseVertex(std::function<void(Halfedge*)> op);
    void traverseFace(std::function<void(Halfedge*)> op);
};

struct Vertex {
    Halfedge *he;
    v3f pos;

    std::vector<Vertex*> neighbors() const;

    void draw() const;
};

struct Edge {
    Halfedge *he;
    bool dirty, unsafe;

    v3f midpoint() const;

    void draw() const;
};

struct Face {
    Halfedge *he;

    v3f normal() const;
    v3f centroid() const;

    void draw() const;
};

struct invalid {
    bool operator() (const Halfedge& h) { return h.f==nullptr; }
    bool operator() (const Vertex& v) { return v.he==nullptr; }
    bool operator() (const Edge& e) { return e.he==nullptr; }
    bool operator() (const Face& f) { return f.he==nullptr; }
};
