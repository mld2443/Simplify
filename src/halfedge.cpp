#include "halfedge.h"


size_t Halfedge::collapse() {
    size_t deleted_faces = 0ul;

    //update origin points
    v->he = prev->flip;
    flip->v->update(v);

    //remove this halfedge
    valid = false;

    //check if this face is a triangle
    if (next->next == prev) {
        ++deleted_faces;

        //mark all obselete objects
        next->e->valid = false;
        prev->valid = false;
        next->valid = false;
        f->valid = false;

        //update outer flips
        prev->flip->flip = next->flip;
        next->flip->flip = prev->flip;

        //update outer edges
        prev->e->he = prev->flip;
        next->flip->e = prev->e;

        //ensure the outside point points to valid halfedge
        prev->v->he = next->flip;
    }
    else {
        //update edges to skip this obselete halfedge
        prev->next = next;
        next->prev = prev;
    }

    //remove opposite halfedge
    flip->valid = false;

    //check if the flip's face is a triangle
    if (flip->next->next == flip->prev) {
        ++deleted_faces;

        //mark all obselete flip objects
        flip->prev->e->valid = false;
        flip->prev->valid = false;
        flip->next->valid = false;
        flip->f->valid = false;

        //update outer flip's flips
        flip->prev->flip->flip = flip->next->flip;
        flip->next->flip->flip = flip->prev->flip;

        //update flip's outer edges
        flip->next->e->he = flip->next->flip;
        flip->prev->flip->e = flip->next->e;

        //ensure the flip's outside point points to valid halfedge
        flip->prev->v->he = flip->next->flip;
    }
    else {
        //update flip's edges to skip this obselete halfedge
        flip->prev->next = flip->next;
        flip->next->prev = flip->prev;
    }

    return deleted_faces;
}

void Halfedge::traverseVertex(std::function<void(Halfedge*)> op) {
    Halfedge *it = this;
    do op(it);
    while ((it = it->flip->next) != this);
}

void Halfedge::traverseFace(std::function<void(Halfedge*)> op) {
    Halfedge *it = this;
    do op(it);
    while ((it = it->next) != this);
}


//void Vertex::update()

void Vertex::markEdges() {
    he->traverseVertex([] (Halfedge* he) { he->e->dirty = true; });
}

void Vertex::draw() const {
    glVertex3fv(&pos.x);
}


v3f Edge::midpoint() const {
    return (he->v->pos + he->flip->v->pos) / 2.0f;
}

void Edge::draw() const {
    he->v->draw();
    he->flip->v->draw();
}


v3f Face::normal() const {
    return (he->next->next->v->pos - he->v->pos).cross(he->next->next->next->v->pos - he->next->v->pos).normalize();
}

v3f Face::centroid() const {
    size_t count = 0ul;
    v3f sum;

    he->traverseFace([&](Halfedge* he){ sum += he->v->pos; ++count; });

    return sum / count;
}

void Face::draw() const {
    v3f n = normal();
    glNormal3fv(&n.x);

    he->traverseFace([](Halfedge* he){ he->v->draw(); });
}
