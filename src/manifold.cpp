#include "manifold.h"

#include <GL/gl.h>   // glBegin, glEnd, glMaterialfv, GL_*
#include <fstream>   // ifstream
#include <string>    // string, getline
#include <cctype>    // isdigit
#include <limits>    // numeric_limits::min, max
#include <map>       // map
#include <iterator>  // next

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
#ifndef NDEBUG
template <class VertexType>
void Manifold<VertexType>::verifyConnections() {
    for (const auto &halfedge : m_halfedges) {
        if (!halfedge.invalid()) {
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
        if (!vertex.invalid() && vertex.he->v != &vertex)
                throw 1u<<4u;

    for (const auto &edge : m_edges)
        if (!edge.invalid() && edge.he->e != &edge)
                throw 1u<<5u;

    for (const auto &face : m_faces) {
        if (!face.invalid()) {
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

template <class VertexType>
Manifold<VertexType>::Manifold(const char* objfile) : m_trianglesOnly(true) {
    map<pair<const Vertex*, const Vertex*>, Edge*> edgeHash;

    vector<Vertex*> vertexPointers;
    ifstream file(objfile);
    string token;

    while (!file.eof()) {
        file >> token;
        if (token[0] == '#') {
            // Discard comments
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

            m_faces.emplace_back(nullptr);
            Halfedge *first = nullptr, *prev = nullptr;

            for (auto index = faceVerts.begin(); index != faceVerts.end(); ++index) {
                auto nextIndex = next(index);
                if (nextIndex == faceVerts.end())
                    nextIndex = faceVerts.begin();

                Edge *edge = nullptr;
                const auto key = pair<const Vertex*, const Vertex*>(max(*index, *nextIndex), min(*index, *nextIndex));

                if (auto result = edgeHash.find(key); result != edgeHash.end()) {
                    edge = result->second;
                } else {
                    m_edges.emplace_back(nullptr, false, false);
                    edge = edgeHash[key] = &m_edges.back();
                }

                m_halfedges.emplace_back(nullptr, prev, edge->he, *index, edge, &m_faces.back());
                if (index != faceVerts.begin())
                    prev->next = &m_halfedges.back();

                if (edge->he)
                    edge->he->flip = &m_halfedges.back();

                edge->he = prev = &m_halfedges.back();

                if (!first)
                    first = prev;

                prev->v->he = prev;
            }

            prev->next = first;
            first->prev = m_faces.back().he = &m_halfedges.back();
        }
    }

    file.close();

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
    list<const Face*> nonTris;

    // Should be faster
    static const GLfloat white[] = { 1.0f, 1.0f, 1.0f };
    glEnable(GL_LIGHTING);
    glMaterialfv(GL_FRONT, GL_AMBIENT, white);
    glBegin(GL_TRIANGLES); {
        for (const Face &face : m_faces)
            if (m_trianglesOnly || face.isTriangle())
                face.draw();
            else
                nonTris.push_back(&face);
    } glEnd();

    // Slower but draws degree 4+ polys correctly
    static const GLfloat blue[] = { 0.6f, 0.6f, 1.0f };
    glMaterialfv(GL_FRONT, GL_AMBIENT, blue);
    for (const Face *face : nonTris) {
        glBegin(GL_POLYGON); {
            face->draw();
        } glEnd();
    }
}

template <class VertexType>
void Manifold<VertexType>::drawEdges() const {
    static const GLfloat yellow[] = { 1.0f, 1.0f, 0.0f, 1.0f };
    glDisable(GL_LIGHTING);
    glColor4fv(yellow);
    glBegin(GL_LINES); {
        for (const Edge &edge : m_edges)
            edge.draw();
    } glEnd();
}

template <class VertexType>
void Manifold<VertexType>::drawVertices() const {
    static const GLfloat red[] = { 1.0f, 0.0f, 0.0f, 1.0f };
    glDisable(GL_LIGHTING);
    glColor4fv(red);
    glBegin(GL_POINTS); {
        for (const Vertex &vertex : m_vertices)
            vertex.draw();
    } glEnd();
}


//////////////////////////////////////
// TEMPLATE DECLARATIONS FOR SANITY //
//////////////////////////////////////
#include "errorfunction.h"
template class Manifold<Vertex>;
template class Manifold<QEFVertex>;
