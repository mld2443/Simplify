
//
//  halfedge.h
//  Simplify
//
//  Created by Matthew Dillard on 11/9/15.
//

#ifndef halfedge_h
#define halfedge_h

#include <GLUT/GLUT.h>
#include <OpenGL/gl.h>
#include <vector>
#include <math.h>
#include <list>

struct v3 {
    float x,y,z;
    
    v3 operator+(const v3& v) const { return {x + v.x, y + v.y, z + v.z}; }
    v3 operator-(const v3& v) const { return {x - v.x, y - v.y, z - v.z}; }
    v3 operator*(const float d) const { return {x * d, y * d, z * d}; }
    v3 operator/(const float d) const { return {x / d, y / d, z / d}; }
    v3* operator+=(const v3& v) { x += v.x; y += v.y; z += v.z; return this; }
    v3* operator-=(const v3& v) { x -= v.x; y -= v.y; z -= v.z; return this; }
    v3* operator*=(const float d) { x *= d; y *= d; z *= d; return this; }
    v3* operator/=(const float d) { x /= d; y /= d; z /= d; return this; }
    v3* operator=(const v3& v) { x = v.x; y = v.y; z = v.z; return this; }
    
    float abs() const { return sqrt(x*x + y*y + z*z); }
    float dot(const v3& v) const { return x*v.x + y*v.y + z*v.z; }
    v3 cross(const v3& v) const { return {y*v.z - z*v.y, z*v.x - x*v.z, x*v.y - y*v.x}; }
    v3 normalize() const { float mag = abs(); return {x/mag, y/mag, z/mag}; }
};

struct vertex;
struct edge;
struct face;

struct halfedge {
    halfedge *next, *prev, *flip;
    face *f;
    vertex *o;
    edge *e;
    bool valid;
    
    unsigned int collapse();
};

struct qef {
    float n;
    v3 Sv;
    float vtv;
    
    qef(const float _n=0, const v3& _Sv={}, const float _vtv=0): n(_n), Sv(_Sv), vtv(_vtv) {}
    qef(halfedge* he);
    
    float eval(const v3& v) const { return n * v.dot(v) - 2 * v.dot(Sv) + vtv; }
    
    qef operator+(const qef& q) const { return {n + q.n, Sv + q.Sv, vtv + q.vtv}; }
    qef* operator=(const qef& q) { n = q.n; Sv = q.Sv; vtv = q.vtv; return this; }
};

struct vertex {
    v3 pos;
    qef q;
    halfedge *he;
    bool valid;
    
    ~vertex() {}
    
    void calcQEF() { q = qef(he); }
    
    std::vector<vertex*> neighbors() const {
        std::vector<vertex*> neighborhood;
        
        halfedge *trav = he;
        do {
            neighborhood.push_back(trav->flip->o);
            trav = trav->flip->next;
        } while(trav != he);
        
        return neighborhood;
    }
    
    void update(vertex* v) {
        valid = false;
        
        halfedge *trav = he;
        do {
            trav->o = v;
            trav = trav->flip->next;
        } while(trav != he);
    }
    
    void markEdges();
    
    vertex* operator=(const vertex& v) { pos = v.pos; q = v.q; he = v.he; return this; }
};

struct edge {
    halfedge *he;
    bool dirty, valid;
    
    v3 midpoint() const { return (he->o->pos + he->flip->o->pos)/2; }
    v3 getNewPt() const { return (he->o->q.Sv + he->flip->o->q.Sv)/(he->o->q.n + he->flip->o->q.n); }
    float getCombinedError() const { return (he->o->q + he->flip->o->q).eval(getNewPt()); }
    
    unsigned int collapse() { valid = false; return he->collapse(); }
    
    void draw() const {
        GLfloat white[] = {1.0,1.0,1.0};
        glMaterialfv(GL_FRONT, GL_AMBIENT, white);
        glBegin(GL_LINES); {
            glVertex3d(he->o->pos.x, he->o->pos.y, he->o->pos.z);
            glVertex3d(he->flip->o->pos.x, he->flip->o->pos.y, he->flip->o->pos.z);
        } glEnd();
    }
};

struct face {
    halfedge *he;
    bool valid;
    
    v3 normal() const {
        return (he->next->next->o->pos - he->o->pos).cross(he->next->next->next->o->pos - he->next->o->pos).normalize();
    }
    
    v3 centroid() const {
        unsigned int val = 0;
        v3 r = {0,0,0};
        
        halfedge *trav = he;
        do {
            ++val;
            r += trav->o->pos;
            trav = trav->next;
        } while(trav != he);
        
        return r/val;
    }
    
    void draw() const {
        glBegin(GL_POLYGON); {
            v3 n = normal();
            glNormal3d(n.x, n.y, n.z);
            
            halfedge *trav = he;
            do {
                glVertex3d(trav->o->pos.x, trav->o->pos.y, trav->o->pos.z);
                trav = trav->next;
            } while(trav != he);
        } glEnd();
    }
};

#endif /* halfedge_h */
