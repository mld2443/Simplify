#pragma once

#include <concepts> // same_as
#include <cmath>    // sqrt
#include <iostream> // ostream
#include <utility>  // index_sequence, make_index_sequence


namespace linalg {
    // TODO:
    // [x] Convert to conventional CRTP
    // [x] Signed offsets and strides
    // [x] Extract more common functionality of the various base classes into new superbase?
    // [x] Summation of elements, eg like the dot product
    // [ ] Reference type matrices
    // [ ] Matrix from vectors?
    // [ ] vector matrix multiply?
    // [ ] Matrix transpose
    // [ ] Vector transpose -> Matrix? Makes cartesian products simple

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

            constexpr decltype(auto) operator*(this auto& self) { return *self.pos; }
            constexpr decltype(auto) operator++(this auto& self) { self.pos += S; return self; }
            constexpr bool operator==(const Iterator& o) const = default;
        };

    public:
        // Iterators for loops
        constexpr auto begin(this auto& self) -> COPYCONSTFORTYPE(decltype(self), Iterator) { return { self.data }; }
        constexpr auto   end(this auto& self) -> COPYCONSTFORTYPE(decltype(self), Iterator) { return { self.data + static_cast<ssize_t>(M*N) * S }; }

        constexpr decltype(auto) get(this auto& self, size_t i)           { return self.data[static_cast<ssize_t>(i)       * S]; }
        constexpr decltype(auto) get(this auto& self, size_t m, size_t n) { return self.data[static_cast<ssize_t>(n + m*N) * S]; }
    };

    template <typename T, size_t M, size_t N>
    class TensorRoot {
    public:
        constexpr auto map(this const auto& self, auto func) { return self.mapInternal(func, MAKEINDICES(M*N)); }
        constexpr auto fold(this const auto& self, auto func, T starting) { return self.foldInternal(func, starting, MAKEINDICES(M*N)); }

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
    };


    ///////////////////
    // STORAGE TYPES //
    ///////////////////

    // Value type that owns its own data
    template <typename T, size_t M, size_t N, ssize_t>
    class ValueType : public StorageRoot<T, M, N, 1z> {
        friend StorageRoot<T, M, N, 1z>;
    protected:
        T data[M*N];

        template <std::same_as<T>... Ts> requires(sizeof...(Ts) == 0uz || sizeof...(Ts) == M*N)
        constexpr ValueType(Ts&&... payload) : data{ payload... } {}
    };

    // Reference-type that points to data (no ref counting!)
    //   These should be treated as transient, kinda like an r-value
    template <typename T, size_t M, size_t N, ssize_t S>
    class ReferenceType : public StorageRoot<T, M, N, S> {
        friend StorageRoot<T, M, N, S>;
    protected:
        T* data;

        constexpr ReferenceType(T* origin, ssize_t offset) : data(origin + offset) {}
    };


    //////////////////
    // TENSOR TYPES //
    //////////////////

    // VECTOR
    template <typename T, size_t N, ssize_t S, STORAGECLASS STORAGETYPE>
    class VectorBase : public TensorRoot<T, N, 1uz>, public STORAGETYPE<T, N, 1uz, S> {
    public:
        using BaseType = T;
        template <typename TYPE>
        using ReturnType = VectorBase<TYPE, N, 1uz, ValueType>;

    private:
        template <typename T2, ssize_t S2, STORAGECLASS OTHERSTORAGE, size_t... IDX>
        constexpr auto dotInternal(this const auto& self, const VectorBase<T2, N, S2, OTHERSTORAGE>& v, std::index_sequence<IDX...>) {
            return ((self[IDX] * v[IDX]) + ...);
        }

    public:
        // Constructors
        template <size_t... IDX>
        constexpr VectorBase(T value, std::index_sequence<IDX...>) : STORAGETYPE<T, N, 1uz, 1z>(value + T(0uz & IDX)...) {}
        template <std::same_as<T>... Ts>
        constexpr VectorBase(Ts&&... payload) : STORAGETYPE<T, N, 1uz, 1z>(std::forward<T>(payload)...) {}
        constexpr VectorBase(T* origin, ssize_t offset) : STORAGETYPE<T, N, 1uz, S>(origin, offset) {}

        // Accessors
        constexpr decltype(auto) operator[](this auto& self, size_t i) { return self.get(i); }

        // Member operator overloads
        constexpr auto operator-() const { return this->map([](auto& e){ return -e; }); }
        constexpr auto operator*(const auto& s) const { return this->map([&s](const T& e){ return e * s; }); }
        constexpr auto operator/(const auto& s) const { return this->map([&s](const T& e){ return e / s; }); }
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
    class MatrixBase : public TensorRoot<T, M, N>, public STORAGETYPE<T, M, N, S> {
    public:
        using BaseType = T;
        template <typename TYPE>
        using ReturnType = MatrixBase<TYPE, M, N, 1z, ValueType>;

    private:
        template <size_t... IDX>
        constexpr static ReturnType<T> identityInternal(std::index_sequence<IDX...>) {
            return { (IDX % (M + 1uz) ? T(0) : T(1))... };
        }
        template <typename T2, size_t O, ssize_t S2, STORAGECLASS OTHERSTORAGE, size_t... IDX>
        constexpr auto matrixMultiply(const MatrixBase<T2, N, O, S2, OTHERSTORAGE>& m, std::index_sequence<IDX...>) const {
            return MatrixBase<decltype(T()*T2()), M, O, 1z, ValueType>{ getRow(IDX / O).dot(m.getCol(IDX % O))... };
        }

    public:
        // Constructors
        template <std::same_as<T>... Ts>
        constexpr MatrixBase(Ts&&... payload) : STORAGETYPE<T, M, N, 1z>(std::forward<T>(payload)...) {}

        // Identity matrix for some reason
        constexpr static auto I() requires(M == N) { return identityInternal(MAKEINDICES(M*N)); }

        // Accessors
        constexpr decltype(auto) operator[](this auto& self, size_t m, size_t n) { return self.get(m, n); }
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
    public:
        template <std::same_as<T>... Ts>
        constexpr Vector(Ts&&... payload) : VectorBase<T, N, 1z, ValueType>(std::forward<T>(payload)...) {}
        constexpr Vector(T value) : VectorBase<T, N, 1z, ValueType>(value, MAKEINDICES(N)) {}
    };
    // Deduction guide to get the value of N
    template <typename T, size_t N>
    Vector(T) -> Vector<T, N>;
    template <typename T, std::same_as<T>... Ts>
    Vector(T&&, Ts&&...) -> Vector<T, 1uz + sizeof...(Ts)>;

    // Vector reference-type struct
    template <typename T, size_t N, ssize_t S = 1z>
    class VectorRef : public VectorBase<T, N, S, ReferenceType> {
    public:
        constexpr VectorRef(T* origin, ssize_t offset = 0z) : VectorBase<T, N, S, ReferenceType>(origin, offset) {}
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

#if 0
namespace legacy {
    ////////////
    // VECTOR //
    ////////////

    // Vector value-type base class
    template <typename T, size_t N, size_t>
    class [[deprecated]] VecVal {
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
    class [[deprecated]] VecRef {
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
    class [[deprecated]] Vector : private VECTYPE<T, N, STRIDE> {
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
    class [[deprecated]] Matrix {
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
#endif
