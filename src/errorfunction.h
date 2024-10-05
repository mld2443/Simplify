#pragma once

#include "halfedge.h"


struct QuadraticErrorFunction {
    float n;
    v3f Sv;
    float Svtv;

    QuadraticErrorFunction() = default;
    QuadraticErrorFunction(float n, const v3f& Sv, float Svtv);
    QuadraticErrorFunction(Halfedge* he);

    float eval(const v3f& v) const;

    QuadraticErrorFunction operator+(const QuadraticErrorFunction& qef) const;
};


struct QEFVertex : public Vertex {
    QuadraticErrorFunction qef;

    void calcQEF();
};
