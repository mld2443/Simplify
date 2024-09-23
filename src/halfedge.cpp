//
//  halfedge.cpp
//  Simplify
//
//  Created by Matthew Dillard on 11/28/15.
//

#include "halfedge.h"

qef::qef(halfedge* he){
    n = 0;
    vtv = 0;
    Sv = {0,0,0};

    halfedge *trav = he;
    do {
        ++n;
        Sv += trav->flip->o->pos;
        vtv += trav->flip->o->pos.dot(trav->flip->o->pos);
        trav = trav->flip->next;
    } while(trav != he);
}

unsigned int halfedge::collapse() {
    unsigned int deleted_faces = 0;

    //update origin points
    o->he = prev->flip;
    flip->o->update(o);

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
        prev->o->he = next->flip;
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
        flip->prev->o->he = flip->next->flip;
    }
    else {
        //update flip's edges to skip this obselete halfedge
        flip->prev->next = flip->next;
        flip->next->prev = flip->prev;
    }

    return deleted_faces;
}

void vertex::markEdges() {
    halfedge *trav = he;
    do {
        trav->e->dirty = true;
        trav = trav->flip->next;
    } while(trav != he);
}
