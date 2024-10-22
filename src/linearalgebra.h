#pragma once

#include <concepts> // same_as
#include <cmath>    // sqrt
#include <iostream> // ostream
#include <utility>  // index_sequence, make_index_sequence


namespace linalg {
    // TODO:
    // [x] Convert to conventional CRTP
    // [x] Signed offsets and strides
    // [ ] Reference type matrices
    // [ ] Matrix from vectors?
    // [ ] vector matrix multiply?
    // [ ] Matrix transpose
    // [ ] Vector transpose -> Matrix? Makes cartesian products simple
    // [ ] Extract more common functionality of the various base classes into new superbase?
    // [ ] Summation of elements, eg like the dot product

    /////////////////////
    // STORAGE CLASSES //
    /////////////////////

    // Value type that owns its own data
    template <typename T, size_t M, size_t N, template <typename, size_t, size_t, class> class BASE>
    class ValueType : public BASE<T, M, N, ValueType<T, M, N, BASE>> {
        friend class BASE<T, M, N, ValueType<T, M, N, BASE>>;
    private:
        T data[M*N];

    public:
        template <std::same_as<T>... Ts> requires(sizeof...(Ts) == 0uz || sizeof...(Ts) == M*N)
        constexpr ValueType(Ts&&... payload) : data{ payload... } {}

        // Iterators for loops
        constexpr       T* begin()       { return data;       }
        constexpr       T*   end()       { return data + M*N; }
        constexpr const T* begin() const { return data;       }
        constexpr const T*   end() const { return data + M*N; }

    protected:
        constexpr       T& get(size_t i)       { return data[i]; }
        constexpr const T& get(size_t i) const { return data[i]; }
        constexpr       T& get(size_t m, size_t n)       { return data[n + m*N]; }
        constexpr const T& get(size_t m, size_t n) const { return data[n + m*N]; }
    };

    // Reference-type that points to data (no ref counting!)
    //   These should be treated as transient, kinda like an r-value
    template <typename T, size_t M, size_t N, ssize_t S, template <typename, size_t, size_t, class> class BASE>
    class ReferenceType : public BASE<T, M, N, ReferenceType<T, M, N, S, BASE>> {
        friend class BASE<T, M, N, ReferenceType<T, M, N, S, BASE>>;
    private:
        T* data;

    public:
        class Iterator {
        private:
            mutable T* pos;

        public:
            constexpr Iterator(T* p) : pos(p) {}

            constexpr       T& operator*()       { return *pos; }
            constexpr const T& operator*() const { return *pos; }
            constexpr const Iterator& operator++() const { pos += S; return *this; }
            constexpr bool operator==(const Iterator& o) const = default;
        };

    public:
        constexpr ReferenceType(T* origin, ssize_t offset) : data(origin + offset) {}

        // Iterators for loops
        constexpr       Iterator begin()       { return { data };                                 }
        constexpr       Iterator   end()       { return { data + static_cast<ssize_t>(M*N) * S }; }
        constexpr const Iterator begin() const { return { data };                                 }
        constexpr const Iterator   end() const { return { data + static_cast<ssize_t>(M*N) * S }; }

    protected:
        constexpr       T& get(size_t i)       { return data[static_cast<ssize_t>(i) * S]; }
        constexpr const T& get(size_t i) const { return data[static_cast<ssize_t>(i) * S]; }
        constexpr       T& get(size_t m, size_t n)       { return data[static_cast<ssize_t>(n + m*N) * S]; }
        constexpr const T& get(size_t m, size_t n) const { return data[static_cast<ssize_t>(n + m*N) * S]; }
    };


    //////////////////
    // BASE CLASSES //
    //////////////////

    // COMMON
    template <typename T, size_t M, size_t N, class DERIVED>
    class CommonBase {
    protected:
        // CRTP 'this' wrapper; obsolete with C++23 'explicit this'
        constexpr       DERIVED* me()       { return static_cast<      DERIVED*>(this); }
        constexpr const DERIVED* me() const { return static_cast<const DERIVED*>(this); }

        // Dirty index_sequence function implementations
        template <size_t... IDX>
        static constexpr inline auto broadcastInternal(T value, std::index_sequence<IDX...>) { return { (value + T(0uz * IDX))... }; }
        template <size_t... IDX>
        constexpr auto sumInternal(std::index_sequence<IDX...>) const {
            return ((this->me()->get(IDX)) + ...);
        }
        // template <size_t... IDX>
        // constexpr auto mapInternal(auto func, std::index_sequence<IDX...>) const {
        //     return { func(this->me()->get(IDX))... };
        // }
        // template <typename T2, class OTHERTYPE, size_t... IDX>
        // constexpr auto binaryMapInternal(auto func, const VectorBase<T2, N, 1uz, OTHERTYPE>& v, std::index_sequence<IDX...>) const {
        //     return { func(this->me()->get(IDX), v[IDX])... };
        // }
    };


    // VECTOR
    template <typename T, size_t N, size_t, class DERIVED>
    class VectorBase : public CommonBase<T, N, 1uz, DERIVED> {
    private:
        template <typename TYPE>
        using ReturnType = ValueType<TYPE, N, 1uz, VectorBase>;
        template <size_t SIZE>
        using makeInds = std::make_index_sequence<SIZE>;

        // Dirty index_sequence function implementations
        // template <size_t... IDX>
        // static constexpr inline ReturnType<T> broadcastInternal(T value, std::index_sequence<IDX...>) { return { (value + T(0uz * IDX))... }; }
        template <size_t... IDX>
        constexpr auto mapInternal(auto func, std::index_sequence<IDX...>) const {
            return ReturnType<decltype(func(T()))>{ func(this->me()->get(IDX))... };
        }
        template <typename T2, class OTHERTYPE, size_t... IDX>
        constexpr auto binaryMapInternal(auto func, const VectorBase<T2, N, 1uz, OTHERTYPE>& v, std::index_sequence<IDX...>) const {
            return ReturnType<decltype(func(T(), T2()))>{ func(this->me()->get(IDX), v[IDX])... };
        }
        template <size_t... IDX>
        constexpr void mapWriteInternal(auto func, std::index_sequence<IDX...>) {
            (func(this->me()->get(IDX)), ...);
        }
        template <typename T2, class OTHERTYPE, size_t... IDX>
        constexpr void binaryMapWriteInternal(auto func, const VectorBase<T2, N, 1uz, OTHERTYPE>& v, std::index_sequence<IDX...>) {
            (func(this->me()->get(IDX), v[IDX]), ...);
        }
        template <typename T2, class OTHERTYPE, size_t... IDX>
        constexpr auto dotInternal(const VectorBase<T2, N, 1uz, OTHERTYPE>& v, std::index_sequence<IDX...>) const {
            return ((this->me()->get(IDX) * v[IDX]) + ...);
        }

    public:
        // Accessors
        constexpr       T& operator[](size_t i)       { return this->me()->get(i); }
        constexpr const T& operator[](size_t i) const { return this->me()->get(i); }

        // Functional style mapping method
        constexpr auto map(auto func) const { return mapInternal(func, makeInds<N>{}); }

        // Member operator overloads
        constexpr auto operator-() const { return map([](auto& d){ return -d; }); }
        template <typename T2>
        constexpr auto operator*(const T2& s) const { return map([&s](const T& e){ return e * s; }); }
        template <typename T2>
        constexpr auto operator/(const T2& s) const { return map([&s](const T& e){ return e / s; }); }
        template <typename T2, class OTHERTYPE>
        constexpr auto operator+(const VectorBase<T2, N, 1uz, OTHERTYPE>& v) const { return binaryMapInternal([](const T& e1, const T2& e2){ return e1 + e2; }, v, makeInds<N>{}); }
        template <typename T2, class OTHERTYPE>
        constexpr auto operator-(const VectorBase<T2, N, 1uz, OTHERTYPE>& v) const { return binaryMapInternal([](const T& e1, const T2& e2){ return e1 - e2; }, v, makeInds<N>{}); }

        // Mutating operators
        template <typename T2>
        inline auto& operator*=(const T2& s) { mapWriteInternal([&s](T& e){ e *= s; }, makeInds<N>{}); return *this->me(); }
        template <typename T2>
        inline auto& operator/=(const T2& s) { mapWriteInternal([&s](T& e){ e /= s; }, makeInds<N>{}); return *this->me(); }
        template <typename T2, class OTHERTYPE>
        inline auto& operator+=(const VectorBase<T2, N, 1uz, OTHERTYPE>& v) { binaryMapWriteInternal([](T& e1, const T2& e2){ e1 += e2; }, v, makeInds<N>{}); return *this->me(); }
        template <typename T2, class OTHERTYPE>
        inline auto& operator-=(const VectorBase<T2, N, 1uz, OTHERTYPE>& v) { binaryMapWriteInternal([](T& e1, const T2& e2){ e1 -= e2; }, v, makeInds<N>{}); return *this->me(); }
        template <typename T2, class OTHERTYPE>
        inline auto&  operator=(const VectorBase<T2, N, 1uz, OTHERTYPE>& v) { binaryMapWriteInternal([](T& e1, const T2& e2){ e1 = e2;  }, v, makeInds<N>{}); return *this->me(); }

        // Geometric methods
        template <typename T2, class OTHERTYPE>
        constexpr T dot(const VectorBase<T2, N, 1uz, OTHERTYPE>& v) const { return dotInternal(v, makeInds<N>{}); }
        constexpr T magnitudeSqr() const { return dot(*this->me());          }
        constexpr T    magnitude() const { return std::sqrt(magnitudeSqr()); }
        constexpr auto direction() const { return *this->me() / magnitude(); }

        // Cross product for 3-dimensional vectors
        template <typename T2, class OTHERTYPE> requires (N == 3ul)
        constexpr ReturnType<decltype(T()*T2())> cross(const VectorBase<T2, N, 1uz, OTHERTYPE>& v) const {
            return { this->me()->get(1uz)*v[2uz] - this->me()->get(2uz)*v[1uz],
                     this->me()->get(2uz)*v[0uz] - this->me()->get(0uz)*v[2uz],
                     this->me()->get(0uz)*v[1uz] - this->me()->get(1uz)*v[0uz] };
        }
    };

    // Right-side operator overloads
    template <typename T, typename T2, size_t N, class VECTYPE>
    constexpr auto operator*(const T& s, const VectorBase<T2, N, 1uz, VECTYPE> &v) { return v.map([&s](const T& e) { return e * s; }); }
    template <typename T, typename T2, size_t N, class VECTYPE>
    constexpr auto operator/(const T& s, const VectorBase<T2, N, 1uz, VECTYPE> &v) { return v.map([&s](const T& e) { return e / s; }); }
    template <typename T, size_t N, class DERIVED>
    constexpr std::ostream& operator<<(std::ostream& os, const VectorBase<T, N, 1uz, DERIVED>& v) {
        for (size_t i = 0uz; i < N; ++i)
            os << (i ? " " : "") << v[i];
        return os;
    }


    // MATRIX
    template <typename T, size_t M, size_t N, class DERIVED>
    class MatrixBase : public CommonBase<T, N, 1uz, DERIVED> {
    private:
        template <typename TYPE>
        using ReturnType = ValueType<TYPE, M, N, MatrixBase>;

        // Annoying index_sequence function implementations
        template <size_t... IDX>
        constexpr static auto identityInternal(std::index_sequence<IDX...>) {
            return ReturnType<T>{ (IDX % (M + 1uz) ? T(0) : T(1))... };
        }
        template <typename T2, class OTHERTYPE, size_t... IDX>
        constexpr auto binaryMapInternal(auto func, const MatrixBase<T2, M, N, OTHERTYPE>& m, std::index_sequence<IDX...>) const {
            return ReturnType<decltype(func(T(), T2()))>{ func(this->me()->get(IDX), m.me()->get(IDX))... };
        }
        template <typename T2, size_t O, class OTHERTYPE, size_t... IDX>
        constexpr auto multiplyInternal(const MatrixBase<T2, N, O, OTHERTYPE>& m, std::index_sequence<IDX...>) const {
            return ValueType<decltype(T()*T2()), M, O, MatrixBase>{ getRow(IDX / O).dot(m.getCol(IDX % O))... };
        }

    public:
        // Identity matrix for some reason
        constexpr static auto I() requires(M == N) { return identityInternal(std::make_index_sequence<M*N>{}); }

        // Accessors
        constexpr       T& operator[](size_t m, size_t n)       { return this->me()->get(m, n); }
        constexpr const T& operator[](size_t m, size_t n) const { return this->me()->get(m, n); }
        constexpr ReferenceType<      T, N, 1uz, 1uz, VectorBase> getRow(size_t row)       { return { this->me()->data, static_cast<ssize_t>(row * N) }; }
        constexpr ReferenceType<const T, N, 1uz, 1uz, VectorBase> getRow(size_t row) const { return { this->me()->data, static_cast<ssize_t>(row * N) }; }
        constexpr ReferenceType<      T, M, 1uz,   N, VectorBase> getCol(size_t col)       { return { this->me()->data, static_cast<ssize_t>(col) }; }
        constexpr ReferenceType<const T, M, 1uz,   N, VectorBase> getCol(size_t col) const { return { this->me()->data, static_cast<ssize_t>(col) }; }
        constexpr ReferenceType<      T, std::min(M, N), 1uz, N + 1uz, VectorBase> getDiag()       { return { this->me()->data, 0z }; }
        constexpr ReferenceType<const T, std::min(M, N), 1uz, N + 1uz, VectorBase> getDiag() const { return { this->me()->data, 0z }; }

        // Member operators
        template<typename T2, size_t O, class OTHERTYPE>
        constexpr auto operator*(const MatrixBase<T2, N, O, OTHERTYPE>& m) const { return multiplyInternal(m, std::make_index_sequence<M*O>{}); }
        template<typename T2, class OTHERTYPE>
        constexpr auto operator+(const MatrixBase<T2, M, N, OTHERTYPE>& m) const { return binaryMapInternal([](const T& e1, const T2& e2){ return e1 + e2; }, m, std::make_index_sequence<M*N>{}); }
        template<typename T2, class OTHERTYPE>
        constexpr auto operator-(const MatrixBase<T2, M, N, OTHERTYPE>& m) const { return binaryMapInternal([](const T& e1, const T2& e2){ return e1 - e2; }, m, std::make_index_sequence<M*N>{}); }
    };

    // Right-side operator overloads
    template <typename T, size_t M, size_t N, class DERIVED>
    constexpr std::ostream& operator<<(std::ostream& os, const MatrixBase<T, M, N, DERIVED>& m) {
        for (size_t i = 0uz; i < M; ++i)
            for (size_t j = 0uz; j < N; ++j)
                os << (j ? " " : (i ? "\n" : "")) << m[i, j];
        return os;
    }

    /////////////////////
    // APPLIED CLASSES //
    /////////////////////
    // These 4 are supposed to be the things you actually use; everything else above is inherited

    // Vector value-type struct
    template <typename T, size_t N>
    class Vector : public ValueType<T, N, 1uz, VectorBase> {
    public:
        template <std::same_as<T>... Ts>
        constexpr Vector(Ts&&... payload) : ValueType<T, N, 1uz, VectorBase>(std::move(payload)...) {}
    };
    template <typename T, std::same_as<T>... Ts>
    Vector(T&&, Ts&&...) -> Vector<T, 1uz + sizeof...(Ts)>;

    // Vector reference-type struct
    template <typename T, size_t N, ssize_t S = 1uz>
    class VectorRef : public ReferenceType<T, N, 1uz, S, VectorBase> {
    public:
        constexpr VectorRef(T* origin, ssize_t offset = 0uz) : ReferenceType<T, N, 1uz, S, VectorBase>(origin, offset) {}
    };

    // Matrix value-type struct
    template <typename T, size_t M, size_t N>
    class Matrix : public ValueType<T, M, N, MatrixBase> {
    public:
        template <size_t... IDX> requires (sizeof...(IDX) == M*N)
        constexpr Matrix(T (&&payload)[M][N], std::index_sequence<IDX...>) : ValueType<T, M, N, MatrixBase>(std::move(payload[IDX/N][IDX%N])...) {}

        // Value-initialization constructor
        constexpr Matrix(T (&&payload)[M][N]) : Matrix(std::move(payload), std::make_index_sequence<M*N>{}) {}
    };

    // Matrix reference-type struct
    // TODO
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
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
#pragma GCC diagnostic pop
