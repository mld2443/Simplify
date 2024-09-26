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
QuadraticErrorFunction::QuadraticErrorFunction(float n, const v3f& Sv, float vtv)
  : n(n)
  , Sv(Sv)
  , vtv(vtv) {
}

QuadraticErrorFunction::QuadraticErrorFunction(const Halfedge* he) : QuadraticErrorFunction() {
    const Halfedge *trav = he;
    do {
        ++n;
        Sv += trav->flip->v->pos;
        vtv += trav->flip->v->pos.dot(trav->flip->v->pos);
        trav = trav->flip->next;
    } while (trav != he);
}

float QuadraticErrorFunction::eval(const v3f& v) const {
    return n * v.dot(v) - 2 * v.dot(Sv) + vtv;
}

QuadraticErrorFunction QuadraticErrorFunction::operator+(const QuadraticErrorFunction& qef) const {
    return { n + qef.n, Sv + qef.Sv, vtv + qef.vtv };
}

QuadraticErrorFunction& QuadraticErrorFunction::operator=(const QuadraticErrorFunction& qef) {
    n = qef.n;
    Sv = qef.Sv;
    vtv = qef.vtv;
    return *this;
}

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

void Collapsible::collapse(Edge *e) {
    auto qef(getQEF(e->he->v) + getQEF(e->he->flip->v));
    e->he->v->pos = getNewPoint(e);
    m_countDeletedFaces += (unsigned long)e->collapse();

    getQEF(e->he->v) = qef;
    e->he->v->markEdges();
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

        bool dirty() const { return e->dirty; }
        bool valid() const { return e->valid; }

        bool operator<(const EdgeWithError& e) const { return error < e.error; }
    };

    // Populate the priority queue
    std::priority_queue<EdgeWithError, std::vector<EdgeWithError>> errors;
    for (auto &e : m_edges) {
        errors.push({ &e });
    }

    // The heart of the algorithm
    while (m_faces.size() - m_countDeletedFaces > count) {
        if (errors.empty()) { // No more safe edges! Panic!
            m_countDeletedFaces = (unsigned int)m_faces.size();
        } else {
            auto elem = errors.top();
            if (elem.dirty()) {
                auto e = errors.top().e;
                errors.pop();
                e->dirty = false;
                errors.push({ e });
            } else {
                if (!elem.valid()) { // invalid?
                    errors.pop();
                }
                else {
                    if (checkSafety(elem.e)) { //unsafe edge
                        errors.top().e->unsafe = true;
                        errors.pop();
                    } else { // Collapse it!
                        collapse(elem.e);

                        auto v = elem.e->he->v;
                        Halfedge *trav = v->he;
                        do {
                            if (trav->e->unsafe) {
                                trav->e->unsafe = false;
                                errors.push({ trav->e });
                            }
                            trav = trav->flip->next;
                        } while(trav != v->he);
                    }
                }
            }
        }
    }

    verify();
}
