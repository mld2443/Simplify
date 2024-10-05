#pragma once

#include <cmath>     // sqrt
#include <iostream>  // istream, ostream
#include <algorithm> // max


template <typename T>
struct v3 {
    T x,y,z;

    inline v3   operator-()            const { return { -x, -y, -z };                }
    inline v3   operator+(const v3& v) const { return { x + v.x, y + v.y, z + v.z }; }
    inline v3   operator-(const v3& v) const { return { x - v.x, y - v.y, z - v.z }; }
    inline v3   operator*(T d)         const { return { x * d,   y * d,   z * d };   }
    inline v3   operator/(T d)         const { return { x / d,   y / d,   z / d };   }
    inline v3& operator+=(const v3& v) { x += v.x; y += v.y; z += v.z; return *this; }
    inline v3& operator-=(const v3& v) { x -= v.x; y -= v.y; z -= v.z; return *this; }
    inline v3& operator*=(T d)         { x *= d;   y *= d;   z *= d;   return *this; }
    inline v3& operator/=(T d)         { x /= d;   y /= d;   z /= d;   return *this; }
    inline v3&  operator=(const v3& v) { x = v.x;  y = v.y;  z = v.z;  return *this; }

    inline T    dot(const v3& v) const { return x*v.x + y*v.y + z*v.z;                           }
    inline v3 cross(const v3& v) const { return { y*v.z - z*v.y, z*v.x - x*v.z, x*v.y - y*v.x }; }

    inline T  lengthSqr() const { return this->dot(*this);       }
    inline T     length() const { return std::sqrt(lengthSqr()); }
    inline v3 normalize() const { return *this / length();       }

    inline T max() const { return std::max({ x, y, z });  }
};

template <typename T>
std::istream& operator>>(std::istream& is, v3<T>& v) {
    return is >> v.x >> v.y >> v.z;
}

template <typename T>
std::ostream& operator<<(std::ostream& os, const v3<T>& v) {
    return os << v.x << " " << v.y << " " << v.z;
}


using f32v3 = v3<float>;
