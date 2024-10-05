#include "halfedge.h"

#include <GL/gl.h> // glVertex3fv, glNormal3fv


static bool surgicalRemoval(Halfedge* he) {
    if (he->f->isTriangle()) { // Triangles get removed
        // Update outer flips
        he->prev->flip->flip = he->next->flip;
        he->next->flip->flip = he->prev->flip;

        // Update outer edges
        he->prev->e->he = he->prev->flip;
        he->next->flip->e = he->prev->e;

        // Ensure the outside point points to removed halfedge
        he->prev->v->he = he->next->flip;

        // Mark all these for removal
        he->next->e->he = nullptr;
        he->prev->f = nullptr;
        he->next->f = nullptr;
        he->f->he = nullptr;
        he->f = nullptr;

        return true;
    } else { // More than 3 sides, just a little housekeeping
        // Make sure the face is not pointing at this particular halfedge
        he->f->he = he->next;

        // Update edges to skip this obselete halfedge
        he->prev->next = he->next;
        he->next->prev = he->prev;

        // Still marks the halfedge to be removed
        he->f = nullptr;

        return false;
    }
}

// Pretty much irreversible, better mean it!
uint64_t Halfedge::collapse() {
    uint64_t deletedFaces = 0ul;

    // Make this vertex point to a different halfedge with the same root
    v->he = prev->flip;

    {
        // While the halfedges are still connected, update the roots of the vertex to be removed
        Vertex *condemned = flip->v;
        flip->v->traverseEdges([&](Halfedge* he){ he->v = this->v; });

        // Mark this vertex for removal, have to save it off because the above changes flip->v
        condemned->he = nullptr;
    }

    deletedFaces += surgicalRemoval(flip);
    deletedFaces += surgicalRemoval(this);

    // Mark the edge for removal
    e->he = nullptr;

    return deletedFaces;
}


bool Vertex::isValence3() const {
    return he->flip->next->flip->next->flip->next == he;
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

bool Face::isTriangle() const {
    return he->next->next->next == he;
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
