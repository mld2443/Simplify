#pragma once

#include "simd.h"

#include <cstdint>    // uint64_t
#include <functional> // function


struct Vertex;
struct Edge;
struct Face;

struct Halfedge {
    Halfedge *next, *prev, *flip;
    Vertex *v;
    Edge *e;
    Face *f;

    uint64_t collapse();

    void invalidate();
    bool invalid() const;
};

struct Vertex {
    Halfedge *he;
    f32v3 pos;

    void traverseEdges(std::function<void(Halfedge*)> op) const;

    void draw() const;

    void invalidate();
    bool invalid() const;
};

struct Edge {
    Halfedge *he;
    //bool dirty, unsafe;

    f32v3 midpoint() const;

    void draw() const;

    void invalidate();
    bool invalid() const;
};

struct Face {
    Halfedge *he;

    f32v3 normal() const;
    f32v3 centroid() const;
    bool isTriangle() const;

    void traversePerimeter(std::function<void(Halfedge*)> op) const;

    void draw() const;

    void invalidate();
    bool invalid() const;
};
