#pragma once

#include <list>
#include <map>

#include "halfedge.h"


class Manifold {
private:
    Edge* get_edge(Vertex *v1, Vertex* v2);
    bool checkSafety(const Edge *e) const;
    void collapse(Edge *e);
    int verify();

public:
    Manifold() = default;

    Vertex* add_vert(const float x, const float y, const float z);
    void add_face(const std::list<Vertex*>& verts);

    void clear();

    void cleanup();

    void simplify(const unsigned long count);

    void draw() const;

private:
    std::list<Vertex> vertices;
    std::list<Face> faces;
    std::list<Edge> edges;
    std::list<Halfedge> halfedges;

    std::map<std::pair<Vertex*, Vertex*>, Edge*> edge_hash;

    unsigned long deleted_faces;
};
