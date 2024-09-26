#pragma once

#include <cmath>


template <typename T>
struct v3 {
    T x,y,z;

    v3   operator+(const v3& v) const { return { x + v.x, y + v.y, z + v.z }; }
    v3   operator-(const v3& v) const { return { x - v.x, y - v.y, z - v.z }; }
    v3   operator*(T d)         const { return { x * d,   y * d,   z * d };   }
    v3   operator/(T d)         const { return { x / d,   y / d,   z / d };   }
    v3& operator+=(const v3& v) { x += v.x; y += v.y; z += v.z; return *this; }
    v3& operator-=(const v3& v) { x -= v.x; y -= v.y; z -= v.z; return *this; }
    v3& operator*=(T d)         { x *= d;   y *= d;   z *= d;   return *this; }
    v3& operator/=(T d)         { x /= d;   y /= d;   z /= d;   return *this; }
    v3&  operator=(const v3& v) { x = v.x;  y = v.y;  z = v.z;  return *this; }

    T dot(const v3& v) const { return x*v.x  + y*v.y  + z*v.z; }
    v3 cross(const v3& v) const { return { y*v.z - z*v.y, z*v.x - x*v.z, x*v.y - y*v.x }; }

    T lengthSqr() const { return this->dot(*this); }
    T length() const { return std::sqrt(lengthSqr()); }
    v3 normalize() const { return *this / length(); }
};

using v3f = v3<float>;
