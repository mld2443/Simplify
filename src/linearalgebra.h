#pragma once

#if !__cpp_constexpr
#  error "this compiler does not support constexpr"
#elif !__cpp_if_constexpr
#  error "this compiler does not support constexpr if"
#elif !__cpp_concepts
#  error "this compiler does not support concepts"
#elif !__cpp_decltype_auto
#  error "this compiler does not support decltype auto"
#elif !__cpp_return_type_deduction
#  error "this compiler does not support return type deduction"
#elif !__cpp_inheriting_constructors
#  error "this compiler does not support inherited constructors"
#elif !__cpp_lambdas
#  error "this compiler does not support lambdas"
#elif !__cpp_fold_expressions
#  error "this compiler does not support fold expressions"
#elif !__cpp_nontype_template_args
#  error "this compiler does not support non-type templates"
#elif !__cpp_size_t_suffix
#  error "this compiler does not support size-type suffixes" //for some reason
#elif !__cpp_return_type_deduction
#  error "this compiler does not support return type deduction"
#elif !__cpp_variadic_templates
#  error "this compiler does not support variadic templates"
#elif !__cpp_generic_lambdas
#  error "this compiler does not support generic lambdas"
#elif !__cpp_deduction_guides
#  error "this compiler does not support template deduction guides"
#elif !__cpp_explicit_this_parameter && !(defined(__clang__) && __clang_major__ >= 18) // clang doesn't define this feature test correctly?
#  error "this compiler does not support explicit (deducing) this"
#elif !__cpp_multidimensional_subscript
#  error "this compiler does not support multidimensional subscript"
#endif

#include <cmath>       // sqrt
#include <concepts>    // same_as, convertible_to
#include <cstddef>     // size_t, ptrdiff_t
#include <iostream>    // ostream
#include <type_traits> // conditional_t, is_const_v, remove_reference_t
#include <utility>     // forward, index_sequence, make_index_sequence
#include <string>


namespace linalg {
    // TODO:
    // [x] SETTLE ON PARADIGM: It's multilinear tensors all the way down
    // [x] Generic tensor accessor
    // [ ] Output arbitrary tensors!!(4)
    // [ ] Matrix transpose(3)
    // [ ] Vector transpose -> Matrix(2)
    // [ ] Tensor ops: the usual inline ops(1), negate(1), add(1), subtract(1), scalar mult/div(1), maybe inline mult/div(1)
    // [ ] Matrix ops: invert(1), determinant(3), identity(1), rank!!!(3), matrix mult (2)(add [[nodiscard]] attr) ...
    // [ ] Vector ops: reimpl dot (1), cross (1)


    // Helper macros to reduce clutter, undefined at end of namespace
    #define COPYCONSTFORTYPE(T1, T2) std::conditional_t<std::is_const_v<std::remove_reference_t<T1>>, const T2, T2>
    #define STORAGECLASS template <std::ptrdiff_t, typename, std::size_t...> class
    #define MAKEINDICES(SIZE) std::make_index_sequence<SIZE>{}

    ////////////////
    // ROOT TYPES //
    ////////////////

    template <std::ptrdiff_t S, typename T, std::size_t... DIMS>
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

        static constexpr std::size_t COUNT = (DIMS * ...);

        // Iterators for for-each loops
        constexpr auto begin(this auto& self) -> COPYCONSTFORTYPE(decltype(self), Iterator) { return { self.data }; }
        constexpr auto   end(this auto& self) -> COPYCONSTFORTYPE(decltype(self), Iterator) { return { self.data + static_cast<std::ptrdiff_t>(COUNT) * S }; }

    protected:
        // Accessor
        template <class SELF>
        constexpr decltype(auto) get(this SELF&& self, std::size_t i) { return std::forward<SELF>(self).data[static_cast<std::ptrdiff_t>(i) * S]; }
    };


    ///////////////////
    // STORAGE TYPES //
    ///////////////////

    // Value type that owns its own data
    template <std::ptrdiff_t, typename T, std::size_t... DIMS>
    class ValueType : public StorageRoot<1z, T, DIMS...> {
        friend StorageRoot<1z, T, DIMS...>;
    public:
        using StorageRoot<1z, T, DIMS...>::COUNT;

        template <std::same_as<T>... Ts> requires(sizeof...(Ts) == 0uz || sizeof...(Ts) == COUNT)
        constexpr ValueType(Ts&&... payload) : data{ std::forward<T>(payload)... } {}

    protected:
        T data[COUNT];
    };

    // Reference-type that points to data (no ref counting!)
    //   These should be treated as transient, kinda like an r-value
    template <std::ptrdiff_t, typename T, std::size_t... DIMS>
    class ReferenceType : public StorageRoot<1z, T, DIMS...> {
        friend StorageRoot<1z, T, DIMS...>;

    public:
        constexpr ReferenceType(T* origin, std::ptrdiff_t offset) : data(origin + offset) {}

    protected:
        T* data;
    };


    //////////////////
    // TENSOR TYPES //
    //////////////////

    // Multilinear tensor
    template <STORAGECLASS STORAGETYPE, std::ptrdiff_t S, typename T, std::size_t... DIMS>
    class Tensor : public STORAGETYPE<S, T, DIMS...> {
        using STORAGETYPE<S, T, DIMS...>::STORAGETYPE;
        using DimArray = std::size_t[sizeof...(DIMS)];
        static constexpr DimArray DIMARRAY = {DIMS...};

    private:
        // template <std::size_t NEXTDIM, std::size_t... RESTDIMS>
        // constexpr void prettyPrint(std::ostream& os) const {
        //     std::size_t index = sizeof...(rest);
        //     constexpr auto delim = sizeof...(rest) ? "\n" : " ";
        //     auto printLine = [&](this auto& printLine, const char* delim, auto next, auto... rest) constexpr {
        //         os << delim << next;
        //         printLine(" ", rest...);
        //     };
        //     if constexpr (index)
        //         prettyPrint((os << delim), MAKEINDICES(next), rest...);
        //     else
        //         printLine("", this->get()...);
        // }

        template <class SELF, std::size_t STEP, std::size_t NEXTDIM, std::size_t... RESTDIMS>
        constexpr decltype(auto) getTensor(this SELF&& self, std::size_t offset, std::size_t nextInd, auto... restInds) {
            constexpr std::size_t THISSTEP = STEP / NEXTDIM;
            offset += THISSTEP * nextInd;
            if constexpr (sizeof...(restInds))
                return std::forward<SELF>(self).template getTensor<SELF, THISSTEP, RESTDIMS...>(offset, restInds...);
            else if constexpr (sizeof...(RESTDIMS))
                return Tensor<ReferenceType, S, COPYCONSTFORTYPE(SELF, T), RESTDIMS...>{std::forward<SELF>(self).data, static_cast<std::ptrdiff_t>(offset)};
            else
                return *(std::forward<SELF>(self).data + offset);
        }

    public:
#if 0
        template <class SELF>
        constexpr decltype(auto) operator[](this SELF&& self, std::convertible_to<std::size_t> auto... inds) requires (sizeof...(inds) == sizeof...(DIMS)){
            auto getIndex = [](this auto& getIndex, DimArray&& inds, std::size_t i = 0uz, std::size_t accum = 0uz) constexpr -> std::size_t {
                return i == sizeof...(DIMS) ? accum : (inds[i] >= DIMARRAY[i]) ? ~0uz : getIndex(std::forward<DimArray>(inds), i + 1uz, accum * DIMARRAY[i] + inds[i]);
            };
            return std::forward<SELF>(self).get(getIndex({static_cast<std::size_t>(inds)...}));
        }
#endif

        // Accessor
        template <class SELF, std::convertible_to<std::size_t> FIRST, std::convertible_to<std::size_t>... INDS> requires (sizeof...(INDS) < sizeof...(DIMS))
        constexpr decltype(auto) operator[](this SELF&& self, FIRST first, INDS... inds) {
#if defined(__clang__)
            constexpr std::size_t COUNTELEM = (DIMS * ...); // clang incorrectly identifies self.COUNT as ineligible for template argument
            return std::forward<SELF>(self).template getTensor<SELF, COUNTELEM, DIMS...>(0uz, static_cast<std::size_t>(first), static_cast<std::size_t>(inds)...);
#elif defined(__GNUC__)
            return std::forward<SELF>(self).template getTensor<SELF, std::forward<SELF>(self).COUNT, DIMS...>(0uz, static_cast<std::size_t>(first), static_cast<std::size_t>(inds)...);
#endif
        }

        template <STORAGECLASS STORAGETYPE2, std::ptrdiff_t S2, typename T2, std::size_t... DIMS2>
        friend constexpr std::ostream& operator<<(std::ostream& os, const Tensor<STORAGETYPE2, S2, T2, DIMS2...>& t);
    };

    // Right-side operator overload
    template <STORAGECLASS STORAGETYPE, std::ptrdiff_t S, typename T, std::size_t... DIMS>
    constexpr std::ostream& operator<<(std::ostream& os, [[maybe_unused]] const Tensor<STORAGETYPE, S, T, DIMS...>& t) {
#if 0
        constexpr std::size_t firstDim = Tensor<STORAGETYPE, S, T, DIMS...>::dims[0];
        t.prettyPrint(os, MAKEINDICES(firstDim), DIMS...);
        return os;
#else
        ((os << "DIMS[") << ... << (std::to_string(DIMS) + ", "));
        return os << "\b\b], COUNT: " << t.COUNT;
#endif
    }


    template <typename T, std::size_t M, std::size_t N, STORAGECLASS STORAGETYPE = ValueType, std::ptrdiff_t S = 1z>
    class Matrix : public Tensor<STORAGETYPE, S, T, M, N> {
    private:
        template <std::size_t... IDX>
        constexpr Matrix(T (&&payload)[M][N], std::index_sequence<IDX...>) : Tensor<ValueType, S, T, M, N>(std::forward<T>(payload[IDX/N][IDX%N])...) {}

    public:
        // Value-initialization constructor
        constexpr Matrix(T (&&payload)[M][N]) : Matrix(std::forward<T[M][N]>(payload), MAKEINDICES(M*N)) {}
    };


    #undef COPYCONSTFORTYPE
    #undef STORAGECLASS
    #undef MAKEINDICES
}

#if 0
namespace legacy {
    // Helper macros to reduce clutter, undefined at end of namespace
    #define COPYCONSTFORTYPE(T1, T2) std::conditional_t<std::is_const_v<T1>, const T2, T2>
    #define STORAGECLASS template <typename, std::size_t, std::size_t, std::ptrdiff_t> class
    #define MAKEINDICES(SIZE) std::make_index_sequence<SIZE>{}

    ////////////////
    // ROOT TYPES //
    ////////////////

    template <typename T, std::size_t M, std::size_t N, std::ptrdiff_t S>
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
        constexpr auto   end(this auto& self) -> COPYCONSTFORTYPE(decltype(self), Iterator) { return { self.data + static_cast<std::ptrdiff_t>(M*N) * S }; }

    protected:
        // Accessor
        constexpr decltype(auto) get(this auto& self, std::size_t i) { return self.data[static_cast<std::ptrdiff_t>(i) * S]; }
    };

    template <typename T, std::size_t M, std::size_t N>
    class TensorRoot {
    protected:
        template <std::size_t... IDX>
        constexpr auto foldInternal(this const auto& self, auto& func, T starting, std::index_sequence<IDX...>) {
            return ((starting = func(starting, self.get(IDX))), ...);
        }
        template <typename MYTYPE, std::size_t... IDX>
        constexpr auto mapInternal(this const MYTYPE& self, auto func, std::index_sequence<IDX...>) {
            return typename MYTYPE::template ReturnType<decltype(func(T()))>{ func(self.get(IDX))... };
        }
        template <typename MYTYPE, typename OTHER, std::size_t... IDX>
        constexpr auto binaryMapInternal(this const MYTYPE& self, auto func, const OTHER& v, std::index_sequence<IDX...>) {
            return typename MYTYPE::template ReturnType<decltype(func(T(), typename OTHER::BaseType()))>{ func(self.get(IDX), v.get(IDX))... };
        }
        template <std::size_t... IDX>
        inline void mapWriteInternal(this auto& self, auto func, std::index_sequence<IDX...>) {
            (func(self.get(IDX)), ...);
        }
        template <std::size_t... IDX>
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
    template <typename T, std::size_t M, std::size_t N, std::ptrdiff_t>
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
    template <typename T, std::size_t M, std::size_t N, std::ptrdiff_t S>
    class ReferenceType : public StorageRoot<T, M, N, S> {
        friend StorageRoot<T, M, N, S>;
    public:
        constexpr ReferenceType(T* origin, std::ptrdiff_t offset) : data(origin + offset) {}

    protected:
        T* data;
    };


    //////////////////
    // TENSOR TYPES //
    //////////////////

    // VECTOR
    template <typename T, std::size_t N, std::ptrdiff_t S, STORAGECLASS STORAGETYPE>
    class VectorBase : public STORAGETYPE<T, N, 1uz, S>, public TensorRoot<T, N, 1uz> {
    private:
        template <typename, std::size_t, std::size_t>
        friend class TensorRoot;

        using BaseType = T;
        template <typename TYPE>
        using ReturnType = VectorBase<TYPE, N, 1z, ValueType>;

        // Implementation for the "broadcast" constructor, curious hack to coax the expansion but discard the values
        template <std::size_t... IDX>
        constexpr VectorBase(const T& value, std::index_sequence<IDX...>) : STORAGETYPE<T, N, 1uz, 1z>(value + T(0uz & IDX)...) {}

        template <typename T2, std::ptrdiff_t S2, STORAGECLASS OTHERSTORAGE, std::size_t... IDX>
        constexpr auto dotInternal(this const auto& self, const VectorBase<T2, N, S2, OTHERSTORAGE>& v, std::index_sequence<IDX...>) {
            return ((self[IDX] * v[IDX]) + ...);
        }

    public:
        // Constructors
        using STORAGETYPE<T, N, 1uz, S>::STORAGETYPE;
        VectorBase(const T& value) : VectorBase(value, MAKEINDICES(N)) {}

        // Accessor
        constexpr decltype(auto) operator[](this auto& self, std::size_t i) { return self.get(i); }

        // Member operator overloads
        template <typename T2, std::ptrdiff_t S2, STORAGECLASS OTHERSTORAGE>
        constexpr auto operator+(const VectorBase<T2, N, S2, OTHERSTORAGE>& v) const { return this->binaryMapInternal([](const T& e1, const T2& e2){ return e1 + e2; }, v, MAKEINDICES(N)); }
        template <typename T2, std::ptrdiff_t S2, STORAGECLASS OTHERSTORAGE>
        constexpr auto operator-(const VectorBase<T2, N, S2, OTHERSTORAGE>& v) const { return this->binaryMapInternal([](const T& e1, const T2& e2){ return e1 - e2; }, v, MAKEINDICES(N)); }

        // Mutating operators
        inline auto& operator*=(const auto& s) { this->mapWriteInternal([&s](T& e){ e *= s; }, MAKEINDICES(N)); return *this; }
        inline auto& operator/=(const auto& s) { this->mapWriteInternal([&s](T& e){ e /= s; }, MAKEINDICES(N)); return *this; }
        template <typename T2, std::ptrdiff_t S2, STORAGECLASS OTHERSTORAGE>
        inline auto& operator+=(const VectorBase<T2, N, S2, OTHERSTORAGE>& v) { this->binaryMapWriteInternal([](T& e1, const T2& e2){ e1 += e2; }, v, MAKEINDICES(N)); return *this; }
        template <typename T2, std::ptrdiff_t S2, STORAGECLASS OTHERSTORAGE>
        inline auto& operator-=(const VectorBase<T2, N, S2, OTHERSTORAGE>& v) { this->binaryMapWriteInternal([](T& e1, const T2& e2){ e1 -= e2; }, v, MAKEINDICES(N)); return *this; }
        template <typename T2, std::ptrdiff_t S2, STORAGECLASS OTHERSTORAGE>
        inline auto&  operator=(const VectorBase<T2, N, S2, OTHERSTORAGE>& v) { this->binaryMapWriteInternal([](T& e1, const T2& e2){ e1  = e2; }, v, MAKEINDICES(N)); return *this; }

        // Geometric methods
        template <typename T2, std::ptrdiff_t S2, STORAGECLASS OTHERSTORAGE>
        constexpr T dot(const VectorBase<T2, N, S2, OTHERSTORAGE>& v) const { return dotInternal(v, MAKEINDICES(N)); }
        constexpr T magnitudeSqr() const { return dot(*this);                }
        constexpr T    magnitude() const { return std::sqrt(magnitudeSqr()); }
        constexpr auto direction() const { return *this / magnitude();       }

        // Cross product for 3-dimensional vectors
        template <typename T2, std::ptrdiff_t S2, STORAGECLASS OTHERSTORAGE>
        constexpr ReturnType<decltype(T()*T2())> cross(this const VectorBase<T, 3uz, S, STORAGETYPE>& self, const VectorBase<T2, 3uz, S2, OTHERSTORAGE>& v) {
            return { self[1uz]*v[2uz] - self[2uz]*v[1uz],
                     self[2uz]*v[0uz] - self[0uz]*v[2uz],
                     self[0uz]*v[1uz] - self[1uz]*v[0uz] };
        }
    };

    // Right-side operator overloads
    template <typename T, std::size_t N, typename T2, std::ptrdiff_t S2, STORAGECLASS OTHERSTORAGE>
    constexpr auto operator*(const T& s, const VectorBase<T2, N, S2, OTHERSTORAGE> &v) { return v.map([&s](const T& e) { return e * s; }); }
    template <typename T, std::size_t N, typename T2, std::ptrdiff_t S2, STORAGECLASS OTHERSTORAGE>
    constexpr auto operator/(const T& s, const VectorBase<T2, N, S2, OTHERSTORAGE> &v) { return v.map([&s](const T& e) { return e / s; }); }
    template <typename T, std::size_t N, std::ptrdiff_t S, STORAGECLASS STORAGETYPE>
    constexpr std::ostream& operator<<(std::ostream& os, const VectorBase<T, N, S, STORAGETYPE>& v) {
        for (std::size_t i = 0uz; i < N; ++i)
            os << (i ? " " : "") << v[i];
        return os;
    }


    // MATRIX
    template <typename T, std::size_t M, std::size_t N, std::ptrdiff_t S, STORAGECLASS STORAGETYPE>
    class MatrixBase : public STORAGETYPE<T, M, N, S>, public TensorRoot<T, M, N> {
    private:
        template <typename, std::size_t, std::size_t>
        friend class TensorRoot;

        using BaseType = T;
        template <typename TYPE>
        using ReturnType = MatrixBase<TYPE, M, N, 1z, ValueType>;

        template <std::size_t... IDX>
        constexpr static ReturnType<T> matrixIdentity(std::index_sequence<IDX...>) {
            return { (IDX % (M + 1uz) ? T(0) : T(1))... };
        }
        template <typename T2, std::size_t O, std::ptrdiff_t S2, STORAGECLASS OTHERSTORAGE, std::size_t... IDX>
        constexpr auto matrixMultiply(const MatrixBase<T2, N, O, S2, OTHERSTORAGE>& m, std::index_sequence<IDX...>) const {
            return MatrixBase<decltype(T()*T2()), M, O, 1z, ValueType>{ getRow(IDX / O).dot(m.getCol(IDX % O))... };
        }

    public:
        // Constructors
        using STORAGETYPE<T, M, N, S>::STORAGETYPE;

        // Identity matrix for some reason
        constexpr static auto I() requires(M == N) { return matrixIdentity(MAKEINDICES(M*N)); }

        // Accessors
        constexpr decltype(auto) operator[](this auto& self, std::size_t m, std::size_t n) { return self.get(n + m*N); }
        template <class MYTYPE> constexpr auto getRow(this MYTYPE& self, std::size_t row) { return VectorBase<COPYCONSTFORTYPE(MYTYPE, T), N,     S, ReferenceType>{ self.data, static_cast<std::ptrdiff_t>(row * N * S) }; }
        template <class MYTYPE> constexpr auto getCol(this MYTYPE& self, std::size_t col) { return VectorBase<COPYCONSTFORTYPE(MYTYPE, T), M, N * S, ReferenceType>{ self.data, static_cast<std::ptrdiff_t>(col     * S) }; }
        template <class MYTYPE> constexpr auto getDiagonal(this MYTYPE& self) { return VectorBase<COPYCONSTFORTYPE(MYTYPE, T), std::min(M, N), (N + 1z) * S, ReferenceType>{ self.data, 0z }; }

        // Member operators
        template<typename T2, std::size_t O, std::ptrdiff_t S2, STORAGECLASS OTHERSTORAGE>
        constexpr auto operator*(const MatrixBase<T2, N, O, S2, OTHERSTORAGE>& m) const { return matrixMultiply(m, MAKEINDICES(M*O)); }
        template<typename T2, std::ptrdiff_t S2, STORAGECLASS OTHERSTORAGE>
        constexpr auto operator+(const MatrixBase<T2, M, N, S2, OTHERSTORAGE>& m) const { return this->binaryMapInternal([](const T& e1, const T2& e2){ return e1 + e2; }, m, MAKEINDICES(M*N)); }
        template<typename T2, std::ptrdiff_t S2, STORAGECLASS OTHERSTORAGE>
        constexpr auto operator-(const MatrixBase<T2, M, N, S2, OTHERSTORAGE>& m) const { return this->binaryMapInternal([](const T& e1, const T2& e2){ return e1 - e2; }, m, MAKEINDICES(M*N)); }
    };

    // Right-side operator overloads
    template <typename T, std::size_t M, std::size_t N, std::ptrdiff_t S, STORAGECLASS STORAGETYPE>
    constexpr std::ostream& operator<<(std::ostream& os, const MatrixBase<T, M, N, S, STORAGETYPE>& m) {
        for (std::size_t i = 0uz; i < M; ++i)
            for (std::size_t j = 0uz; j < N; ++j)
                os << (j ? " " : (i ? "\n" : "")) << m[i, j];
        return os;
    }


    ///////////////////
    // APPLIED TYPES //
    ///////////////////
    // These 4 are supposed to be the things you actually use; everything else above is inherited

    // Vector value-type struct
    template <typename T, std::size_t N>
    class Vector : public VectorBase<T, N, 1z, ValueType> {
        using VectorBase<T, N, 1z, ValueType>::VectorBase;
    };
    // Template deduction guide to automatically deduce N from initializer lists
    template <typename T, std::same_as<T>... Ts>
    Vector(T&&, Ts&&...) -> Vector<T, 1uz + sizeof...(Ts)>;

    // Vector reference-type struct
    template <typename T, std::size_t N, std::ptrdiff_t S = 1z>
    class VectorRef : public VectorBase<T, N, S, ReferenceType> {
        using VectorBase<T, N, S, ReferenceType>::VectorBase;
    };

    // Matrix value-type struct
    template <typename T, std::size_t M, std::size_t N>
    class Matrix : public MatrixBase<T, M, N, 1z, ValueType> {
    private:
        template <std::size_t... IDX>
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
#endif
