#pragma once

#include <iostream>
#include <fstream>
#include <utility>
#include <float.h>
#include <cctype>
#include <vector>
#include <map>


template <typename T>
struct Bounds {
    struct Dimension {
        T lo = FLT_MAX, hi = FLT_MIN;

        void addSample(T s) {
            if (s < lo) lo = s;
            if (s > hi) hi = s;
        }

        T delta() const { return hi - lo; }
    };

    Dimension x, y, z;

    void addPoint(T _x, T _y, T _z) {
        x.addSample(_x);
        y.addSample(_y);
        z.addSample(_z);
    }
};

Bounds<float> bounds;
Manifold m;

void loadOBJ(const char *filename, const bool invert = false) {
    m.clear();

    std::vector<Vertex*> v_pointers;
    std::ifstream file(filename);
    std::string token;

    while (!file.eof()) {
        file >> token;
        if (token[0] == '#') {
            std::string dummy;
            getline(file, dummy);
        }
        else if (token[0] == 'v' && token[1] != 'n') {
            float x, y, z;
            file >> x >> y >> z;

            bounds.addPoint(x, y, z);

            v_pointers.push_back(m.add_vert(x, y, z));
        }
        else if (token[0] == 'f') {
            std::list<Vertex*> face_verts;
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
