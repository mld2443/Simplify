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
