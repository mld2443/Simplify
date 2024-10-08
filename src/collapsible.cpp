#include "collapsible.h"

#include <iostream> // cout, endl
#include <queue>    // priority_queue, greater
#include <set>      // set

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
    // Compute neighborhood of vertices touching one side of prospective edge, less the vertices on its faces
    set<Vertex*> neighbors;
    bool hasOtherNeighbors = false;

    for (Halfedge *it = e->he->flip->next->flip->next; it != e->he->prev->flip; it = it->flip->next) {
        neighbors.insert(it->flip->v);
        hasOtherNeighbors = true;
    }

    // Check the neighborhood on the other side for any matches
    for (Halfedge *it = e->he->next->flip->next; it != e->he->flip->prev->flip; it = it->flip->next) {
        if (neighbors.contains(it->flip->v))
            return false;
        hasOtherNeighbors = true;
    }

    // Allow collpase if there are neighbors, or if either side of this edge is not a triangle
    return hasOtherNeighbors || (!e->he->f->isTriangle() || !e->he->flip->f->isTriangle());
}

Vertex* Collapsible::collapse(Edge *e) {
    auto combinedError(getQEF(e->he->v) + getQEF(e->he->flip->v));

    // Get the new point and calculate its position before altering the topology
    Vertex *combined = e->he->v;
    combined->pos = getNewPoint(e);

    // Collapse the triangle and record how many faces we removed
    m_removedCount += e->he->collapse();

    getQEF(combined) = combinedError;
    return combined;
}


// PUBLIC FUNCTIONS

Collapsible::Collapsible(const char* objfile)
  : Manifold(objfile)
  , m_removedCount(0ul) {
    for (auto &v : m_vertices)
        v.calcQEF();
}

void Collapsible::simplify(uint64_t finalCount) {
    struct EdgeWithError {
        Edge *e;
        float error;

        EdgeWithError(Edge* e): e(e), error(getCombinedError(e)) {}
        EdgeWithError(const EdgeWithError& e) = default;

        auto operator<=>(const EdgeWithError& e) const { return error <=> e.error; }
    };

    // Populate the priority queue
    priority_queue<EdgeWithError, vector<EdgeWithError>, greater<EdgeWithError>> errors;
    for (auto &e : m_edges)
        errors.emplace(&e);

    // The heart of the algorithm
    const uint64_t delta = m_faces.size() - finalCount;
    while (m_removedCount < delta && !errors.empty()) {
        Edge *top = errors.top().e;
        errors.pop();

        if (top->invalid()) {
            // This edge has been deleted during a collapse, remove it from queue
        } else if (top->dirty) {
            // Error has been increased, recalculate it
            top->dirty = false;
            errors.emplace(top);
        } else if (!checkSafety(top)) {
            // Unsafe edge, remove it, but we'll add it back if a neighbor collapses
            top->unsafe = true;
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

#ifndef NDEBUG
    verifyConnections();
#endif

    cout << "Sizes: " << m_vertices.size() << " vertices, " << m_faces.size() << " faces, " << m_edges.size() << " edges, " << m_halfedges.size() << " halfedges" << endl;

    const size_t rV = m_vertices.remove_if([](auto& v){ return v.invalid(); });
    const size_t rF = m_faces.remove_if([](auto& f){ return f.invalid(); });
    const size_t rE = m_edges.remove_if([](auto& e){ return e.invalid(); });
    const size_t rH = m_halfedges.remove_if([](auto& he){ return he.invalid(); });

    cout << "Removed: " << rV << " vertices, " << rF << " faces, " << rE << " edges, " << rH << " halfedges" << endl;
    cout << "Remaining: " << m_vertices.size() << " vertices, " << m_faces.size() << " faces, " << m_edges.size() << " edges, " << m_halfedges.size() << " halfedges" << endl;
}
