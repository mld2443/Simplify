#pragma once

#include <cmath>       // sqrt
#include <concepts>    // same_as
#include <iostream>    // ostream
#include <type_traits> // conditional_t, is_const_v
#include <utility>     // forward, index_sequence, make_index_sequence


namespace linalg {
    // TODO:
    // [ ] SETTLE ON PARADIGM: applied types need to exist? copy-swap?
    // [ ] Reference type matrices?
    // [ ] Matrix from vectors?
    // [ ] vector matrix multiply?
    // [ ] Implement missing Matrix operators (*, /, +=, -=, &c.)
    // [ ] Matrix transpose
    // [ ] Vector transpose -> Vector?
    // [ ] Vector transpose -> Matrix? Makes cartesian products simple
    // [ ] Vector reverse? -> could simply return -1 stride and with offset?


    // Helper macros to reduce clutter, undefined at end of namespace
    #define COPYCONSTFORTYPE(T1, T2) std::conditional_t<std::is_const_v<T1>, const T2, T2>
    #define STORAGECLASS template <typename, size_t, size_t, ssize_t> class
    #define MAKEINDICES(SIZE) std::make_index_sequence<SIZE>{}

    ////////////////
    // ROOT TYPES //
    ////////////////

    template <typename T, size_t M, size_t N, ssize_t S>
    class StorageRoot {
    public:
        class Iterator {
        private:
            mutable T* pos;

        public:
            constexpr Iterator(T* p) : pos(p) {}

            constexpr decltype(auto)  operator*(this auto& self) { return *self.pos; }
            constexpr decltype(auto) operator++(this auto& self) { self.pos += S; return self; }
            constexpr bool operator==(const Iterator& o) const = default;
        };

    public:
        // Iterators for for-each loops
        constexpr auto begin(this auto& self) -> COPYCONSTFORTYPE(decltype(self), Iterator) { return { self.data }; }
        constexpr auto   end(this auto& self) -> COPYCONSTFORTYPE(decltype(self), Iterator) { return { self.data + static_cast<ssize_t>(M*N) * S }; }

    protected:
        // Accessor
        constexpr decltype(auto) get(this auto& self, size_t i) { return self.data[static_cast<ssize_t>(i) * S]; }
    };

    template <typename T, size_t M, size_t N>
    class TensorRoot {
    protected:
        template <size_t... IDX>
        constexpr auto foldInternal(this const auto& self, auto& func, T starting, std::index_sequence<IDX...>) {
            return ((starting = func(starting, self.get(IDX))), ...);
        }
        template <typename MYTYPE, size_t... IDX>
        constexpr auto mapInternal(this const MYTYPE& self, auto func, std::index_sequence<IDX...>) {
            return typename MYTYPE::template ReturnType<decltype(func(T()))>{ func(self.get(IDX))... };
        }
        template <typename MYTYPE, typename OTHER, size_t... IDX>
        constexpr auto binaryMapInternal(this const MYTYPE& self, auto func, const OTHER& v, std::index_sequence<IDX...>) {
            return typename MYTYPE::template ReturnType<decltype(func(T(), typename OTHER::BaseType()))>{ func(self.get(IDX), v.get(IDX))... };
        }
        template <size_t... IDX>
        inline void mapWriteInternal(this auto& self, auto func, std::index_sequence<IDX...>) {
            (func(self.get(IDX)), ...);
        }
        template <size_t... IDX>
        inline void binaryMapWriteInternal(this auto& self, auto func, const auto& v, std::index_sequence<IDX...>) {
            (func(self.get(IDX), v.get(IDX)), ...);
        }

    public:
        constexpr auto fold(this const auto& self, auto func, T starting) { return self.foldInternal(func, starting, MAKEINDICES(M*N)); }
        constexpr auto map(this const auto& self, auto func) { return self.mapInternal(func, MAKEINDICES(M*N)); }

        // Member operator overloads
        constexpr auto operator-(this const auto& self)                { return self.map([  ](auto&    e){ return    -e; }); }
        constexpr auto operator*(this const auto& self, const auto& s) { return self.map([&s](const T& e){ return e * s; }); }
        constexpr auto operator/(this const auto& self, const auto& s) { return self.map([&s](const T& e){ return e / s; }); }
    };


    ///////////////////
    // STORAGE TYPES //
    ///////////////////

    // Value type that owns its own data
    template <typename T, size_t M, size_t N, ssize_t>
    class ValueType : public StorageRoot<T, M, N, 1z> {
        friend StorageRoot<T, M, N, 1z>;
    public:
        template <std::same_as<T>... Ts> requires(sizeof...(Ts) == 0uz || sizeof...(Ts) == M*N)
        constexpr ValueType(Ts&&... payload) : data{ payload... } {}

    protected:
        T data[M*N];
    };

    // Reference-type that points to data (no ref counting!)
    //   These should be treated as transient, kinda like an r-value
    template <typename T, size_t M, size_t N, ssize_t S>
    class ReferenceType : public StorageRoot<T, M, N, S> {
        friend StorageRoot<T, M, N, S>;
    public:
        constexpr ReferenceType(T* origin, ssize_t offset) : data(origin + offset) {}

    protected:
        T* data;
    };


    //////////////////
    // TENSOR TYPES //
    //////////////////

    // VECTOR
    template <typename T, size_t N, ssize_t S, STORAGECLASS STORAGETYPE>
    class VectorBase : public STORAGETYPE<T, N, 1uz, S>, public TensorRoot<T, N, 1uz> {
    private:
        template <typename, size_t, size_t>
        friend class TensorRoot;

        using BaseType = T;
        template <typename TYPE>
        using ReturnType = VectorBase<TYPE, N, 1uz, ValueType>;

        // Implementation for the "broadcast" constructor, curious hack to coax the expansion but discard the values
        template <size_t... IDX>
        constexpr VectorBase(const T& value, std::index_sequence<IDX...>) : STORAGETYPE<T, N, 1uz, 1z>(value + T(0uz & IDX)...) {}

        template <typename T2, ssize_t S2, STORAGECLASS OTHERSTORAGE, size_t... IDX>
        constexpr auto dotInternal(this const auto& self, const VectorBase<T2, N, S2, OTHERSTORAGE>& v, std::index_sequence<IDX...>) {
            return ((self[IDX] * v[IDX]) + ...);
        }

    public:
        // Constructors
        using STORAGETYPE<T, N, 1uz, S>::STORAGETYPE;
        VectorBase(const T& value) : VectorBase(value, MAKEINDICES(N)) {}

        // Accessor
        constexpr decltype(auto) operator[](this auto& self, size_t i) { return self.get(i); }

        // Member operator overloads
        template <typename T2, ssize_t S2, STORAGECLASS OTHERSTORAGE>
        constexpr auto operator+(const VectorBase<T2, N, S2, OTHERSTORAGE>& v) const { return this->binaryMapInternal([](const T& e1, const T2& e2){ return e1 + e2; }, v, MAKEINDICES(N)); }
        template <typename T2, ssize_t S2, STORAGECLASS OTHERSTORAGE>
        constexpr auto operator-(const VectorBase<T2, N, S2, OTHERSTORAGE>& v) const { return this->binaryMapInternal([](const T& e1, const T2& e2){ return e1 - e2; }, v, MAKEINDICES(N)); }

        // Mutating operators
        inline auto& operator*=(const auto& s) { this->mapWriteInternal([&s](T& e){ e *= s; }, MAKEINDICES(N)); return *this; }
        inline auto& operator/=(const auto& s) { this->mapWriteInternal([&s](T& e){ e /= s; }, MAKEINDICES(N)); return *this; }
        template <typename T2, ssize_t S2, STORAGECLASS OTHERSTORAGE>
        inline auto& operator+=(const VectorBase<T2, N, S2, OTHERSTORAGE>& v) { this->binaryMapWriteInternal([](T& e1, const T2& e2){ e1 += e2; }, v, MAKEINDICES(N)); return *this; }
        template <typename T2, ssize_t S2, STORAGECLASS OTHERSTORAGE>
        inline auto& operator-=(const VectorBase<T2, N, S2, OTHERSTORAGE>& v) { this->binaryMapWriteInternal([](T& e1, const T2& e2){ e1 -= e2; }, v, MAKEINDICES(N)); return *this; }
        template <typename T2, ssize_t S2, STORAGECLASS OTHERSTORAGE>
        inline auto&  operator=(const VectorBase<T2, N, S2, OTHERSTORAGE>& v) { this->binaryMapWriteInternal([](T& e1, const T2& e2){ e1  = e2; }, v, MAKEINDICES(N)); return *this; }

        // Geometric methods
        template <typename T2, ssize_t S2, STORAGECLASS OTHERSTORAGE>
        constexpr T dot(const VectorBase<T2, N, S2, OTHERSTORAGE>& v) const { return dotInternal(v, MAKEINDICES(N)); }
        constexpr T magnitudeSqr() const { return dot(*this);                }
        constexpr T    magnitude() const { return std::sqrt(magnitudeSqr()); }
        constexpr auto direction() const { return *this / magnitude();       }

        // Cross product for 3-dimensional vectors
        template <typename T2, ssize_t S2, STORAGECLASS OTHERSTORAGE>
        constexpr ReturnType<decltype(T()*T2())> cross(this const VectorBase<T, 3ul, S, STORAGETYPE>& self, const VectorBase<T2, N, S2, OTHERSTORAGE>& v) {
            return { self[1uz]*v[2uz] - self[2uz]*v[1uz],
                     self[2uz]*v[0uz] - self[0uz]*v[2uz],
                     self[0uz]*v[1uz] - self[1uz]*v[0uz] };
        }
    };

    // Right-side operator overloads
    template <typename T, size_t N, typename T2, ssize_t S2, STORAGECLASS OTHERSTORAGE>
    constexpr auto operator*(const T& s, const VectorBase<T2, N, S2, OTHERSTORAGE> &v) { return v.map([&s](const T& e) { return e * s; }); }
    template <typename T, size_t N, typename T2, ssize_t S2, STORAGECLASS OTHERSTORAGE>
    constexpr auto operator/(const T& s, const VectorBase<T2, N, S2, OTHERSTORAGE> &v) { return v.map([&s](const T& e) { return e / s; }); }
    template <typename T, size_t N, ssize_t S, STORAGECLASS STORAGETYPE>
    constexpr std::ostream& operator<<(std::ostream& os, const VectorBase<T, N, S, STORAGETYPE>& v) {
        for (size_t i = 0uz; i < N; ++i)
            os << (i ? " " : "") << v[i];
        return os;
    }


    // MATRIX
    template <typename T, size_t M, size_t N, ssize_t S, STORAGECLASS STORAGETYPE>
    class MatrixBase : public STORAGETYPE<T, M, N, S>, public TensorRoot<T, M, N> {
    private:
        template <typename, size_t, size_t>
        friend class TensorRoot;

        using BaseType = T;
        template <typename TYPE>
        using ReturnType = MatrixBase<TYPE, M, N, 1z, ValueType>;

        template <size_t... IDX>
        constexpr static ReturnType<T> matrixIdentity(std::index_sequence<IDX...>) {
            return { (IDX % (M + 1uz) ? T(0) : T(1))... };
        }
        template <typename T2, size_t O, ssize_t S2, STORAGECLASS OTHERSTORAGE, size_t... IDX>
        constexpr auto matrixMultiply(const MatrixBase<T2, N, O, S2, OTHERSTORAGE>& m, std::index_sequence<IDX...>) const {
            return MatrixBase<decltype(T()*T2()), M, O, 1z, ValueType>{ getRow(IDX / O).dot(m.getCol(IDX % O))... };
        }

    public:
        // Constructors
        using STORAGETYPE<T, M, N, S>::STORAGETYPE;

        // Identity matrix for some reason
        constexpr static auto I() requires(M == N) { return matrixIdentity(MAKEINDICES(M*N)); }

        // Accessors
        constexpr decltype(auto) operator[](this auto& self, size_t m, size_t n) { return self.get(n + m*N); }
        template <class MYTYPE> constexpr auto getRow(this MYTYPE& self, size_t row) { return VectorBase<COPYCONSTFORTYPE(MYTYPE, T), N,     S, ReferenceType>{ self.data, static_cast<ssize_t>(row * N * S) }; }
        template <class MYTYPE> constexpr auto getCol(this MYTYPE& self, size_t col) { return VectorBase<COPYCONSTFORTYPE(MYTYPE, T), M, N * S, ReferenceType>{ self.data, static_cast<ssize_t>(col     * S) }; }
        template <class MYTYPE> constexpr auto getDiagonal(this MYTYPE& self) { return VectorBase<COPYCONSTFORTYPE(MYTYPE, T), std::min(M, N), (N + 1z) * S, ReferenceType>{ self.data, 0z }; }

        // Member operators
        template<typename T2, size_t O, ssize_t S2, STORAGECLASS OTHERSTORAGE>
        constexpr auto operator*(const MatrixBase<T2, N, O, S2, OTHERSTORAGE>& m) const { return matrixMultiply(m, MAKEINDICES(M*O)); }
        template<typename T2, ssize_t S2, STORAGECLASS OTHERSTORAGE>
        constexpr auto operator+(const MatrixBase<T2, M, N, S2, OTHERSTORAGE>& m) const { return this->binaryMapInternal([](const T& e1, const T2& e2){ return e1 + e2; }, m, MAKEINDICES(M*N)); }
        template<typename T2, ssize_t S2, STORAGECLASS OTHERSTORAGE>
        constexpr auto operator-(const MatrixBase<T2, M, N, S2, OTHERSTORAGE>& m) const { return this->binaryMapInternal([](const T& e1, const T2& e2){ return e1 - e2; }, m, MAKEINDICES(M*N)); }
    };

    // Right-side operator overloads
    template <typename T, size_t M, size_t N, ssize_t S, STORAGECLASS STORAGETYPE>
    constexpr std::ostream& operator<<(std::ostream& os, const MatrixBase<T, M, N, S, STORAGETYPE>& m) {
        for (size_t i = 0uz; i < M; ++i)
            for (size_t j = 0uz; j < N; ++j)
                os << (j ? " " : (i ? "\n" : "")) << m[i, j];
        return os;
    }


    ///////////////////
    // APPLIED TYPES //
    ///////////////////
    // These 4 are supposed to be the things you actually use; everything else above is inherited

    // Vector value-type struct
    template <typename T, size_t N>
    class Vector : public VectorBase<T, N, 1z, ValueType> {
        using VectorBase<T, N, 1z, ValueType>::VectorBase;
    };
    // Template deduction guide to automatically deduce N from initializer lists
    template <typename T, std::same_as<T>... Ts>
    Vector(T&&, Ts&&...) -> Vector<T, 1uz + sizeof...(Ts)>;

    // Vector reference-type struct
    template <typename T, size_t N, ssize_t S = 1z>
    class VectorRef : public VectorBase<T, N, S, ReferenceType> {
        using VectorBase<T, N, S, ReferenceType>::VectorBase;
    };

    // Matrix value-type struct
    template <typename T, size_t M, size_t N>
    class Matrix : public MatrixBase<T, M, N, 1z, ValueType> {
    private:
        template <size_t... IDX>
        constexpr Matrix(T (&&payload)[M][N], std::index_sequence<IDX...>) : MatrixBase<T, M, N, 1z, ValueType>(std::forward<T>(payload[IDX/N][IDX%N])...) {}

    public:
        // Value-initialization constructor
        constexpr Matrix(T (&&payload)[M][N]) : Matrix(std::forward<T[M][N]>(payload), MAKEINDICES(M*N)) {}
    };

    // Matrix reference-type struct
    // TODO

    #undef COPYCONSTFORTYPE
    #undef STORAGECLASS
    #undef MAKEINDICES
}
