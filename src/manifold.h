#pragma once

#include <list>
#include <map>

#include "halfedge.h"


class Manifold {
private:
    struct QuadraticErrorFunction {
        float n;
        v3f Sv;
        float vtv;

        QuadraticErrorFunction() = default;
        QuadraticErrorFunction(float n, const v3f& Sv, float vtv);
        QuadraticErrorFunction(const Halfedge* he);

        float eval(const v3f& v) const;

        QuadraticErrorFunction operator+(const QuadraticErrorFunction& qef) const;
        QuadraticErrorFunction& operator=(const QuadraticErrorFunction& qef);
    };

    struct QEFVertex : public Vertex {
        QuadraticErrorFunction qef;

        void calcQEF();
    };

    static QuadraticErrorFunction& getQEF(Vertex* v);
    static v3f getNewPoint(const Edge* e);
    static float getCombinedError(const Edge* e);

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

    Edge* getEdge(const Vertex *v1, const Vertex* v2);
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
    std::list<QEFVertex> vertices;
    std::list<Face> faces;
    std::list<Edge> edges;
    std::list<Halfedge> halfedges;

    std::map<std::pair<const Vertex*, const Vertex*>, Edge*> edge_hash;

    AABB<float> bounds;

    unsigned long deleted_faces;
};
