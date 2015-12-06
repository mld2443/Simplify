//
//  fileio.h
//  Simplify
//
//  Created by Matthew Dillard on 11/8/15.
//

#ifndef fileio_h
#define fileio_h

#include <iostream>
#include <fstream>
#include <utility>
#include <float.h>
#include <cctype>
#include <vector>
#include <map>

float xlow, ylow, zlow, xhigh, yhigh, zhigh;

manifold m;

void load(const char *filename, const bool invert = false) {
    m.clear();
    
    std::vector<vertex*> v_pointers;
    xlow = ylow = zlow = FLT_MAX;
    xhigh = yhigh = zhigh = -FLT_MAX;
    
    std::ifstream file(filename);
    std::string token;
    
    while (!file.eof()) {
        file >> token;
        if(token[0] == '#'){
            std::string dummy;
            getline(file, dummy);
        }
        else if (token[0] == 'v' && token[1] != 'n') {
            float x, y, z;
            file >> x >> y >> z;
            
            {
                if (x < xlow)
                    xlow = x;
                else if (x > xhigh)
                    xhigh = x;
                if (y < ylow)
                    ylow = y;
                else if (y > yhigh)
                    yhigh = y;
                if (z < zlow)
                    zlow = z;
                else if (z > zhigh)
                    zhigh = z;
            }
            
            v_pointers.push_back(m.add_vert(x, y, z));
        }
        else if (token[0] == 'f') {
            std::list<vertex*> face_verts;
            file >> std::ws;
            while (std::isdigit(file.peek())) {
                std::string vnum;
                file >> vnum >> std::ws;
                
                std::size_t found = vnum.find("//");
                if (found!=std::string::npos)
                    vnum = vnum.substr(0,found);
                
                if (invert)
                    face_verts.push_front(v_pointers[std::stoul(vnum)-1]);
                else
                    face_verts.push_back(v_pointers[std::stoul(vnum)-1]);
            }
            
            m.add_face(face_verts);
        }
    }
    
    m.cleanup();
}


#endif /* fileio_h */
