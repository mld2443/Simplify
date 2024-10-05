#include "halfedge.h"

#include <GL/gl.h> // glVertex3fv, glNormal3fv

using namespace std;


uint64_t Halfedge::collapse() {
    uint64_t deleted_faces = 0ul;

    // Make this vertex point to a different halfedge with the same root
    v->he = prev->flip;

    {
        // While the halfedges are still connected, update the roots of the vertex to be removed
        Vertex *condemned = flip->v;
        flip->v->traverseEdges([&](Halfedge* he){ he->v = this->v; });

        // Mark this vertex for removal, have to save it off because the above changes flip->v
        condemned->he = nullptr;
    }

    // Check if this face is a triangle
    if (next->next == prev) {
        ++deleted_faces;

        // Update outer flips
        prev->flip->flip = next->flip;
        next->flip->flip = prev->flip;

        // Update outer edges
        prev->e->he = prev->flip;
        next->flip->e = prev->e;

        // Ensure the outside point points to removed halfedge
        prev->v->he = next->flip;

        // Mark all these for removal
        next->e->he = nullptr;
        prev->f = nullptr;
        next->f = nullptr;
        f->he = nullptr;
    } else {
        // Update edges to skip this obselete halfedge
        prev->next = next;
        next->prev = prev;
    }

    // Remove this halfedge
    f = nullptr;

    // Check if the flip's face is a triangle
    if (flip->next->next == flip->prev) {
        ++deleted_faces;

        // Update outer flip's flips
        flip->prev->flip->flip = flip->next->flip;
        flip->next->flip->flip = flip->prev->flip;

        // Update flip's outer edges
        flip->next->e->he = flip->next->flip;
        flip->prev->flip->e = flip->next->e;

        // Ensure the flip's outside point points to removed halfedge
        flip->prev->v->he = flip->next->flip;

        // Mark all removed flip objects
        flip->prev->e->he = nullptr;
        flip->prev->f = nullptr;
        flip->next->f = nullptr;
        flip->f->he = nullptr;
    } else {
        // Update flip's edges to skip this obselete halfedge
        flip->prev->next = flip->next;
        flip->next->prev = flip->prev;
    }

    // Mark the opposite halfedge for removal
    flip->f = nullptr;

    // Mark the edge for removal
    e->he = nullptr;

    return deleted_faces;
}


uint64_t Vertex::degree() const {
    uint64_t degree = 0ul;

    traverseEdges([&](Halfedge*) { ++degree; });

    return degree;
}

void Vertex::traverseEdges(std::function<void(Halfedge*)> op) const {
    Halfedge *it = he;
    do op(it);
    while ((it = it->flip->next) != he);
}

void Vertex::draw() const {
    glVertex3fv(&pos.x);
}


f32v3 Edge::midpoint() const {
    return (he->v->pos + he->flip->v->pos) / 2.0f;
}

void Edge::draw() const {
    he->v->draw();
    he->flip->v->draw();
}


f32v3 Face::normal() const {
    return (he->next->next->v->pos - he->v->pos).cross(he->next->next->next->v->pos - he->next->v->pos).normalize();
}

f32v3 Face::centroid() const {
    uint64_t degree = 0ul;
    f32v3 sum;

    traversePerimeter([&](Halfedge* he){ sum += he->v->pos; ++degree; });

    return sum / degree;
}

uint64_t Face::degree() const {
    uint64_t degree = 0ul;

    traversePerimeter([&](Halfedge*) { ++degree; });

    return degree;
}

void Face::traversePerimeter(std::function<void(Halfedge*)> op) const {
    Halfedge *it = he;
    do op(it);
    while ((it = it->next) != he);
}

void Face::draw() const {
    const f32v3 n = normal();
    glNormal3fv(&n.x);

    traversePerimeter([](Halfedge* he){ he->v->draw(); });
}
