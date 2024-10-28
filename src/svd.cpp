//#include "svd.h"
#include "linearalgebra.h"

#include <iostream>

using namespace std;
//using linalg::Vector;
using linalg::Matrix;

#define STR_EVAL(...) #__VA_ARGS__ "=> " << __VA_ARGS__
#define LINE_EVAL(...) #__VA_ARGS__ "=\n" << __VA_ARGS__

// special
//  1 horizontal
//  2 v \n
//  3 d \n, \t per X
// pattern start
//  4 v \n
//  5 v \n\n
//  6 d \n\n, \t per X
//  7 v \n
//  8 v \n\n
//  9 d \n\n, \t\t per X
// 10 v \n
// 11 v \n\n
// 12 d \n\n, \t\t\t per X
// ...

//////////
// MAIN //
//////////
int main() {
    // Massive 6-dimensional multilinear tensor
    [[maybe_unused]] constexpr auto tensor1 = linalg::Tensor<linalg::ValueType, 1z, int, 2uz, 2uz, 2uz, 2uz, 2uz, 2uz>{
         0,  1,
         2,  3,
             4,  5,
             6,  7,
         8,  9,
        10, 11,
            12, 13,
            14, 15,

        16, 17,
        18, 19,
            20, 21,
            22, 23,
        24, 25,
        26, 27,
            28, 29,
            30, 31,

            32, 33,
            34, 35,
                36, 37,
                38, 39,
            40, 41,
            42, 43,
                44, 45,
                46, 47,

            48, 49,
            50, 51,
                52, 53,
                54, 55,
            56, 57,
            58, 59,
                60, 61,
                62, 63,
    };

    cout << STR_EVAL(sizeof(tensor1)) << ", " STR_EVAL(tensor1) << endl;
    cout << STR_EVAL(tensor1[0, 1, 1, 0, 0, 1]) << ", " STR_EVAL(tensor1[0][1][1][0][0][1]) << endl;
    // Test for compile-time evaluation using array bounds
    int emptyIntArray[tensor1[0][1, 1, 0][0, 1]] = {};
    cout << STR_EVAL(sizeof(emptyIntArray)/sizeof(int)) << endl;

    // Matrices
    // constexpr auto m1 = Matrix{ { { 1.0, 0.0, 1.0, 0.0, 1.0 },
    //                               { 0.0, 1.0, 0.0, 1.0, 0.0 } } };

    // constexpr auto m2 = Matrix{ { { 1, 0, 1 },
    //                               { 0, 1, 0 },
    //                               { 1, 0, 1 },
    //                               { 0, 1, 0 },
    //                               { 1, 0, 1 } } };

    // constexpr auto m3 = m1 * m2;

    // cout << "constexpr " LINE_EVAL(m1) << "\n" "constexpr " LINE_EVAL(m2) /*<< "\n" "constexpr " LINE_EVAL(m3)*/ << endl;

//    // Vectors
//    auto v1 = Vector{ 1.0, 2.0, 3.0 };
//    cout << STR_EVAL(v1) << "\t<- a 'value-type' vector that owns its data." << endl;
//
//    float reallyLongArray[] = { -1.0f, -3.0f, 0.0f,   1.0f,
//                                4.2f,  3.9f,  -33.0f, 0.003f,
//                                14.0f, 0.0f,  0.0f,   22.0f };
//    auto v2 = linalg::VectorRef<float, 3uz, -3z>(reallyLongArray, 8uz);
//    //                          type   size stride                offset
//
//    cout << STR_EVAL(v2) << "\t<- a 'reference-type' vector that doesn't own.\nreallyLongArray=";
//    for (const auto &e : reallyLongArray)
//        cout << " " << e;
//    cout << endl;
//
//    for (auto &e : v2)
//        ++e;
//
//    cout << "incremented v2's elements.\nreallyLongArray=";
//    for (const auto &e : reallyLongArray)
//        cout << " " << e;
//    cout << endl;
//
//    // Utilities showcase
//    auto f1 = [](double a){ return a > 0.0; };
//    auto f2 = [](bool a, bool b){ return a && b; };
//    v2 -= v1;
//    cout << "v2-=v1; " STR_EVAL(v2) << "\t" << STR_EVAL(v2*4u) << "\t" << STR_EVAL(v1 - v2) << "\t" STR_EVAL(v1.cross(v2)) << "\t" STR_EVAL(v1.map(f1).fold(f2, true)) << "\t" STR_EVAL(v1.direction()) << endl;
//
//    // Matrices
//    constexpr auto m1 = Matrix{ { { 1.0, 0.0, 1.0, 0.0, 1.0 },
//                                  { 0.0, 1.0, 0.0, 1.0, 0.0 } } };
//
//    constexpr auto m2 = Matrix{ { { 1, 0, 1 },
//                                  { 0, 1, 0 },
//                                  { 1, 0, 1 },
//                                  { 0, 1, 0 },
//                                  { 1, 0, 1 } } };
//
//    constexpr auto m3 = m1 * m2;
//
//    cout << "constexpr " LINE_EVAL(m1) << "\n" "constexpr " LINE_EVAL(m2) << "\n" "constexpr " LINE_EVAL(m3) << endl;
//
//    // auto v4 = Vector{ 1.0, -1.0, -1.0, 0.5, 0.1 };
//    // cout << LINE_EVAL(m1) << "\n" STR_EVAL(m1.getRow(1)) << "\n" STR_EVAL(m1.getCol(3)) << "\n" STR_EVAL(v4) << "\n" STR_EVAL(m1 * v4) << endl;
//
//    auto m5 = Matrix<unsigned, 5uz, 5uz>::I();
//
//    m5.getRow(3uz) += Vector<uint32_t, 5uz>{ 4u };
//    m5.getRow(0uz) = m5.getCol(4uz);
//    m5.getDiagonal() *= 3u;
//
//    cout << LINE_EVAL(m5) << endl;
//    cout << LINE_EVAL(m1 + m1) << endl;

    return 0;
}
