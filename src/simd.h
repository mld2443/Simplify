#pragma once

#include <cmath>
#include <iostream>


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

template <typename T>
std::istream& operator>>(std::istream& is, v3<T>& v) {
    return is >> v.x >> v.y >> v.z;
}

template <typename T>
std::ostream& operator<<(std::ostream& os, const v3<T>& v) {
    return os << v.x << "" << v.y << " " << v.z;
}


using v3f = v3<float>;
