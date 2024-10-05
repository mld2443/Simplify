#pragma once

#include "manifold.h"
#include "errorfunction.h"


class Collapsible : public Manifold<QEFVertex> {
private:
    static QuadraticErrorFunction& getQEF(Vertex* v);
    static v3f getNewPoint(const Edge* e);
    static float getCombinedError(const Edge* e);

    bool checkSafety(const Edge *e) const;
    Vertex* collapse(Edge *e);

public:
    Collapsible(const char* objfile, bool invert = false);

    void simplify(const unsigned long count);

private:
    size_t m_countDeletedFaces;
};
