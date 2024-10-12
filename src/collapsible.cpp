#include "collapsible.h"

#include <iostream> // cout, endl
#include <queue>    // priority_queue, greater
#include <set>      // set

using namespace std;


/////////////////
// DistanceQEF //
/////////////////
DistanceQEF::DistanceQEF(float n, const f32v3& Sv, float Svtv) : n(n), Sv(Sv), Svtv(Svtv) {
}

DistanceQEF::DistanceQEF(Halfedge* he) : DistanceQEF() {
    he->v->traverseEdges([&](Halfedge* it) {
        ++n;
        Sv += it->flip->v->pos;
        Svtv += it->flip->v->pos.dot(it->flip->v->pos);
    });
}

float DistanceQEF::evaluateErrorImpl(const f32v3& p) const {
    return n * p.dot(p) - 2 * p.dot(Sv) + Svtv;
}

f32v3 DistanceQEF::minimizeErrorImpl() const {
    return Sv/n;
}

DistanceQEF DistanceQEF::operator+(const DistanceQEF& qef) const {
    return { n + qef.n, Sv + qef.Sv, Svtv + qef.Svtv };
}


//////////////
// PlaneQEF //
//////////////
PlaneQEF::PlaneQEF(const f32v3& Snnt012, const f32v3& Snnt458, const f32v3& Snd, float Sd)
  : Snnt012(Snnt012)
  , Snnt458(Snnt458)
  , Snd(Snd)
  , Sd2(Sd) {
}

PlaneQEF::PlaneQEF(Vertex* v) : PlaneQEF() {
    v->traverseEdges([&](Halfedge* it) {
        const f32v3 n_i = it->f->normal();
        const float d_i = it->v->pos.dot(n_i);
        Snnt012 += n_i * n_i.x;
        Snnt458 += { n_i.y * n_i.y, n_i.y * n_i.z, n_i.z * n_i.z };
        Snd += n_i * d_i;
        Sd2 += d_i * d_i;
    });
}

float PlaneQEF::evaluateErrorImpl(const f32v3& p) const {
    return p.dot({ p.dot(Snnt012), p.dot(Snnt345()), p.dot(Snnt678())}) - 2.0f * p.dot(Snd) + Sd2;
}

f32v3 PlaneQEF::minimizeErrorImpl() const {
    // NEED PSEUDOINVERSE
    return {};
}

PlaneQEF PlaneQEF::operator+(const PlaneQEF& qef) const {
    return { Snnt012 + qef.Snnt012, Snnt458 + qef.Snnt458, Snd + qef.Snd, Sd2 + qef.Sd2 };
}


/////////////
// QEFEdge //
/////////////
void QEFEdge::updateQEF() {
    qef = static_cast<QEFVertex*>(he->v)->qef + static_cast<QEFVertex*>(he->flip->v)->qef;
    newPos = qef.minimizeError();
    error = qef.evaluateError(newPos);
    dirty = false;
}

bool QEFEdge::checkSafety() const {
    // Compute neighborhood of vertices touching one side of prospective edge, less the vertices on its faces
    set<Vertex*> neighbors;
    bool hasOtherNeighbors = false;

    for (Halfedge *it = he->flip->next->flip->next; it != he->prev->flip; it = it->flip->next) {
        neighbors.insert(it->flip->v);
        hasOtherNeighbors = true;
    }

    // Check the neighborhood on the other side for any matches
    for (Halfedge *it = he->next->flip->next; it != he->flip->prev->flip; it = it->flip->next) {
        if (neighbors.contains(it->flip->v))
            return false;
        hasOtherNeighbors = true;
    }

    // Allow collpase if there are neighbors, or if either side of this edge is not a triangle
    return hasOtherNeighbors || (!he->f->isTriangle() || !he->flip->f->isTriangle());
}

size_t QEFEdge::collapse() {
    // Get the new point and calculate its position before altering the topology
    QEFVertex *remaining = static_cast<QEFVertex*>(he->v);
    remaining->qef = qef;
    remaining->pos = newPos;

    // Collapse the triangle and report how many faces we removed
    return he->collapse();
}


/////////////////
// Collapsible //
/////////////////
Collapsible::Collapsible(const char* objfile)
  : Manifold(objfile)
  , m_removedCount(0ul) {
    for (auto &vertex : m_vertices)
        vertex.qef = { vertex.he };

    for (auto &edge : m_edges)
        edge.updateQEF();
}

void Collapsible::simplify(uint64_t finalCount) {
    struct EdgeRef {
        QEFEdge *e;

        auto operator<=>(const EdgeRef& o) const { return e->error <=> o.e->error; }
    };

    // Populate the priority queue
    priority_queue<EdgeRef, vector<EdgeRef>, greater<EdgeRef>> errors;
    for (QEFEdge &e : m_edges)
        errors.push({ &e });

    // The heart of the algorithm
    const uint64_t delta = m_faces.size() - finalCount;
    while (m_removedCount < delta && !errors.empty()) {
        QEFEdge *top = errors.top().e;
        errors.pop();

        if (top->invalid()) {
            // This edge has been deleted during a collapse, remove it from queue
        } else if (top->dirty) {
            // Error has been increased, recalculate it
            top->updateQEF();
            errors.push({ top });
        } else if (!top->checkSafety()) {
            // Unsafe edge, remove it, but we'll add it back if a neighbor collapses
            top->unsafe = true;
        } else { // Collapse it!
            auto remainingVertex = top->he->v;
            m_removedCount += top->collapse();

            remainingVertex->traverseEdges([&](Halfedge* he) {
                auto edge = static_cast<QEFEdge*>(he->e);
                edge->dirty = true;
                if (edge->unsafe) {
                    edge->unsafe = false;
                    errors.push({ edge });
                }
            });
        }
    }

#ifndef NDEBUG
    verifyConnections();
#endif

    m_vertices.remove_if([](auto& v){ return v.invalid(); });
    m_faces.remove_if([](auto& f){ return f.invalid(); });
    m_edges.remove_if([](auto& e){ return e.invalid(); });
    m_halfedges.remove_if([](auto& he){ return he.invalid(); });
}
