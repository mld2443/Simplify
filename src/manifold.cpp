#include "manifold.h"
#include "element.h"

#include <iostream>
#include <algorithm>
#include <utility>
#include <queue>


using namespace std;

edge* manifold::get_edge(vertex *v1, vertex* v2) {
    auto key = std::pair<vertex*, vertex*>(max(v1, v2), min(v1, v2));

    if (auto result = edge_hash.find(key); result != edge_hash.end())
        return result->second;

    edges.push_back({nullptr, false, false, true});

    return edge_hash[key] = &edges.back();
}

// very time expensive
// this is the only code that prevents working with non-triangular meshes
bool manifold::checkSafety(const edge *e) const {
    auto v1nbrs(e->he->o->neighbors());
    auto v2nbrs(e->he->flip->o->neighbors());
    std::sort(v1nbrs.begin(), v1nbrs.end());
    std::sort(v2nbrs.begin(), v2nbrs.end());
    std::vector<vertex*> intersect(v1nbrs.size() + v2nbrs.size());
    auto it = std::set_intersection(v1nbrs.begin(), v1nbrs.end(), v2nbrs.begin(), v2nbrs.end(), intersect.begin());

    if(it-intersect.begin() == 2)
        return true;

    return false;
}

void manifold::collapse(edge *e) {
    auto qef(e->he->o->qef + e->he->flip->o->qef);
    e->he->o->pos = e->getNewPt();
    deleted_faces += (unsigned long)e->collapse();

    e->he->o->qef = qef;
    e->he->o->markEdges();
}

int manifold::verify() {
    vertices.remove_if(invalid());
    faces.remove_if(invalid());
    edges.remove_if(invalid());
    halfedges.remove_if(invalid());

    int rvalue = 0;

    for (auto &v : vertices) {
        if (v.valid) {
            if (!v.he->valid)
                rvalue |= 1<<0;
        }
    }
    for (auto &f : faces) {
        if (f.valid) {
            if (!f.he->valid)
                rvalue |= 1<<1;
            if (f.he->next->next->next != f.he)
                rvalue |= 1<<2;
            if (f.he->prev->prev->prev != f.he)
                rvalue |= 1<<3;
        }
    }
    for (auto &h : halfedges) {
        if (h.valid) {
            if (!h.o->valid)
                rvalue |= 1<<4;
            if (!h.e->valid)
                rvalue |= 1<<5;
            if (!h.f->valid)
                rvalue |= 1<<6;
            if (&h != h.flip->flip)
                rvalue |= 1<<7;
        }
    }
    for (auto &e : edges) {
        if (e.valid) {
            if (!e.he->valid)
                rvalue |= 1<<8;
        }
    }

    return rvalue;
}

vertex* manifold::add_vert(const float x, const float y, const float z) {
    vertices.push_back({{x,y,z},{},nullptr, true});
    return &vertices.back();
}

void manifold::add_face(const std::list<vertex*>& verts) {
    faces.push_back({nullptr, true});

    halfedge *first = nullptr, *prev = nullptr;

    for (auto it = verts.begin(); it != verts.end(); ++it) {
        auto next = it;
        if (++next == verts.end())
            next = verts.begin();

        edge *e = get_edge(*it, *next);

        halfedges.push_back({nullptr,prev,e->he,&faces.back(),*it,e, true});
        if (it != verts.begin())
            prev->next = &halfedges.back();

        if (e->he)
            e->he->flip = &halfedges.back();

        e->he = prev = &halfedges.back();
        if (it == verts.begin())
            first = prev;

        prev->o->he = prev;
    }

    prev->next = first;
    first->prev = faces.back().he = &halfedges.back();
}

void manifold::clear() {
    vertices.clear();
    faces.clear();
    edges.clear();
    halfedges.clear();

    edge_hash.clear();

    deleted_faces = 0;
}

void manifold::cleanup() {
    edge_hash.clear();

    deleted_faces = 0;

    for (auto &v : vertices)
        v.calcQEF();

    auto result = verify();
    if (result)
        std::cout << "ERROR, CODE " << std::oct << result << std::endl;
}

void manifold::simplify(const unsigned long count) {
    //construct PQ
    std::priority_queue<element, std::vector<element>, elementComp> errors;
    for (auto &e : edges) {
        errors.push(&e);
    }

    //do the algorithm
    while (faces.size() - deleted_faces > count) {
        if (!errors.empty()) {
            auto  elem = errors.top();
            if (!elem.dirty()) {
                if (elem.valid()) {
                    if (checkSafety(elem.e)) {
                        collapse(elem.e);

                        auto v = elem.e->he->o;
                        halfedge *trav = v->he;
                        do {
                            if (trav->e->unsafe) {
                                trav->e->unsafe = false;
                                errors.push(trav->e);
                            }
                            trav = trav->flip->next;
                        } while(trav != v->he);
                    }
                    else { //unsafe edge
                        errors.top().e->unsafe = true;
                        errors.pop();
                    }

                }
                else { //invalid
                    errors.pop();
                }
            }
            else { //dirty
                auto e = errors.top().e;
                errors.pop();
                e->dirty = false;
                errors.push(e);
            }
        }
        else { //no more safe edges!
            deleted_faces = (unsigned int)faces.size();
        }
    }

    auto result = verify();
    if (result)
        std::cout << "ERROR, CODE " << std::oct << result << std::endl;
}

void manifold::draw(const bool drawcontrol) const {
    for (auto &f : faces)
        f.draw();
}

