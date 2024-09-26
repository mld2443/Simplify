#include "manifold.h"
#include "element.h"

#include <iostream>  //
#include <algorithm> //
#include <utility>   //
#include <queue>     // priority_queue
#include <fstream>   // fstream
#include <string>    // string, getline
#include <cctype>    // isdigit
#include <limits>    // numeric_limits::min, max

using namespace std;


///////////////////////
// Bounds, Dimension //
///////////////////////
template <typename T>
Manifold::AABB<T>::Dimension::Dimension()
  : lo(numeric_limits<T>::max())
  , hi(numeric_limits<T>::min()) {
}

template <typename T>
void Manifold::AABB<T>::Dimension::addSample(T s) {
    if (s < lo) lo = s;
    if (s > hi) hi = s;
}

template <typename T>
T Manifold::AABB<T>::Dimension::delta() const {
    return hi - lo;
}

template <typename T>
T Manifold::AABB<T>::Dimension::centroid() const {
    return hi + lo / static_cast<float>(2.0);
}

template <typename T>
void Manifold::AABB<T>::addPoint(const v3<T>& v) {
    x.addSample(v.x);
    y.addSample(v.y);
    z.addSample(v.z);
}


//////////////
// Manifold //
//////////////
// PRIVATE FUNCTIONS
Edge* Manifold::getEdge(Vertex *v1, Vertex* v2) {
    auto key = std::pair<Vertex*, Vertex*>(max(v1, v2), min(v1, v2));

    if (auto result = edge_hash.find(key); result != edge_hash.end())
        return result->second;

    edges.push_back({ nullptr, false, false, true });

    return edge_hash[key] = &edges.back();
}

// very time expensive
// this is the only code that prevents working with non-triangular meshes
bool Manifold::checkSafety(const Edge *e) const {
    auto v1nbrs(e->he->o->neighbors());
    auto v2nbrs(e->he->flip->o->neighbors());
    sort(v1nbrs.begin(), v1nbrs.end());
    sort(v2nbrs.begin(), v2nbrs.end());
    vector<Vertex*> intersect(v1nbrs.size() + v2nbrs.size());
    auto it = set_intersection(v1nbrs.begin(), v1nbrs.end(), v2nbrs.begin(), v2nbrs.end(), intersect.begin());

    if (it-intersect.begin() == 2)
        return true;

    return false;
}

void Manifold::collapse(Edge *e) {
    auto qef(e->he->o->qef + e->he->flip->o->qef);
    e->he->o->pos = e->getNewPt();
    deleted_faces += (unsigned long)e->collapse();

    e->he->o->qef = qef;
    e->he->o->markEdges();
}

int Manifold::verify() {
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

Vertex* Manifold::addPoint(v3f& p) {
    vertices.push_back({ p, {}, nullptr, true });
    return &vertices.back();
}

void Manifold::addFace(const std::list<Vertex*>& verts) {
    faces.push_back({ nullptr, true });

    Halfedge *first = nullptr, *prev = nullptr;

    for (auto it = verts.begin(); it != verts.end(); ++it) {
        auto next = it;
        if (++next == verts.end())
            next = verts.begin();

        Edge *e = getEdge(*it, *next);

        halfedges.push_back({ nullptr, prev, e->he, &faces.back(), *it, e, true });
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


// PUBLIC FUNCTIONS

Manifold::Manifold(const char* objfile, bool invert) {
    vector<Vertex*> v_pointers;
    ifstream file(objfile);
    string token;

    while (!file.eof()) {
        file >> token;
        // Ignore comments
        if (token[0] == '#') {
            string dummy;
            getline(file, dummy);
        }
        // Process vertices, discard normals
        else if (token[0] == 'v' && token[1] != 'n') {
            v3f p;
            file >> p;

            bounds.addPoint(p);

            v_pointers.push_back(addPoint(p));
        }
        // Process faces
        else if (token[0] == 'f') {
            list<Vertex*> face_verts;
            file >> ws;
            while (isdigit(file.peek())) {
                string vnum;
                file >> vnum >> ws;

                // Again, discard indices to normals
                if (size_t found = vnum.find("//"); found != string::npos)
                    vnum = vnum.substr(0, found);

                // Some models may be inside out
                if (invert)
                    face_verts.push_front(v_pointers[stoul(vnum) - 1]);
                else
                    face_verts.push_back(v_pointers[stoul(vnum) - 1]);
            }

            addFace(face_verts);
        }
    }

    edge_hash.clear();

    deleted_faces = 0;

    for (auto &v : vertices)
        v.calcQEF();

    auto result = verify();
    if (result)
        std::cout << "ERROR, CODE " << std::oct << result << std::endl;
}

v3f Manifold::getAABBSizes() const {
    return { bounds.x.delta(), bounds.y.delta(), bounds.z.delta() };
}

v3f Manifold::getAABBCentroid() const {
    return { bounds.x.centroid(), bounds.y.centroid(), bounds.z.centroid() };
}

void Manifold::simplify(const unsigned long count) {
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
                        Halfedge *trav = v->he;
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

void Manifold::draw() const {
    for (auto &f : faces)
        f.draw();
}
