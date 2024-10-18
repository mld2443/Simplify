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
        constexpr VecVal(Ts&&... values) : data{ values... } {}

        constexpr       T& get(size_t i)       { return data[i]; }
        constexpr const T& get(size_t i) const { return data[i]; }

        constexpr       T* beginImpl()       { return data;     }
        constexpr       T*   endImpl()       { return data + N; }
        constexpr const T* beginImpl() const { return data;     }
        constexpr const T*   endImpl() const { return data + N; }
    };

    // Vector reference-type base class
    template <typename T, size_t N, size_t STRIDE>
    class VecRef {
    public:
        template <typename PointerType>
        class Iterator {
        private:
            PointerType* pos;

        public:
            constexpr Iterator(PointerType *p) : pos(p) {}

            constexpr PointerType& operator*() const { return *pos; }
            constexpr Iterator& operator++() { pos += STRIDE; return *this; }
            constexpr bool operator==(const Iterator& o) const = default;
        };

    private:
        T* data;

    protected:
        constexpr VecRef(T* origin, size_t offset) : data(origin + offset) {}

        constexpr       T& get(size_t i)       { return data[i * STRIDE]; }
        constexpr const T& get(size_t i) const { return data[i * STRIDE]; }

        constexpr Iterator<T>       beginImpl()       { return { data              }; }
        constexpr Iterator<T>         endImpl()       { return { data + N * STRIDE }; }
        constexpr Iterator<const T> beginImpl() const { return { data              }; }
        constexpr Iterator<const T>   endImpl() const { return { data + N * STRIDE }; }
    };

    // Generic Vector class that can transparently perform operations and transformations on and between reference and
    // value type vectors of arbitrary, compile-time dimensions, even between mixed data types, such as float4 + int4.
    template <typename T, size_t N, size_t STRIDE = 1uz, template<typename, size_t, size_t> class VECTYPE = VecVal>
    class Vector : private VECTYPE<T, N, STRIDE> {
    private:
        // Dirty index_sequence function implementations
        template <size_t... IDX>
        static constexpr inline auto broadcastInternal(T value, std::index_sequence<IDX...>) { return Vector<T, N>{ (value + T(0uz * IDX))... }; }
        template <size_t... IDX>
        constexpr auto mapInternal(auto func, std::index_sequence<IDX...>) const {
            return Vector{ func(this->get(IDX))... };
        }
        template <typename T2, size_t STRIDE2, template<typename, size_t, size_t> class OTHERTYPE, size_t... IDX>
        constexpr auto binaryMapInternal(auto func, const Vector<T2, N, STRIDE2, OTHERTYPE>& v, std::index_sequence<IDX...>) const {
            return Vector{ func(this->get(IDX), v[IDX])... };
        }
        template <size_t... IDX>
        constexpr void mapWriteInternal(auto func, std::index_sequence<IDX...>) {
            (func(this->get(IDX)), ...);
        }
        template <typename T2, size_t STRIDE2, template<typename, size_t, size_t> class OTHERTYPE, size_t... IDX>
        constexpr void binaryMapWriteInternal(auto func, const Vector<T2, N, STRIDE2, OTHERTYPE>& v, std::index_sequence<IDX...>) {
            (func(this->get(IDX), v[IDX]), ...);
        }
        template <typename T2, size_t STRIDE2, template<typename, size_t, size_t> class OTHERTYPE, size_t... IDX>
        constexpr auto dotInternal(const Vector<T2, N, STRIDE2, OTHERTYPE>& v, std::index_sequence<IDX...>) const {
            return ((this->get(IDX) * v[IDX]) + ...);
        }

    public:
        // Value-type constructor
        template <std::same_as<T>... Ts>
        constexpr Vector(Ts&&... data) : VECTYPE<T, N, STRIDE>(static_cast<T&&>(data)...) {}
        // Reference-type constructor
        constexpr Vector(T* origin, size_t offset = 0uz) : VECTYPE<T, N, STRIDE>(origin, offset) {}

        static constexpr Vector broadcast(T value) { return broadcastInternal(value, std::make_index_sequence<N>{}); }

        // Accessors
        constexpr       T& operator[](size_t i)       { return this->get(i); }
        constexpr const T& operator[](size_t i) const { return this->get(i); }

        // Iterators for loops
        constexpr       auto begin()       { return this->beginImpl(); }
        constexpr       auto   end()       { return this->endImpl();   }
        constexpr const auto begin() const { return this->beginImpl(); }
        constexpr const auto   end() const { return this->endImpl();   }

        // Exposed mapping method
        constexpr auto map(auto func) const { return mapInternal(func, std::make_index_sequence<N>{}); }

        // Member operator overloads
        constexpr auto operator-() const { return map([](auto& d){ return -d; }); }
        template <typename T2>
        constexpr auto operator*(const T2& s) const { return map([&s](const T& e){ return e * s; }); }
        template <typename T2>
        constexpr auto operator/(const T2& s) const { return map([&s](const T& e){ return e / s; }); }
        template <typename T2, size_t STRIDE2, template<typename, size_t, size_t> class OTHERTYPE>
        constexpr auto operator+(const Vector<T2, N, STRIDE2, OTHERTYPE>& v) const { return binaryMapInternal([](const T& e1, const T2& e2){ return e1 + e2; }, v, std::make_index_sequence<N>{}); }
        template <typename T2, size_t STRIDE2, template<typename, size_t, size_t> class OTHERTYPE>
        constexpr auto operator-(const Vector<T2, N, STRIDE2, OTHERTYPE>& v) const { return binaryMapInternal([](const T& e1, const T2& e2){ return e1 - e2; }, v, std::make_index_sequence<N>{}); }

        // Mutating operators
        template <typename T2>
        constexpr auto operator*=(const T2& s) { mapWriteInternal([&s](T& e){ e *= s; }, std::make_index_sequence<N>{}); return *this; }
        template <typename T2>
        constexpr auto operator/=(const T2& s) { mapWriteInternal([&s](T& e){ e /= s; }, std::make_index_sequence<N>{}); return *this; }
        template <typename T2, size_t STRIDE2, template<typename, size_t, size_t> class OTHERTYPE>
        constexpr auto operator+=(const Vector<T2, N, STRIDE2, OTHERTYPE>& v) { binaryMapWriteInternal([](T& e1, const T2& e2){ e1 += e2; }, v, std::make_index_sequence<N>{}); return *this; }
        template <typename T2, size_t STRIDE2, template<typename, size_t, size_t> class OTHERTYPE>
        constexpr auto operator-=(const Vector<T2, N, STRIDE2, OTHERTYPE>& v) { binaryMapWriteInternal([](T& e1, const T2& e2){ e1 -= e2; }, v, std::make_index_sequence<N>{}); return *this; }
        template <typename T2, size_t STRIDE2, template<typename, size_t, size_t> class OTHERTYPE>
        constexpr auto  operator=(const Vector<T2, N, STRIDE2, OTHERTYPE>& v) { binaryMapWriteInternal([](T& e1, const T2& e2){ e1 = e2;  }, v, std::make_index_sequence<N>{}); return *this; }

        // Geometric methods
        constexpr T   magnitudeSqr() const { return this->dot(*this);          }
        constexpr T      magnitude() const { return std::sqrt(magnitudeSqr()); }
        constexpr Vector direction() const { return *this / magnitude();       }
        template <typename T2, size_t STRIDE2, template<typename, size_t, size_t> class OTHERTYPE>
        constexpr auto dot(const Vector<T2, N, STRIDE2, OTHERTYPE>& v) const {
            return dotInternal(v, std::make_index_sequence<N>{});
        }

        // Cross product for 3-dimensional vectors
        template <typename T2, size_t STRIDE2, template<typename, size_t, size_t> class OTHERTYPE> requires (N == 3ul)
        constexpr auto cross(const Vector<T2, N, STRIDE2, OTHERTYPE>& v) const {
            return Vector{ this->get(1)*v[2] - this->get(2)*v[1], this->get(2)*v[0] - this->get(0)*v[2], this->get(0)*v[1] - this->get(1)*v[0] };
        }
    };

    // Specialization allows for automatic template deduction and disallows 0-length array for value-types.
    template <typename T, std::same_as<T>... Ts>
    Vector(T&&, Ts&&...) -> Vector<T, 1uz + sizeof...(Ts), 1uz, VecVal>;

    // Right-side operator overloads
    template <typename T, typename T2, size_t N, size_t STRIDE, template<typename, size_t, size_t> class VECTYPE>
    constexpr auto operator*(const T& s, const Vector<T2, N, STRIDE, VECTYPE> &v) { return v.map([&s](const T& e) { return e * s; }); }
    template <typename T, typename T2, size_t N, size_t STRIDE, template<typename, size_t, size_t> class VECTYPE>
    constexpr auto operator/(const T& s, const Vector<T2, N, STRIDE, VECTYPE> &v) { return v.map([&s](const T& e) { return e / s; }); }
    template <typename T, size_t N, size_t STRIDE, template<typename, size_t, size_t> class VECTYPE>
    constexpr std::ostream& operator<<(std::ostream& os, const Vector<T, N, STRIDE, VECTYPE>& v) {
        for (size_t i = 0uz; i < N; ++i)
            os << (i ? " " : "") << v[i];
        return os;
    }


    ////////////
    // MATRIX //
    ////////////

    // Generic Matrix class
    template <typename T, size_t M, size_t N>
    class Matrix {
    private:
        T data[M * N];

        // Annoying index_sequence function implementations
        template <size_t... IDX>
        constexpr static Matrix identityInternal(std::index_sequence<IDX...>) {
            return { (IDX % (M + 1uz) ? T(0) : T(1))... };
        }
        template <typename T2, size_t O, size_t... IDX>
        constexpr auto multiplyInternal(const Matrix<T2, N, O>& m, std::index_sequence<IDX...>) const {
            return Matrix<decltype(T()*T2()), M, O>{ getRow(IDX / O).dot(m.getCol(IDX % O))... };
        }

    public:
        // Empty initializer, not sure if it's even useful.
        constexpr Matrix() = default;

        // Special index_sequence initializer FIXME can't be private? is it because of the inlining?
        template <std::same_as<T>... Ts> requires(sizeof...(Ts) == 0uz || sizeof...(Ts) == M*N)
        constexpr Matrix(Ts&&... values) : data{ values... } {}

        // Vector initialization -- problematic given that matrices don't have the same owning/reference dichotomy
        //template <size_t STRIDE, template<typename, size_t, size_t> class VECTYPE>
        //constexpr Matrix(const Vector<T, M, STRIDE, VECTYPE>& v);

        template <size_t... IDX> requires (sizeof...(IDX) == M*N)
        constexpr Matrix(T (&&data)[M][N], std::index_sequence<IDX...>) : data{ data[IDX/N][IDX%N]... } {}

        // Value-initialization constructor
        constexpr Matrix(T (&&data)[M][N]) : Matrix(std::move(data), std::make_index_sequence<M*N>{}) {}

        // Identity matrix for some reason
        constexpr static Matrix I() requires(M == N) { return identityInternal(std::make_index_sequence<M*N>{}); }

        // Accessors
        constexpr       T& operator[](size_t m, size_t n)       { return data[n + m*N]; }
        constexpr const T& operator[](size_t m, size_t n) const { return data[n + m*N]; }
        constexpr Vector<      T, N, 1uz, VecRef> getRow(size_t row)       { return { data + row * N }; }
        constexpr Vector<const T, N, 1uz, VecRef> getRow(size_t row) const { return { data + row * N }; }
        constexpr Vector<      T, M,   N, VecRef> getCol(size_t col)       { return { data + col }; }
        constexpr Vector<const T, M,   N, VecRef> getCol(size_t col) const { return { data + col }; }
        constexpr Vector<      T, std::min(M, N), N+1uz, VecRef> getDiag()       { return { data }; }
        constexpr Vector<const T, std::min(M, N), N+1uz, VecRef> getDiag() const { return { data }; }

        // Member operators
        template<typename T2, size_t O>
        constexpr auto operator*(const Matrix<T2, N, O>& m) const { return multiplyInternal(m, std::make_index_sequence<M*O>{}); }
    };

    // Right-side operator overloads
    template <typename T, size_t M, size_t N>
    constexpr std::ostream& operator<<(std::ostream& os, const Matrix<T, M, N>& m) {
        for (size_t i = 0uz; i < M; ++i)
            for (size_t j = 0uz; j < N; ++j)
                os << (j ? " " : (i ? "\n" : "")) << m[i, j];
        return os;
    }
}
