//
//  manifold.h
//  Simplify
//
//  Created by Matthew Dillard on 11/9/15.
//

#ifndef manifold_h
#define manifold_h

#include <algorithm>
#include <iostream>
#include <utility>
#include <queue>
#include <map>

#include "element.h"

class manifold {
private:
    std::list<vertex> vertices;
    std::list<face> faces;
    std::list<edge> edges;
    std::list<halfedge> halfedges;
    
    std::map<std::pair<vertex*, vertex*>, edge*> edge_hash;
    
    unsigned int deleted_faces;
    
    edge* get_edge(std::list<edge>& edge_list, vertex *v1, vertex* v2) {
        auto key = std::pair<vertex*, vertex*>((v1 > v2)? v1 : v2, (v1 > v2)? v2 : v1);
        auto ii = edge_hash.find(key);
        
        if (ii != edge_hash.end())
            return ii->second;
        
        edge_list.push_back({nullptr, false, true});
        
        return edge_hash[key] = &edge_list.back();
    }
    
    // very time expensive
    bool checkSafety(const vertex* v1, const vertex* v2) const {
        auto v1nbrs(v1->neighbors());
        auto v2nbrs(v2->neighbors());
        std::sort(v1nbrs.begin(), v1nbrs.end());
        std::sort(v2nbrs.begin(), v2nbrs.end());
        std::vector<vertex*> intersect(v1nbrs.size() + v2nbrs.size());
        auto it = std::set_intersection(v1nbrs.begin(), v1nbrs.end(), v2nbrs.begin(), v2nbrs.end(), intersect.begin());
        
        if(it-intersect.begin() == 2)
            return true;
        
        return false;
    }
    
    bool collapse(edge *e) {
        if (!checkSafety(e->he->o, e->he->flip->o))
            return false;
        
        auto QEF(e->he->o->q + e->he->flip->o->q);
        e->he->o->pos = e->getNewPt();
        deleted_faces += e->collapse();
        
        e->he->o->q = QEF;
        e->he->o->markEdges();
        
        return true;
    }
    
public:
    manifold() {}
    
    vertex* add_vert(const float x, const float y, const float z) {
        vertices.push_back({{x,y,z},{},nullptr, true});
        return &vertices.back();
    }
    
    void add_face(const std::vector<vertex*>& ref, const std::vector<unsigned int> verts) {
        faces.push_back({nullptr, true});
        
        halfedge *first = nullptr, *prev = nullptr;
        
        for (int i = 0; i < verts.size(); ++i) {
            edge *e = get_edge(edges, ref[verts[i]], ref[verts[(i < verts.size() - 1)? i+1 : 0]]);
            
            halfedges.push_back({nullptr,prev,e->he,&faces.back(),ref[verts[i]],e, true});
            if (i > 0)
                prev->next = &halfedges.back();
            
            if (e->he)
                e->he->flip = &halfedges.back();
            
            e->he = prev = &halfedges.back();
            if (i == 0)
                first = prev;
            
            prev->o->he = prev;
        }
        
        prev->next = first;
        first->prev = faces.back().he = &halfedges.back();
    }
    
    void cleanup() {
        edge_hash.clear();
        deleted_faces = 0;
        
        for (auto &v : vertices)
            v.calcQEF();
    }
    
    void simplify(const unsigned int count) {
        //construct PQ
        std::priority_queue<element, std::vector<element>, elementComp> errors;
        for (auto &e : edges) {
            errors.push(&e);
        }
        
        std::list<element> unsafe_edges;
        
        //do the algorithm
        while (faces.size() - deleted_faces > count) {
            if (!errors.top().dirty()) {
                if (errors.top().valid()) {
                    while (!collapse(errors.top().e)) {
                        std::cout << "unsafe" << std::endl;
                        unsafe_edges.push_back(errors.top());
                        errors.pop();
                    }
                    if (!unsafe_edges.empty()) {
                        for (auto &elem : unsafe_edges)
                            errors.push(elem);
                        
                        unsafe_edges.clear();
                    }
                }
                else {
                    std::cout << "invalid" << std::endl;
                }
                errors.pop();
            }
            else {
                auto e = errors.top().e;
                errors.pop();
                e->dirty = false;
                errors.push(e);
            }
        }
        
        //update the manifold's data structures to delete obsolete data
        for (auto it = vertices.begin(); it != vertices.end(); ++it) {
            if (!it->valid)
                vertices.erase(it);
        }
        for (auto it = faces.begin(); it != faces.end(); ++it) {
            if (!it->valid)
                faces.erase(it);
        }
        for (auto it = edges.begin(); it != edges.end(); ++it) {
            if (!it->valid)
                edges.erase(it);
        }
        for (auto it = halfedges.begin(); it != halfedges.end(); ++it) {
            if (!it->valid)
                halfedges.erase(it);
        }
    }
    
    void draw(const bool drawcontrol) const {
        for (auto &f : faces) {
            if (f.valid)
                f.draw();
            else
                std::cout << "invalid!" << std::endl;
        }
    }
};

#endif /* manifold_h */
