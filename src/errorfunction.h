#pragma once

#include "halfedge.h"


template <class Derived>
struct QuadraticErrorFunction {
    inline float evaluateError(const f32v3& p) const { return static_cast<const Derived*>(this)->evaluateErrorImpl(p); }
    inline f32v3 minimizeError() const { return static_cast<const Derived*>(this)->minimizeErrorImpl(); }

    inline QuadraticErrorFunction operator+(const Derived& qef) const { return *static_cast<const Derived*>(this) + qef; }
};

struct DistanceQEF : public QuadraticErrorFunction<DistanceQEF> {
    float n;
    f32v3 Sv;
    float Svtv;

    DistanceQEF() = default;
    DistanceQEF(float n, const f32v3& Sv, float Svtv);
    DistanceQEF(Halfedge* he);

    float evaluateErrorImpl(const f32v3& p) const;
    f32v3 minimizeErrorImpl() const;

    DistanceQEF operator+(const DistanceQEF& qef) const;
};

struct PlaneQEF : public QuadraticErrorFunction<PlaneQEF> {
    f32v3 Snnt012, Snnt458, Snd;
    float Sd2;

    PlaneQEF() = default;
    PlaneQEF(const f32v3& Snnt012, const f32v3& Snnt458, const f32v3& Snd, float Sd2);
    PlaneQEF(Vertex* v);

    float evaluateErrorImpl(const f32v3& p) const;
    f32v3 minimizeErrorImpl() const;

    PlaneQEF operator+(const PlaneQEF& qef) const;

private:
    f32v3 Snnt345() const { return { Snnt012.y, Snnt458.x, Snnt458.y }; }
    f32v3 Snnt678() const { return { Snnt012.z, Snnt458.y, Snnt458.z }; }
};

using QEFType = DistanceQEF;


struct QEFVertex : public Vertex {
    QEFType qef;

    QEFVertex(Halfedge* he, f32v3 pos): Vertex{he, pos}, qef() {}
};


struct QEFEdge : public Edge {
    QEFType qef;
    f32v3 newPos;
    float error;
    bool dirty, unsafe;

    QEFEdge(nullptr_t): Edge{nullptr}, error(), dirty(), unsafe() {}

    void updateQEF();
    bool checkSafety() const;
    size_t collapse();
};
