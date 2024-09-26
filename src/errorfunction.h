#pragma once

#include "halfedge.h"


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
