#include "collapsible.h"
#include "element.h"

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
Collapsible::QuadraticErrorFunction::QuadraticErrorFunction(float n, const v3f& Sv, float vtv)
  : n(n)
  , Sv(Sv)
  , vtv(vtv) {
}

Collapsible::QuadraticErrorFunction::QuadraticErrorFunction(const Halfedge* he) : QuadraticErrorFunction() {
    const Halfedge *trav = he;
    do {
        ++n;
        Sv += trav->flip->v->pos;
        vtv += trav->flip->v->pos.dot(trav->flip->v->pos);
        trav = trav->flip->next;
    } while (trav != he);
}

float Collapsible::QuadraticErrorFunction::eval(const v3f& v) const {
    return n * v.dot(v) - 2 * v.dot(Sv) + vtv;
}

Collapsible::QuadraticErrorFunction Collapsible::QuadraticErrorFunction::operator+(const QuadraticErrorFunction& qef) const {
    return { n + qef.n, Sv + qef.Sv, vtv + qef.vtv };
}

Collapsible::QuadraticErrorFunction& Collapsible::QuadraticErrorFunction::operator=(const QuadraticErrorFunction& qef) {
    n = qef.n;
    Sv = qef.Sv;
    vtv = qef.vtv;
    return *this;
}

void Collapsible::QEFVertex::calcQEF() {
    qef = { he };
}

Collapsible::QuadraticErrorFunction& Collapsible::getQEF(Vertex *v) {
    return static_cast<QEFVertex*>(v)->qef;
}

v3f Collapsible::getNewPoint(const Edge *e) {
    return (getQEF(e->he->v).Sv + getQEF(e->he->flip->v).Sv)/(getQEF(e->he->v).n + getQEF(e->he->flip->v).n);
}

float Collapsible::getCombinedError(const Edge* e) {
    return (getQEF(e->he->v) + getQEF(e->he->flip->v)).eval(getNewPoint(e));
}


///////////////////////
// Bounds, Dimension //
///////////////////////
Collapsible::AABB::Dimension::Dimension()
  : lo(numeric_limits<float>::max())
  , hi(numeric_limits<float>::min()) {
}

void Collapsible::AABB::Dimension::addSample(float s) {
    if (s < lo) lo = s;
    if (s > hi) hi = s;
}

float Collapsible::AABB::Dimension::delta() const {
    return hi - lo;
}

float Collapsible::AABB::Dimension::centroid() const {
    return (hi + lo) / 2.0f;
}

void Collapsible::AABB::addPoint(const v3f& v) {
    x.addSample(v.x);
    y.addSample(v.y);
    z.addSample(v.z);
}


/////////////////
// Collapsible //
/////////////////
// PRIVATE FUNCTIONS
Edge* Collapsible::getEdge(const Vertex *v1, const Vertex* v2) {
    const auto key = std::pair<const Vertex*, const Vertex*>(max(v1, v2), min(v1, v2));

    if (auto result = edgeHash.find(key); result != edgeHash.end())
        return result->second;

    edges.push_back({ nullptr, false, false, true });

    return edgeHash[key] = &edges.back();
}

// very time expensive
// this is the only code that prevents working with non-triangular meshes
bool Collapsible::checkSafety(const Edge *e) const {
    auto v1nbrs(e->he->v->neighbors());
    auto v2nbrs(e->he->flip->v->neighbors());
    sort(v1nbrs.begin(), v1nbrs.end());
    sort(v2nbrs.begin(), v2nbrs.end());
    vector<Vertex*> intersect(v1nbrs.size() + v2nbrs.size());
    auto it = set_intersection(v1nbrs.begin(), v1nbrs.end(), v2nbrs.begin(), v2nbrs.end(), intersect.begin());

    if (it-intersect.begin() == 2)
        return true;

    return false;
}

void Collapsible::collapse(Edge *e) {
    auto qef(getQEF(e->he->v) + getQEF(e->he->flip->v));
    e->he->v->pos = getNewPoint(e);
    countDeletedFaces += (unsigned long)e->collapse();

    getQEF(e->he->v) = qef;
    e->he->v->markEdges();
}

int Collapsible::verify() {
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
            if (!h.v->valid)
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

Vertex* Collapsible::addPoint(v3f& p) {
    vertices.push_back({ p, nullptr, true, {} });
    return &vertices.back();
}

void Collapsible::addFace(const list<Vertex*>& verts) {
    faces.push_back({ nullptr, true });

    Halfedge *first = nullptr, *prev = nullptr;

    for (auto it = verts.begin(); it != verts.end(); ++it) {
        auto next = it;
        if (++next == verts.end())
            next = verts.begin();

        Edge *e = getEdge(*it, *next);

        halfedges.push_back({ nullptr, prev, e->he, *it, e, &faces.back(), true });
        if (it != verts.begin())
            prev->next = &halfedges.back();

        if (e->he)
            e->he->flip = &halfedges.back();

        e->he = prev = &halfedges.back();
        if (it == verts.begin())
            first = prev;

        prev->v->he = prev;
    }

    prev->next = first;
    first->prev = faces.back().he = &halfedges.back();
}


// PUBLIC FUNCTIONS

Collapsible::Collapsible(const char* objfile, bool invert) {
    vector<Vertex*> v_pointers;
    ifstream file(objfile);
    string token;

    while (!file.eof()) {
        file >> token;
        // Discard comments
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

    edgeHash.clear();

    countDeletedFaces = 0ul;

    for (auto &v : vertices)
        v.calcQEF();

    auto result = verify();
    if (result)
        std::cout << "ERROR, CODE " << std::oct << result << std::endl;
}

v3f Collapsible::getAABBSizes() const {
    return { bounds.x.delta(), bounds.y.delta(), bounds.z.delta() };
}

v3f Collapsible::getAABBCentroid() const {
    return { bounds.x.centroid(), bounds.y.centroid(), bounds.z.centroid() };
}

void Collapsible::simplify(const unsigned long count) {
    //construct PQ
    std::priority_queue<element, std::vector<element>, elementComp> errors;
    for (auto &e : edges) {
        errors.push({ &e, getCombinedError(&e) });
    }

    //do the algorithm
    while (faces.size() - countDeletedFaces > count) {
        if (!errors.empty()) {
            auto  elem = errors.top();
            if (!elem.dirty()) {
                if (elem.valid()) {
                    if (checkSafety(elem.e)) {
                        collapse(elem.e);

                        auto v = elem.e->he->v;
                        Halfedge *trav = v->he;
                        do {
                            if (trav->e->unsafe) {
                                trav->e->unsafe = false;
                                errors.push({ trav->e, getCombinedError(trav->e) });
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
                errors.push({ e, getCombinedError(e) });
            }
        }
        else { //no more safe edges!
            countDeletedFaces = (unsigned int)faces.size();
        }
    }

    auto result = verify();
    if (result)
        std::cout << "ERROR, CODE " << std::oct << result << std::endl;
}

void Collapsible::draw() const {
    for (auto &f : faces)
        f.draw();
}
