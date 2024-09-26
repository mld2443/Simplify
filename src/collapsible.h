#pragma once

#include "manifold.h"

#include <map>


class Collapsible {
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

    Edge* getEdge(const Vertex *v1, const Vertex* v2);
    bool checkSafety(const Edge *e) const;
    void collapse(Edge *e);

    int verify();

    Vertex* addPoint(v3f& p);
    void addFace(const std::list<Vertex*>& verts);

public:
    Collapsible(const char* objfile, bool invert = false);

    v3f getAABBSizes() const;
    v3f getAABBCentroid() const;

    void simplify(const unsigned long count);

    void draw() const;

private:
    std::list<QEFVertex> vertices;
    std::list<Face> faces;
    std::list<Edge> edges;
    std::list<Halfedge> halfedges;

    std::map<std::pair<const Vertex*, const Vertex*>, Edge*> edgeHash;

    AABB bounds;

    unsigned long countDeletedFaces;
};
