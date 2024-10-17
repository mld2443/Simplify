#include "svd.h"

#include <iostream>

using namespace std;
using linalg::Vector;


//////////
// MAIN //
//////////
int main() {
    auto v1 = Vector{ 1.0, 2.0, 3.0 };
    cout << "v1: " << v1 << "\ta 'value-type' vector that owns its data." << endl;

    float reallyLongArray[] = { -1.0f, -3.0f, 0.0f,   1.0f,
                                4.2f,  3.9f,  -33.0f, 0.003f,
                                14.0f, 0.0f,  0.0f,   22.0f };
    auto v2 = Vector<float, 3uz, 3uz, linalg::VecRef>(reallyLongArray, 2uz);
    //               type   size stride    ref/value                   offset

    cout << "v2: " << v2 << "\ta 'reference-type' vector that doesn't own.\nreallyLongArray:";
    for (const auto &e : reallyLongArray)
        cout << " " << e;
    cout << endl;

    for (auto &e : v2)
        e++;

    cout << "added 1 to v2's elements.\nreallyLongArray:";
    for (const auto &e : reallyLongArray)
        cout << " " << e;
    cout << endl;

    // Utilities showcase
    v2 -= v1;
    cout << "v2-=v1; v2: " << v2 << ", v2*4.0: " << v2 * 4.0 << ", v1*v2: " << v1.dot(v2) << ", v1.direction(): " << v1.direction() << endl;

    return 0;
}