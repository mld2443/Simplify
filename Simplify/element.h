//
//  element.h
//  Simplify
//
//  Created by Matthew Dillard on 11/30/15.
//

#ifndef element_h
#define element_h

#include "halfedge.h"

struct element {
    float value;
    edge *e;
    bool dirty, invalid;
    
    element(edge& _e): value(_e.getCombinedError()), e(&_e), dirty(false), invalid(false) {}
    element(const element& el): value(el.value), e(el.e), dirty(el.dirty), invalid(el.invalid) {}
    
    element* operator=(const element& el) {
        value = el.value;
        e = el.e;
        dirty = el.dirty;
        invalid = el.invalid;
        return this;
    }
};

class elementComp {
public:
    bool operator()(const element& e1, const element& e2) {
        if (e1.value > e2.value)
            return true;
        return false;
    }
};

#endif /* element_h */
