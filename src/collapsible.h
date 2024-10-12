#pragma once

#include "manifold.h"
#include "errorfunction.h"


class Collapsible : public Manifold<QEFVertex, QEFEdge> {
public:
    Collapsible(const char* objfile);

    void simplify(uint64_t finalCount);

private:
    size_t m_removedCount;
};
