//
//  manifold.h
//  Simplify
//
//  Created by Matthew Dillard on 11/9/15.
//

#ifndef manifold_h
#define manifold_h

#include <iostream>
#include <utility>
#include <vector>
#include <list>
#include <map>

#include "halfedge.h"

class manifold {
private:
    std::list<vertex> vertices0;
    std::list<face> faces0;
    std::list<edge> edges0;
    std::list<halfedge> halfedges0;
    
    std::list<vertex> *vertices;
    std::list<face> *faces;
    std::list<edge> *edges;
    std::list<halfedge> *halfedges;
    
    std::map<std::pair<vertex*, vertex*>, edge*> edge_hash;
    
    edge* get_edge(std::list<edge>& edge_list, vertex *v1, vertex* v2) {
        auto key = std::pair<vertex*, vertex*>((v1 > v2)? v1 : v2, (v1 > v2)? v2 : v1);
        auto ii = edge_hash.find(key);
        
        if (ii != edge_hash.end())
            return ii->second;
        
        edge_list.push_back({v1, v2, nullptr});
        
        return edge_hash[key] = &edge_list.back();
    }
    
public:
    manifold() {}
    
    vertex* add_vert(const float x, const float y, const float z) {
        vertices0.push_back({{x,y,z},{},nullptr});
        return &vertices0.back();
    }
    
    void add_face(const std::vector<vertex*>& ref, const std::vector<unsigned int> verts) {
        faces0.push_back({nullptr});
        
        halfedge *first = nullptr, *prev = nullptr;
        
        for (int i = 0; i < verts.size(); ++i) {
            edge *e = get_edge(edges0, ref[verts[i]], ref[verts[(i < verts.size() - 1)? i+1 : 0]]);
            
            halfedges0.push_back({nullptr,prev,e->he,&faces0.back(),ref[verts[i]],e});
            if (i > 0)
                prev->next = &halfedges0.back();
            
            if (e->he)
                e->he->flip = &halfedges0.back();
            
            e->he = prev = &halfedges0.back();
            if (i == 0)
                first = prev;
            
            prev->o->he = prev;
        }
        
        prev->next = first;
        first->prev = faces0.back().he = &halfedges0.back();
    }
    
    void cleanup() {
        edge_hash.clear();
        vertices = &vertices0;
        faces = &faces0;
        edges = &edges0;
        halfedges = &halfedges0;
        
        for (auto &v : vertices0)
            v.calcQEF();
    }
    
    void simplify(const unsigned int count) {
        for (auto &e : edges0) {
            
        }
    }
    
    void draw(const bool drawcontrol) {
        if (drawcontrol) {
            if (edges->size() > edges0.size())
                for (auto &e : edges0)
                    e.draw();
        }
        for (auto &f : *faces)
            f.draw();
    }
};

#endif /* manifold_h */
