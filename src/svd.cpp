//#include "svd.h"
#include "linearalgebra.h"

#include <iostream>
#include <cstdint>

using namespace std;
using linalg::Vector;
using linalg::Matrix;

#define TEST_VEC(...) #__VA_ARGS__ "=> " << __VA_ARGS__
#define TEST_MAT(...) #__VA_ARGS__ "=\n" << __VA_ARGS__


//////////
// MAIN //
//////////
int main() {
    // Vectors
    auto v1 = Vector{ 1.0, 2.0, 3.0 };
    cout << TEST_VEC(v1) << "\t<- a 'value-type' vector that owns its data." << endl;

    float reallyLongArray[] = { -1.0f, -3.0f, 0.0f,   1.0f,
                                4.2f,  3.9f,  -33.0f, 0.003f,
                                14.0f, 0.0f,  0.0f,   22.0f };
    auto v2 = linalg::VectorRef<float, 3uz, -3z>(reallyLongArray, 8uz);
    //                          type   size stride                offset

    cout << TEST_VEC(v2) << "\t<- a 'reference-type' vector that doesn't own.\nreallyLongArray=";
    for (const auto &e : reallyLongArray)
        cout << " " << e;
    cout << endl;

    for (auto &e : v2)
        ++e;

    cout << "incremented v2's elements.\nreallyLongArray=";
    for (const auto &e : reallyLongArray)
        cout << " " << e;
    cout << endl;

    // Utilities showcase
    auto f1 = [](double a){ return a > 0.0; };
    auto f2 = [](bool a, bool b){ return a && b; };
    v2 -= v1;
    cout << "v2-=v1; " TEST_VEC(v2) << "\t" << TEST_VEC(v1 - v2) << "\t" TEST_VEC(v1.cross(v2)) << "\t" TEST_VEC(v1.map(f1).fold(f2, true)) << "\t" TEST_VEC(v1.direction()) << endl;

    // Matrices
    constexpr auto m1 = Matrix{ { { 1.0, 0.0, 1.0, 0.0, 1.0 },
                                  { 0.0, 1.0, 0.0, 1.0, 0.0 } } };

    constexpr auto m2 = Matrix{ { { 1, 0, 1 },
                                  { 0, 1, 0 },
                                  { 1, 0, 1 },
                                  { 0, 1, 0 },
                                  { 1, 0, 1 } } };

    constexpr auto m3 = m1 * m2;

    cout << "constexpr " TEST_MAT(m1) << "\n" "constexpr " TEST_MAT(m2) << "\n" "constexpr " TEST_MAT(m3) << endl;

    // auto v4 = Vector{ 1.0, -1.0, -1.0, 0.5, 0.1 };
    // cout << TEST_MAT(m1) << "\n" TEST_VEC(m1.getRow(1)) << "\n" TEST_VEC(m1.getCol(3)) << "\n" TEST_VEC(v4) << "\n" TEST_VEC(m1 * v4) << endl;

    auto m5 = Matrix<unsigned, 5uz, 5uz>::I();

    m5.getRow(3uz) += Vector{ 1u, 4u, 20u, 3u, 3u };
    m5.getRow(0uz) = m5.getCol(4uz);
    m5.getDiagonal() *= 3u;

    cout << TEST_MAT(m5) << endl;
    cout << TEST_MAT(m1 + m1) << endl;

    return 0;
}
