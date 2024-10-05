#include "collapsible.h"

#include <iostream>  // cout
#include <algorithm> // sort, set_intersection
#include <queue>     // priority_queue
#include <fstream>   // ifstream
#include <string>    // string, getline
#include <cctype>    // isdigit
#include <limits>    // numeric_limits::min, max

using namespace std;


/////////////////
// QEF, Vertex //
/////////////////
QuadraticErrorFunction::QuadraticErrorFunction(float n, const f32v3& Sv, float Svtv): n(n), Sv(Sv), Svtv(Svtv) {
}

QuadraticErrorFunction::QuadraticErrorFunction(Halfedge* he) : QuadraticErrorFunction() {
    he->v->traverseEdges([&](Halfedge* it) {
        ++n;
        Sv += it->flip->v->pos;
        Svtv += it->flip->v->pos.dot(it->flip->v->pos);
    });
}

float QuadraticErrorFunction::eval(const f32v3& v) const {
    return n * v.dot(v) - 2 * v.dot(Sv) + Svtv;
}

QuadraticErrorFunction QuadraticErrorFunction::operator+(const QuadraticErrorFunction& qef) const {
    return { n + qef.n, Sv + qef.Sv, Svtv + qef.Svtv };
}


// Cannot be part of the constructor because the vertices are made before all other components
void QEFVertex::calcQEF() {
    qef = { he };
}


QuadraticErrorFunction& Collapsible::getQEF(Vertex *v) {
    return static_cast<QEFVertex*>(v)->qef;
}

f32v3 Collapsible::getNewPoint(const Edge *e) {
    return (getQEF(e->he->v).Sv + getQEF(e->he->flip->v).Sv)/(getQEF(e->he->v).n + getQEF(e->he->flip->v).n);
}

float Collapsible::getCombinedError(const Edge* e) {
    return (getQEF(e->he->v) + getQEF(e->he->flip->v)).eval(getNewPoint(e));
}


/////////////////
// Collapsible //
/////////////////
// PRIVATE FUNCTIONS

bool Collapsible::checkSafety(const Edge *e) {
    if (e->he->f->degree() <= 3ul && e->he->prev->v->degree() <= 3ul)
        return false;
    if (e->he->flip->f->degree() <= 3ul && e->he->flip->prev->v->degree() <= 3ul)
        return false;
    return true;
}

Vertex* Collapsible::collapse(Edge *e) {
    auto combinedError(getQEF(e->he->v) + getQEF(e->he->flip->v));

    // Get the new point and calculate its position before altering the topology
    Vertex *combined = e->he->v;
    combined->pos = getNewPoint(e);

    // Collapse the triangle and record how many faces we removed
    m_countDeletedFaces += e->he->collapse();


    getQEF(combined) = combinedError;
    return combined;
}


// PUBLIC FUNCTIONS

Collapsible::Collapsible(const char* objfile, bool invert)
  : Manifold(objfile, invert)
  , m_countDeletedFaces(0ul) {
    for (auto &v : m_vertices)
        v.calcQEF();
}

void Collapsible::simplify(const size_t count) {
    struct EdgeWithError {
        Edge *e;
        float error;

        EdgeWithError(Edge* e): e(e), error(getCombinedError(e)) {}
        EdgeWithError(const EdgeWithError& e) = default;

        auto operator<=>(const EdgeWithError& e) const { return error <=> e.error; }
    };

    // Populate the priority queue
    priority_queue<EdgeWithError, vector<EdgeWithError>, greater<EdgeWithError>> errors;
    for (auto &e : m_edges) {
        errors.emplace(&e);
    }

    // The heart of the algorithm
    while (m_countDeletedFaces + count < m_faces.size()) {
        if (errors.empty()) { // No more safe edges! Panic!
            break;
        } else {
            Edge *top = errors.top().e;

            if (top->he == nullptr) {
                // This edge has been removed during a collapse, remove it here too
                errors.pop();
            } else if (top->dirty) {
                // Error has been increased, recalculate it
                errors.pop();
                top->dirty = false;
                errors.emplace(top);
            } else if (!checkSafety(top)) {
                // Unsafe edge, remove it and add it back if a neighbor collapses
                top->unsafe = true;
                errors.pop();
            } else { // Collapse it!
                auto v = collapse(top);

                v->traverseEdges([&](Halfedge* he) {
                    he->e->dirty = true;
                    if (he->e->unsafe) {
                        he->e->unsafe = false;
                        errors.emplace(he->e);
                    }
                });
            }
        }
    }

#ifndef NDEBUG
    verify();
#endif

    m_vertices.remove_if(invalid());
    m_faces.remove_if(invalid());
    m_edges.remove_if(invalid());
    m_halfedges.remove_if(invalid());
}
