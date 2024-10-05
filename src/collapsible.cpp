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
QuadraticErrorFunction::QuadraticErrorFunction(float n, const v3f& Sv, float Svtv): n(n), Sv(Sv), Svtv(Svtv) {
}

QuadraticErrorFunction::QuadraticErrorFunction(Halfedge* he) : QuadraticErrorFunction() {
    he->traverseVertex([&](Halfedge* it) {
        ++n;
        Sv += it->flip->v->pos;
        Svtv += it->flip->v->pos.dot(it->flip->v->pos);
    });
}

float QuadraticErrorFunction::eval(const v3f& v) const {
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

v3f Collapsible::getNewPoint(const Edge *e) {
    return (getQEF(e->he->v).Sv + getQEF(e->he->flip->v).Sv)/(getQEF(e->he->v).n + getQEF(e->he->flip->v).n);
}

float Collapsible::getCombinedError(const Edge* e) {
    return (getQEF(e->he->v) + getQEF(e->he->flip->v)).eval(getNewPoint(e));
}


/////////////////
// Collapsible //
/////////////////
// PRIVATE FUNCTIONS

// very time expensive
// this is the only code that prevents working with non-triangular meshes
bool Collapsible::checkSafety(const Edge *e) const {
    auto v1nbrs(e->he->v->neighbors());
    auto v2nbrs(e->he->flip->v->neighbors());
    sort(v1nbrs.begin(), v1nbrs.end());
    sort(v2nbrs.begin(), v2nbrs.end());
    vector<Vertex*> intersect(v1nbrs.size() + v2nbrs.size());
    auto it = set_intersection(v1nbrs.begin(), v1nbrs.end(), v2nbrs.begin(), v2nbrs.end(), intersect.begin());

    if (it - intersect.begin() == 2)
        return true;

    return false;
}

Vertex* Collapsible::collapse(Edge *e) {
    auto qef(getQEF(e->he->v) + getQEF(e->he->flip->v));

    Vertex *combined = e->he->v;
    combined->pos = getNewPoint(e);

    m_countDeletedFaces += e->he->collapse();

    getQEF(combined) = qef;

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

            if (top->he == nullptr) { // invalid?
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

                v->he->traverseVertex([&](Halfedge* he) {
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
