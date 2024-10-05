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

    operator bool() const { return f != nullptr; }
};

struct Vertex {
    Halfedge *he;
    f32v3 pos;

    bool isValence3() const;

    void traverseEdges(std::function<void(Halfedge*)> op) const;

    void draw() const;

    operator bool() const { return he != nullptr; }
};

struct Edge {
    Halfedge *he;
    bool dirty, unsafe;

    f32v3 midpoint() const;

    void draw() const;

    operator bool() const { return he != nullptr; }
};

struct Face {
    Halfedge *he;

    f32v3 normal() const;
    f32v3 centroid() const;
    bool isTriangle() const;

    void traversePerimeter(std::function<void(Halfedge*)> op) const;

    void draw() const;

    operator bool() const { return he != nullptr; }
};
