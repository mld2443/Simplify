//
//  manifold.h
//  Simplify
//
//  Created by Matthew Dillard on 11/9/15.
//
#pragma once

#include <list>
#include <map>

#include "halfedge.h"

class manifold {
private:
    edge* get_edge(vertex *v1, vertex* v2);
    bool checkSafety(const edge *e) const;
    void collapse(edge *e);
    int verify();

public:
    vertex* add_vert(const float x, const float y, const float z);
    void add_face(const std::list<vertex*>& verts);

    void clear();

    void cleanup();

    void simplify(const unsigned long count);

    void draw(const bool drawcontrol) const;

private:
    std::list<vertex> vertices;
    std::list<face> faces;
    std::list<edge> edges;
    std::list<halfedge> halfedges;

    std::map<std::pair<vertex*, vertex*>, edge*> edge_hash;

    unsigned long deleted_faces;
};
