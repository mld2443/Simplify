#pragma once

#include "halfedge.h"


struct element {
    float value;
    Edge *e;

    element(Edge *e): value(e->getCombinedError()), e(e) {}
    element(const element& el): value(el.value), e(el.e) {}

    bool dirty() const { return e->dirty; }
    bool valid() const { return e->valid; }

    element* operator=(const element& el) { value = el.value; e = el.e; return this; }
};

class elementComp {
public:
    bool operator()(const element& e1, const element& e2) {
        if (e1.value > e2.value)
            return true;
        return false;
    }
};
