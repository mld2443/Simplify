#pragma once

#include "halfedge.h"


struct element {
    Edge *e;
    float error;

    element(Edge* e, float combinedError): e(e), error(combinedError) {}
    element(const element& el) = default;

    bool dirty() const { return e->dirty; }
    bool valid() const { return e->valid; }

    element* operator=(const element& el) { error = el.error; e = el.e; return this; }
};

class elementComp {
public:
    bool operator()(const element& e1, const element& e2) {
        if (e1.error > e2.error)
            return true;
        return false;
    }
};
