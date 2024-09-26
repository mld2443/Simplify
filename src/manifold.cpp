#include "manifold.h"

#ifdef DEBUG
#include <iostream>  // cout, oct, endl
#endif

#include <algorithm> // sort, set_intersection
#include <queue>     // priority_queue
#include <fstream>   // ifstream
#include <string>    // string, getline
#include <cctype>    // isdigit
#include <limits>    // numeric_limits::min, max
#include <map>       // map

using namespace std;


///////////////////////
// Bounds, Dimension //
///////////////////////
Manifold::AABB::Dimension::Dimension()
  : lo(numeric_limits<float>::max())
  , hi(numeric_limits<float>::min()) {
}

void Manifold::AABB::Dimension::addSample(float s) {
    if (s < lo) lo = s;
    if (s > hi) hi = s;
}

float Manifold::AABB::Dimension::delta() const {
    return hi - lo;
}

float Manifold::AABB::Dimension::centroid() const {
    return (hi + lo) / 2.0f;
}

void Manifold::AABB::addPoint(const v3f& v) {
    x.addSample(v.x);
    y.addSample(v.y);
    z.addSample(v.z);
}


//////////////
// Manifold //
//////////////
// PRIVATE FUNCTIONS

// very time expensive
// this is the only code that prevents working with non-triangular meshes
bool Manifold::checkSafety(const Edge *e) const {
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

#ifndef NDEBUG
void Manifold::verify() {
    m_vertices.remove_if(invalid());
    m_faces.remove_if(invalid());
    m_edges.remove_if(invalid());
    m_halfedges.remove_if(invalid());

    unsigned result = 0;

    for (auto &v : m_vertices) {
        if (v.valid) {
            if (!v.he->valid)
                result |= 1u<<0u;
        }
    }
    for (auto &f : m_faces) {
        if (f.valid) {
            if (!f.he->valid)
                result |= 1u<<1u;
            if (f.he->next->next->next != f.he)
                result |= 1u<<2u;
            if (f.he->prev->prev->prev != f.he)
                result |= 1u<<3u;
        }
    }
    for (auto &h : m_halfedges) {
        if (h.valid) {
            if (!h.v->valid)
                result |= 1u<<4u;
            if (!h.e->valid)
                result |= 1u<<5u;
            if (!h.f->valid)
                result |= 1u<<6u;
            if (&h != h.flip->flip)
                result |= 1u<<7u;
        }
    }
    for (auto &e : m_edges) {
        if (e.valid) {
            if (!e.he->valid)
                result |= 1u<<8u;
        }
    }

    if (result)
        cout << "ERROR, CODE " << oct << result << endl;
    else
        cout << "Clear skies, captain!" << endl;
}
#endif

Vertex* Manifold::addPoint(v3f& p) {
    m_vertices.push_back({ p, nullptr, true });
    return &m_vertices.back();
}

using edgeHashType = map<pair<const Vertex*, const Vertex*>, Edge*>;
static edgeHashType *p_edgeHash = nullptr;

void Manifold::addFace(const list<Vertex*>& verts) {
    static const auto lookupEdge = [&] (const Vertex *v1, const Vertex* v2) {
        const auto key = std::pair<const Vertex*, const Vertex*>(max(v1, v2), min(v1, v2));

        if (auto result = p_edgeHash->find(key); result != p_edgeHash->end())
            return result->second;

        m_edges.push_back({ nullptr, false, false, true });

        return (*p_edgeHash)[key] = &m_edges.back();
    };

    m_faces.push_back({ nullptr, true });

    Halfedge *first = nullptr, *prev = nullptr;

    for (auto it = verts.begin(); it != verts.end(); ++it) {
        auto next = it;
        if (++next == verts.end())
            next = verts.begin();

        Edge *e = lookupEdge(*it, *next);

        m_halfedges.push_back({ nullptr, prev, e->he, *it, e, &m_faces.back(), true });
        if (it != verts.begin())
            prev->next = &m_halfedges.back();

        if (e->he)
            e->he->flip = &m_halfedges.back();

        e->he = prev = &m_halfedges.back();
        if (it == verts.begin())
            first = prev;

        prev->v->he = prev;
    }

    prev->next = first;
    first->prev = m_faces.back().he = &m_halfedges.back();
}


// PUBLIC FUNCTIONS

Manifold::Manifold(const char* objfile, bool invert) {
    edgeHashType edgeHash;
    p_edgeHash = &edgeHash;

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

            m_bounds.addPoint(p);

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

    file.close();
    p_edgeHash = nullptr;

#ifndef NDEBUG
    verify();
#endif
}

v3f Manifold::getAABBSizes() const {
    return { m_bounds.x.delta(), m_bounds.y.delta(), m_bounds.z.delta() };
}

v3f Manifold::getAABBCentroid() const {
    return { m_bounds.x.centroid(), m_bounds.y.centroid(), m_bounds.z.centroid() };
}

void Manifold::draw() const {
    for (auto &f : m_faces)
        f.draw();
}
