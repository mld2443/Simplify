#include "manifold.h"

#include <GL/gl.h>   // glBegin, glEnd, glMaterialfv, GL_*
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
template <class VertexType>
Manifold<VertexType>::AABB::Dimension::Dimension()
  : lo(numeric_limits<float>::max())
  , hi(numeric_limits<float>::min()) {
}

template <class VertexType>
void Manifold<VertexType>::AABB::Dimension::addSample(float s) {
    if (s < lo) lo = s;
    if (s > hi) hi = s;
}

template <class VertexType>
float Manifold<VertexType>::AABB::Dimension::delta() const {
    return hi - lo;
}

template <class VertexType>
float Manifold<VertexType>::AABB::Dimension::centroid() const {
    return (hi + lo) / 2.0f;
}

template <class VertexType>
void Manifold<VertexType>::AABB::addSample(const f32v3& v) {
    x.addSample(v.x);
    y.addSample(v.y);
    z.addSample(v.z);
}


//////////////
// Manifold //
//////////////
// PRIVATE FUNCTIONS

using edgeHashType = map<pair<const Vertex*, const Vertex*>, Edge*>;
static edgeHashType *s_edgeHash = nullptr;

template <class VertexType>
void Manifold<VertexType>::addFace(const list<Vertex*>& verts) {
    static const auto lookupEdge = [&] (const Vertex *v1, const Vertex* v2) {
        const auto key = pair<const Vertex*, const Vertex*>(max(v1, v2), min(v1, v2));

        if (auto result = s_edgeHash->find(key); result != s_edgeHash->end())
            return result->second;

        m_edges.emplace_back(nullptr, false, false);

        return (*s_edgeHash)[key] = &m_edges.back();
    };

    m_faces.emplace_back(nullptr);

    Halfedge *first = nullptr, *prev = nullptr;

    for (auto it = verts.begin(); it != verts.end(); ++it) {
        auto next = it;
        if (++next == verts.end())
            next = verts.begin();

        Edge *e = lookupEdge(*it, *next);

        m_halfedges.emplace_back(nullptr, prev, e->he, *it, e, &m_faces.back());
        if (it != verts.begin())
            prev->next = &m_halfedges.back();

        if (e->he)
            e->he->flip = &m_halfedges.back();

        e->he = prev = &m_halfedges.back();
        if (!first)
            first = prev;

        prev->v->he = prev;
    }

    prev->next = first;
    first->prev = m_faces.back().he = &m_halfedges.back();
}

#ifndef NDEBUG
template <class VertexType>
void Manifold<VertexType>::verifyConnections() {
    for (const auto &halfedge : m_halfedges) {
        if (halfedge) {
            if (halfedge.v->he == nullptr)
                throw 1u<<0u;
            if (halfedge.e->he == nullptr)
                throw 1u<<1u;
            if (halfedge.f->he == nullptr)
                throw 1u<<2u;
            if (halfedge.flip->flip != &halfedge)
                throw 1u<<3u;
        }
    }

    for (const auto &vertex : m_vertices)
        if (vertex && vertex.he->v != &vertex)
                throw 1u<<4u;

    for (const auto &edge : m_edges)
        if (edge && edge.he->e != &edge)
                throw 1u<<5u;

    for (const auto &face : m_faces) {
        if (face) {
            if (face.he->f != &face)
                throw 1u<<6u;
            if (m_trianglesOnly) {
                if (face.he->next->next->next != face.he)
                    throw 1u<<7u;
                if (face.he->prev->prev->prev != face.he)
                    throw 1u<<8u;
            }
        }
    }
}
#endif


// PUBLIC FUNCTIONS

template <class VertexType>
Manifold<VertexType>::Manifold(const char* objfile) : m_trianglesOnly(true) {
    edgeHashType edgeHash;
    s_edgeHash = &edgeHash;

    vector<Vertex*> vertexPointers;
    ifstream file(objfile);
    string token;

    while (!file.eof()) {
        file >> token;
        // Discard comments
        if (token[0] == '#') {
            string dummy;
            getline(file, dummy);
        } else if (token[0] == 'v' && token[1] != 'n') {
        // Process vertices, discard normals
            f32v3 p;
            file >> p;

            m_bounds.addSample(p);

            m_vertices.push_back({ nullptr, p });
            vertexPointers.push_back(&m_vertices.back());
        } else if (token[0] == 'f') {
            // Process faces
            list<Vertex*> faceVerts;
            file >> ws;
            while (isdigit(file.peek())) {
                string vnum;
                file >> vnum >> ws;

                // Again, discard indices to textures and normals
                if (size_t found = vnum.find("/"); found != string::npos)
                    vnum = vnum.substr(0, found);

                faceVerts.push_back(vertexPointers[stoul(vnum) - 1]);
            }

            if (faceVerts.size() > 3ul)
                m_trianglesOnly = false;

            addFace(faceVerts);
        }
    }

    file.close();
    s_edgeHash = nullptr;

#ifndef NDEBUG
    verifyConnections();
#endif
}

template <class VertexType>
f32v3 Manifold<VertexType>::getAABBSizes() const {
    return { m_bounds.x.delta(), m_bounds.y.delta(), m_bounds.z.delta() };
}

template <class VertexType>
f32v3 Manifold<VertexType>::getAABBCentroid() const {
    return { m_bounds.x.centroid(), m_bounds.y.centroid(), m_bounds.z.centroid() };
}

template <class VertexType>
void Manifold<VertexType>::drawFaces() const {
    static const GLfloat white[] = { 1.0f, 1.0f, 1.0f };
    glEnable(GL_LIGHTING);
    glMaterialfv(GL_FRONT, GL_AMBIENT, white);

    list<const Face*> nonTris;

    // Should be faster
    glBegin(GL_TRIANGLES); {
        for (const auto &f : m_faces)
        if (m_trianglesOnly || f.isTriangle())
            f.draw();
        else
            nonTris.push_back(&f);
    } glEnd();

    static const GLfloat blue[] = { 0.6f, 0.6f, 1.0f };
    glMaterialfv(GL_FRONT, GL_AMBIENT, blue);
    for (const auto *f : nonTris) {
        glBegin(GL_POLYGON); {
            f->draw();
        } glEnd();
    }
}

template <class VertexType>
void Manifold<VertexType>::drawEdges() const {
    static const GLfloat yellow[] = { 1.0f, 1.0f, 0.0f, 1.0f };
    glDisable(GL_LIGHTING);
    glColor4fv(yellow);
    glBegin(GL_LINES); {
        for (const auto &e : m_edges)
            e.draw();
    } glEnd();
}

template <class VertexType>
void Manifold<VertexType>::drawVertices() const {
    static const GLfloat red[] = { 1.0f, 0.0f, 0.0f, 1.0f };
    glDisable(GL_LIGHTING);
    glColor4fv(red);
    glBegin(GL_POINTS); {
        for (const auto &v : m_vertices)
            v.draw();
    } glEnd();
}


//////////////////////////////////////
// TEMPLATE DECLARATIONS FOR SANITY //
//////////////////////////////////////
#include "errorfunction.h"
template class Manifold<Vertex>;
template class Manifold<QEFVertex>;
