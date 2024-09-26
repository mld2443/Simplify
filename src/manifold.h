#pragma once

#include <list>
#include <map>

#include "halfedge.h"


class Manifold {
private:
    template <typename T>
    struct AABB {
        struct Dimension {
            T lo, hi;

            Dimension();

            void addSample(T s);
            T delta() const;
            T centroid() const;
        };

        Dimension x, y, z;

        void addPoint(const v3<T>& v);
    };

    Edge* getEdge(Vertex *v1, Vertex* v2);
    bool checkSafety(const Edge *e) const;
    void collapse(Edge *e);

    int verify();

    Vertex* addPoint(v3f& p);
    void addFace(const std::list<Vertex*>& verts);

public:
    Manifold(const char* objfile, bool invert = false);

    v3f getAABBSizes() const;
    v3f getAABBCentroid() const;

    void simplify(const unsigned long count);

    void draw() const;

private:
    std::list<Vertex> vertices;
    std::list<Face> faces;
    std::list<Edge> edges;
    std::list<Halfedge> halfedges;

    std::map<std::pair<Vertex*, Vertex*>, Edge*> edge_hash;

    AABB<float> bounds;

    unsigned long deleted_faces;
};
