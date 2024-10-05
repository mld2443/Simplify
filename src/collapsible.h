#pragma once

#include "manifold.h"
#include "errorfunction.h"


class Collapsible : public Manifold<QEFVertex> {
private:
    static QuadraticErrorFunction& getQEF(Vertex* v);
    static f32v3 getNewPoint(const Edge* e);
    static float getCombinedError(const Edge* e);
    static bool checkSafety(const Edge *e);

    Vertex* collapse(Edge *e);

public:
    Collapsible(const char* objfile);

    void simplify(uint64_t finalCount);

private:
    size_t m_removedCount;
};
