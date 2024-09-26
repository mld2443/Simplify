#pragma once

#include "halfedge.h"

#include <list>


class Manifold {
private:
    struct AABB {
        struct Dimension {
            float lo, hi;

            Dimension();

            void addSample(float s);
            float delta() const;
            float centroid() const;
        };

        Dimension x, y, z;

        void addPoint(const v3f& v);
    };

    bool checkSafety(const Edge *e) const;

#ifndef NDEBUG
    void verify();
#endif

    Vertex* addPoint(v3f& p);
    void addFace(const std::list<Vertex*>& verts);

public:
    Manifold(const char* objfile, bool invert = false);

    v3f getAABBSizes() const;
    v3f getAABBCentroid() const;

    void draw() const;

private:
    std::list<Vertex>    m_vertices;
    std::list<Face>         m_faces;
    std::list<Edge>         m_edges;
    std::list<Halfedge> m_halfedges;

    AABB m_bounds;
};
