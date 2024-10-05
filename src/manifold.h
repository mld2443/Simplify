#pragma once

#include "halfedge.h"

#include <list>


template <class VertexType = Vertex>
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

        void addSample(const f32v3& v);
    };

    void addFace(const std::list<Vertex*>& verts);

protected:
#ifndef NDEBUG
    void verifyConnections();
#endif

public:
    Manifold(const char* objfile);

    f32v3 getAABBSizes() const;
    f32v3 getAABBCentroid() const;

    void drawFaces() const;
    void drawEdges() const;
    void drawVertices() const;

    void saveToFile(const char* filename) const;

protected:
    std::list<VertexType> m_vertices;
    std::list<Face>          m_faces;
    std::list<Edge>          m_edges;
    std::list<Halfedge>  m_halfedges;

private:
    AABB m_bounds;
    bool m_trianglesOnly;
};
