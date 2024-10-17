#pragma once

#include <concepts> // same_as
#include <cmath>    // sqrt
#include <iostream> // ostream
#include <utility>  // index_sequence, make_index_sequence


namespace linalg {
    ////////////
    // VECTOR //
    ////////////

    // Vector value-type base class
    template <typename T, size_t N, size_t>
    class VecVal {
    private:
        T data[N];

    protected:
        template <std::same_as<T>... Ts>
        VecVal(Ts&&... args) : data{ args... } {}

        T&       get(size_t i)       { return data[i]; }
        const T& get(size_t i) const { return data[i]; }

        inline T* beginImpl() { return data;     }
        inline T*   endImpl() { return data + N; }
        inline const T* beginImpl() const { return data;     }
        inline const T*   endImpl() const { return data + N; }
    };

    // Vector reference-type base class
    template <typename T, size_t N, size_t STRIDE>
    class VecRef {
    public:
        template <typename PointerType>
        struct Iterator {
            PointerType* pos;

            PointerType& operator*() const { return *pos; }
            Iterator& operator++() { pos += STRIDE; return *this; }
            bool operator==(const Iterator& o) const { return pos == o.pos; }
        };

    private:
        T* data;

    protected:
        VecRef(T* origin, size_t offset) : data(origin + offset) {}

        T&       get(size_t i)       { return data[i * STRIDE]; }
        const T& get(size_t i) const { return data[i * STRIDE]; }

        inline Iterator<T> beginImpl() { return { data };              }
        inline Iterator<T>   endImpl() { return { data + N * STRIDE }; }
        inline Iterator<const T> beginImpl() const { return { data };              }
        inline Iterator<const T>   endImpl() const { return { data + N * STRIDE }; }
    };

    // Generic Vector class that can transparently perform operations and transformations on and between reference and
    // value type vectors of arbitrary, compile-time dimensions, even between mixed data types, such as float4 + int4.
    template <typename T, size_t N, size_t STRIDE = 1uz, template<typename, size_t, size_t> class VECTYPE = VecVal>
    class Vector : private VECTYPE<T, N, STRIDE> {
    private:
        template <typename NEWTYPE>
        using ReturnVec = Vector<NEWTYPE, N, 1uz, VecVal>;

        // Dirty mapping function implementations
        template <size_t... INDEX>
        inline auto mapInternal(auto func, std::index_sequence<INDEX...>) const {
            return ReturnVec<decltype(func(T()))>{ func(this->get(INDEX))... };
        }
        template <size_t... INDEX>
        inline void mapWriteInternal(auto func, std::index_sequence<INDEX...>) {
            (func(this->get(INDEX)), ...);
        }
        inline void mapWrite(auto func) {
            mapWriteInternal(func, std::make_index_sequence<N>{});
        }
        template <typename T2, size_t STRIDE2, template<typename, size_t, size_t> class OTHERTYPE, size_t... INDEX>
        inline auto binaryMapInternal(auto func, const Vector<T2, N, STRIDE2, OTHERTYPE>& v, std::index_sequence<INDEX...>) const {
            return ReturnVec<decltype(func(T(),T2()))>{ func(this->get(INDEX), v[INDEX])... };
        }
        template <typename T2, size_t STRIDE2, template<typename, size_t, size_t> class OTHERTYPE>
        inline auto binaryMap(auto func, const Vector<T2, N, STRIDE2, OTHERTYPE>& v) const {
            return binaryMapInternal(func, v, std::make_index_sequence<N>{});
        }
        template <typename T2, size_t STRIDE2, template<typename, size_t, size_t> class OTHERTYPE, size_t... INDEX>
        inline void binaryMapWriteInternal(auto func, const Vector<T2, N, STRIDE2, OTHERTYPE>& v, std::index_sequence<INDEX...>) {
            (func(this->get(INDEX), v[INDEX]), ...);
        }
        template <typename T2, size_t STRIDE2, template<typename, size_t, size_t> class OTHERTYPE>
        inline void binaryMapWrite(auto func, const Vector<T2, N, STRIDE2, OTHERTYPE>& v) {
            binaryMapWriteInternal(func, v, std::make_index_sequence<N>{});
        }
        template <typename T2, size_t STRIDE2, template<typename, size_t, size_t> class OTHERTYPE, size_t... INDEX>
        inline auto dotInternal(const Vector<T2, N, STRIDE2, OTHERTYPE>& v, std::index_sequence<INDEX...>) const {
            return ((this->get(INDEX) * v[INDEX]) + ...);
        }

    public:
        // Value-type constructor
        template <std::same_as<T>... Ts>
        Vector(Ts&&... data) : VECTYPE<T, N, STRIDE>(static_cast<T&&>(data)...) {}
        // Reference-type constructor
        Vector(T* origin, size_t offset = 0uz) : VECTYPE<T, N, STRIDE>(origin, offset) {}

        // Accessors
        T&       operator[](size_t i)       { return this->get(i); }
        const T& operator[](size_t i) const { return this->get(i); }

        // Iterators for loops
        inline auto begin() { return this->beginImpl(); }
        inline auto   end() { return this->endImpl();   }
        inline const auto begin() const { return this->beginImpl(); }
        inline const auto   end() const { return this->endImpl();   }

        // Public mapping method
        inline auto map(auto func) const { return mapInternal(func, std::make_index_sequence<N>{}); }

        inline auto operator-() const { return map([](auto& d){ return -d; }); }
        template <typename T2>
        inline auto operator*(const T2& s) const { return map([&s](const T& e){ return e * s; }); }
        template <typename T2>
        inline auto operator/(const T2& s) const { return map([&s](const T& e){ return e / s; }); }
        template <typename T2, size_t STRIDE2, template<typename, size_t, size_t> class OTHERTYPE>
        inline auto operator+(const Vector<T2, N, STRIDE2, OTHERTYPE>& v) const { return binaryMap([](const T& e1, const T2& e2){ return e1 + e2; }, v); }
        template <typename T2, size_t STRIDE2, template<typename, size_t, size_t> class OTHERTYPE>
        inline auto operator-(const Vector<T2, N, STRIDE2, OTHERTYPE>& v) const { return binaryMap([](const T& e1, const T2& e2){ return e1 - e2; }, v); }

        template <typename T2>
        inline auto operator*=(const T2& s) { mapWrite([&s](T& e){ e *= s; }); return *this; }
        template <typename T2>
        inline auto operator/=(const T2& s) { mapWrite([&s](T& e){ e /= s; }); return *this; }
        template <typename T2, size_t STRIDE2, template<typename, size_t, size_t> class OTHERTYPE>
        inline auto operator+=(const Vector<T2, N, STRIDE2, OTHERTYPE>& v) { binaryMapWrite([](T& e1, const T2& e2){ e1 += e2; }, v); return *this; }
        template <typename T2, size_t STRIDE2, template<typename, size_t, size_t> class OTHERTYPE>
        inline auto operator-=(const Vector<T2, N, STRIDE2, OTHERTYPE>& v) { binaryMapWrite([](T& e1, const T2& e2){ e1 -= e2; }, v); return *this; }
        template <typename T2, size_t STRIDE2, template<typename, size_t, size_t> class OTHERTYPE>
        inline auto operator=(const Vector<T2, N, STRIDE2, OTHERTYPE>& v) { binaryMapWrite([](T& e1, const T2& e2){ e1 = e2; }, v); return *this; }

        inline T         magnitudeSqr() const { return this->dot(*this);          }
        inline T            magnitude() const { return std::sqrt(magnitudeSqr()); }
        inline ReturnVec<T> direction() const { return *this / magnitude();       }

        template <typename T2, size_t STRIDE2, template<typename, size_t, size_t> class OTHERTYPE>
        inline auto dot(const Vector<T2, N, STRIDE2, OTHERTYPE>& v) const {
            return dotInternal(v, std::make_index_sequence<N>{});
        }

        template <typename T2, size_t STRIDE2, template<typename, size_t, size_t> class OTHERTYPE> requires (N == 3ul)
        inline auto cross(const Vector<T2, N, STRIDE2, OTHERTYPE>& v) const {
            return ReturnVec{ this->get(1)*v[2] - this->get(2)*v[1],
                            this->get(2)*v[0] - this->get(0)*v[2],
                            this->get(0)*v[1] - this->get(1)*v[0] };
        }
    };

    // Specialization allows for complete template type deduction and disallows 0-length array for value-types.
    template <typename T, std::same_as<T>... Ts>
    Vector(T&&, Ts&&...) -> Vector<T, 1uz + sizeof...(Ts), 1uz, VecVal>;

    // Right-side operator overloads
    template <typename T, typename T2, size_t N, size_t STRIDE, template<typename, size_t, size_t> class VECTYPE>
    inline auto operator*(const T& s, const Vector<T2, N, STRIDE, VECTYPE> &v) { return v.map([&s](const T& e) { return e * s; }); }
    template <typename T, typename T2, size_t N, size_t STRIDE, template<typename, size_t, size_t> class VECTYPE>
    inline auto operator/(const T& s, const Vector<T2, N, STRIDE, VECTYPE> &v) { return v.map([&s](const T& e) { return e / s; }); }
    template <typename T, size_t N, size_t STRIDE, template<typename, size_t, size_t> class VECTYPE>
    std::ostream& operator<<(std::ostream& os, const Vector<T, N, STRIDE, VECTYPE>& v) {
        for (size_t i = 0uz; i < N; ++i)
            os << (i ? " " : "") << v[i];
        return os;
    }


    ////////////
    // MATRIX //
    ////////////

    template <typename T, size_t M, size_t N>
    struct Matrix {
        T data[M * N];

        Matrix() = default;
        Matrix(const T (&d)[M][N]) { size_t i = 0ul; for (const auto& row : d) for (const auto& elem : row) data[i++] = elem; }

        //Vector<T&, M> row(size_t r) { return Vector<T&, M>(data + r); }

        T& operator[](size_t m, size_t n) { return data[N*m + n]; }
        const T& operator[](size_t m, size_t n) const { return data[N*m + n]; }
    };

    template <typename T, size_t M, size_t N>
    std::ostream& operator<<(std::ostream& os, const Matrix<T, M, N>& m) {
        for (size_t i = 0ul; i < M; ++i)
            for (size_t j = 0ul; j < N; ++j)
                os << (j ? " " : (i ? "\n" : "")) << m[i, j];
        return os;
    }
}
