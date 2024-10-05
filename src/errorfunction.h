#pragma once

#include "halfedge.h"


struct QuadraticErrorFunction {
    float n;
    f32v3 Sv;
    float Svtv;

    QuadraticErrorFunction() = default;
    QuadraticErrorFunction(float n, const f32v3& Sv, float Svtv);
    QuadraticErrorFunction(Halfedge* he);

    float eval(const f32v3& v) const;

    QuadraticErrorFunction operator+(const QuadraticErrorFunction& qef) const;
};


struct QEFVertex : public Vertex {
    QuadraticErrorFunction qef;

    void calcQEF();
};
