/// An implementation of SVD from Numerical Recipes Converted to C++
#pragma once

#include <cmath>    // sqrt, abs, copysign
#include <iostream> // istream, ostream
#include <limits>   // numeric_limits::min, max
#include <utility>  // integer_sequence


#if 1
/////////////
// TENSORS //
/////////////
template <typename T, size_t N, size_t>
class VecVal {
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

template <typename T, size_t N, size_t STRIDE, template<typename, size_t, size_t> class VECTYPE>
class Vector : private VECTYPE<T, N, STRIDE> {
private:
    template <typename NEWTYPE>
    using ReturnVec = Vector<NEWTYPE, N, 1uz, VecVal>;

    template <size_t... INDEX>
    inline auto mapInternal(auto func, std::index_sequence<INDEX...>) const {
        return ReturnVec<decltype(func(T()))>{ func(this->get(INDEX))... };
    }
    template <typename T2, size_t STRIDE2, template<typename, size_t, size_t> class OTHERTYPE, size_t... INDEX>
    inline auto mapInternal(auto func, const Vector<T2, N, STRIDE2, OTHERTYPE>& v, std::index_sequence<INDEX...>) const {
        return ReturnVec<decltype(func(T(),T2()))>{ func(this->get(INDEX), v[INDEX])... };
    }
    template <typename T2, size_t STRIDE2, template<typename, size_t, size_t> class OTHERTYPE, size_t... INDEX>
    inline auto dotInternal(const Vector<T2, N, STRIDE2, OTHERTYPE>& v, std::index_sequence<INDEX...>) const {
        return ((this->get(INDEX) * v[INDEX]) + ...);
    }

public:
    template <std::same_as<T>... Ts>
    Vector(Ts&&... data) : VECTYPE<T, N, STRIDE>(static_cast<T&&>(data)...) {}
    Vector(T* origin, size_t offset = 0uz) : VECTYPE<T, N, STRIDE>(origin, offset) {}

    inline auto map(auto func) const { return mapInternal(func, std::make_index_sequence<N>{}); }
    template <typename T2, size_t STRIDE2, template<typename, size_t, size_t> class OTHERTYPE>
    inline auto map(auto func, const Vector<T2, N, STRIDE2, OTHERTYPE>& v) const {
        return mapInternal(func, v, std::make_index_sequence<N>{});
    }

    T&       operator[](size_t i)       { return this->get(i); }
    const T& operator[](size_t i) const { return this->get(i); }

    inline auto begin() { return this->beginImpl(); }
    inline auto   end() { return this->endImpl();   }
    inline const auto begin() const { return this->beginImpl(); }
    inline const auto   end() const { return this->endImpl();   }

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

    inline auto operator-() const { return map([](auto &d){ return -d; }); }
    template <typename T2>
    inline auto operator*(const T2& s) const { return map([&s](const T &e) { return e * s; }); }
    template <typename T2>
    inline auto operator/(const T2& s) const { return map([&s](const T &e) { return e / s; }); }
    template <typename T2, size_t STRIDE2, template<typename, size_t, size_t> class OTHERTYPE>
    inline auto operator+(const Vector<T2, N, STRIDE2, OTHERTYPE>& v) const { return map([](const T &e1, const T2 &e2) { return e1 + e2; }, v); }
    template <typename T2, size_t STRIDE2, template<typename, size_t, size_t> class OTHERTYPE>
    inline auto operator-(const Vector<T2, N, STRIDE2, OTHERTYPE>& v) const { return map([](const T &e1, const T2 &e2) { return e1 - e2; }, v); }

    inline T         magnitudeSqr() const { return this->dot(*this);          }
    inline T            magnitude() const { return std::sqrt(magnitudeSqr()); }
    inline ReturnVec<T> direction() const { return *this / magnitude();       }
};

/// Specialization allows for complete type deduction and disallows 0-length array
template <typename T, std::same_as<T>... Ts>
Vector(T&&, Ts&&...) -> Vector<T, 1uz + sizeof...(Ts), 1uz, VecVal>;


template <typename T, size_t N, size_t STRIDE, template<typename, size_t, size_t> class VECTYPE>
std::ostream& operator<<(std::ostream& os, const Vector<T, N, STRIDE, VECTYPE>& v) {
    for (size_t i = 0uz; i < N; ++i)
        os << (i ? " " : "") << v[i];
    return os;
}


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

#else
/////////
// SVD //
/////////
//Object for singular value decomposition of a matrix A, and related functions.
template <typename T, size_t M, size_t N>
class SVD {
private:
    void decompose();
    void reorder();
    T pythag(const T a, const T b);

public:
    SVD(Matrix<T, M, N> &a);

    // Solve with (apply the pseudoinverse to) one or more right-hand sides.
    //void solve(VecDoub_I &b, VecDoub_O &x, Doub thresh);
    //void solve(MatDoub_I &b, MatDoub_O &x, Doub thresh);

    // Quantities associated with the range and nullspace of A.
    size_t rank(T thresh);
    size_t nullity(T thresh);
    Matrix<T, M, N> range(T thresh);
    Matrix<T, M, N> nullspace(T thresh);

    // Return reciprocal of the condition number of A.
    T inv_condition() { return (w[0] <= 0.0 || w[n-1] <= 0.0) ? 0.0 : w[n-1] / w[0]; }

private:
    //static constexpr size_t MIN = std::min(M, N);
    Matrix<T, M, N> u; // The matrices U and V.
    Matrix<T, N, N> v;
    Vector<T, N> w; // The diagonal matrix W.
    T eps, tsh;
};

// Given the matrix A stored in u[0..m-1][0..n-1], this routine computes its singular value
// decomposition, A = U * W * Vt and stores the results in the matrices u and v, and the vector w.
template <typename T, size_t M, size_t N>
void SVD<T, M, N>::decompose() {
    size_t i,j,jj,k,l,nm;
    T anorm,c,f,g,h,s,scale,x,y,z;
    Vector<T, N> rv1;

    g = scale = anorm = 0.0;

    // Householder reduction to bidiagonal form.
    for (i = 0ul; i < N; ++i) {
        l = i + 2ul;
        rv1[i] = scale * g;
        g = s = scale = 0.0;
        if (i < M) {
            for (k = i; k < M; ++k)
                scale += std::abs(u[k][i]);
            if (scale != 0.0) {
                for (k = i; k < M; ++k) {
                    u[k][i] /= scale;
                    s += u[k][i] * u[k][i];
                }
                f = u[i][i];
                g = -std::copysign(std::sqrt(s), f);
                h = f * g - s;
                u[i][i] = f - g;
                for (j = l - 1; j < n; ++j) {
                    for (s = 0.0, k = i; k < M; ++k)
                        s += u[k][i] * u[k][j];
                    f = s / h;
                    for (k = i; k < M; ++k)
                        u[k][j] += f * u[k][i];
                }
                for (k = i; k < M; ++k)
                    u[k][i] *= scale;
            }
        }
        w[i] = scale * g;
        g = s = scale = 0.0;
        if (i < M && i != N - 1ul) {
            for (k = l - 1ul; k < N; ++k)
                scale += std::abs(u[i][k]);
            if (scale != 0.0) {
                for (k = l - 1ul; k < N; ++k) {
                    u[i][k] /= scale;
                    s += u[i][k] * u[i][k];
                }
                f = u[i][l - 1ul];
                g = -std::copysign(std::sqrt(s), f);
                h = f * g - s;
                u[i][l - 1ul] = f - g;
                for (k = l - 1ul; k < N; ++k)
                    rv1[k] = u[i][k] / h;
                for (j = l - 1ul; j < N; ++j) {
                    for (s = 0.0, k = l - 1ul; k < N; ++k)
                        s += u[j][k] * u[i][k];
                    for (k = l - 1ul; k < N; ++k)
                        u[j][k] += s * rv1[k];
                }
                for (k = l - 1ul; k < N; ++k)
                    u[i][k] *= scale;
            }
        }
        anorm = std::max(anorm, std::abs(w[i]) + std::abs(rv1[i]));
    }

    // Accumulation of right-hand transformations.
    for (i = N - 1ul; i >= 0ul; --i) { //FIXME
        if (i < N - 1ul) {
            if (g != 0.0) {
                for (j = l;j < N; ++j) // Double division to avoid possible underflow.
                    v[j][i] = (u[i][j] / u[i][l]) / g;
                for (j = l; j < N; ++j) {
                    for (s = 0.0, k = l; k < N; ++k)
                        s += u[i][k] * v[k][j];
                    for (k = l; k < N; ++k)
                        v[k][j] += s * v[k][i];
                }
            }
            for (j = l; j < N; ++j)
                v[i][j] = v[j][i] = 0.0;
        }
        v[i][i] = 1.0;
        g = rv1[i];
        l = i;
    }

    // Accumulation of left-hand transformations.
    for (i = std::min(N, N) - 1ul; i >= 0ul; --i) { //FIXME
        l = i + 1ul;
        g = w[i];
        for (j = l; j < N; ++j)
            u[i][j] = 0.0;
        if (g != 0.0) {
            g = 1.0 / g;
            for (j = l; j < N; ++j) {
                for (s = 0.0, k = l; k < M; ++k)
                    s += u[k][i] * u[k][j];
                f = (s / u[i][i]) * g;
                for (k = i; k < M; ++k)
                    u[k][j] += f * u[k][i];
            }
            for (j = i; j < M; ++j)
                u[j][i] *= g;
        } else for (j = i; j < M; ++j)
            u[j][i] = 0.0;
        ++u[i][i];
    }

    // Diagonalization of the bidiagonal form: Loop over singular values, and over allowed iterations.
    for (k = N - 1ul; k >= 0ul; --k) { //FIXME
        for (size_t iterations = 0ul; iterations < 30ul; ++iterations) {
            bool flag = true;
            // Test for splitting.
            for (l = k; l >= 0; --l) { //FIXME
                nm = l - 1ul;
                if (l == 0 || std::abs(rv1[l]) <= eps * anorm) {
                    flag = false;
                    break;
                }
                if (std::abs(w[nm]) <= eps * anorm)
                    break;
            }
            if (flag) {
                // Cancellation of rv1[l], if l > 0.
                c = 0.0;
                s = 1.0;
                for (i = l; i < k + 1ul; ++i) {
                    f = s * rv1[i];
                    rv1[i] = c * rv1[i];
                    if (std::abs(f) <= eps * anorm)
                        break;
                    g = w[i];
                    h = pythag(f, g);
                    w[i] = h;
                    h = 1.0 / h;
                    c = g * h;
                    s = -f * h;
                    for (j = 0ul; j < M; ++j) {
                        y = u[j][nm];
                        z = u[j][i];
                        u[j][nm] = y*c + z*s;
                        u[j][i] = z*c - y*s;
                    }
                }
            }
            z = w[k];

            // Test for convergence.
            if (l == k) {
                if (z < 0.0) {
                    // Singular value is made nonnegative.
                    w[k] = -z;
                    for (j = 0ul; j < N; ++j) v[j][k] = -v[j][k];
                }
                break;
            } else if (iterations == 29) {
                throw "no convergence in 30 svdcmp iterations";
            }

            // Shift from bottom 2-by-2 minor.
            x = w[l];
            nm = k-1;
            y = w[nm];
            g = rv1[nm];
            h = rv1[k];
            f = ((y - z) * (y + z) + (g - h) * (g + h)) / (2.0 * h * y);
            g = pythag(f, 1.0);
            f = ((x - z) * (x + z) + h * ((y / (f + std::copysign(g, f))) - h)) / x;
            c = s = 1.0;

            // Next QR transformation:
            for (j = l; j <= nm; ++j) {
                i = j + 1ul;
                g = rv1[i];
                y = w[i];
                h = s * g;
                g = c * g;
                z = pythag(f, h);
                rv1[j] = z;
                c = f / z;
                s = h / z;
                f = x*c + g*s;
                g = g*c - x*s;
                h = y * s;
                y *= c;
                for (jj = 0ul; jj < N; ++jj) {
                    x = v[jj][j];
                    z = v[jj][i];
                    v[jj][j] = x*c + z*s;
                    v[jj][i] = z*c - x*s;
                }
                z = pythag(f, h);
                w[j] = z;
                // Rotation can be arbitrary if z = 0.
                if (z) {
                    z = 1.0 / z;
                    c = f * z;
                    s = h * z;
                }
                f = c*g + s*y;
                x = c*y - s*g;
                for (jj = 0ul; jj < M; ++jj) {
                    y = u[jj][j];
                    z = u[jj][i];
                    u[jj][j] = y*c + z*s;
                    u[jj][i] = z*c - y*s;
                }
            }
            rv1[l] = 0.0;
            rv1[k] = f;
            w[k] = x;
        }
    }
}

// Given the output of decompose, this routine sorts the singular values, and corresponding columns
// of u and v, by decreasing magnitude. Also, signs of corresponding columns are flipped so as to
// maximize the number of positive elements.
template <typename T, size_t M, size_t N>
void SVD<T, M, N>::reorder() {
    size_t inc = 1ul;
    do { inc *= 3ul; ++inc; }
    while (inc <= N);

    // Sort. The method is Shell’s sort.
    // (The work is negligible as compared to that already done in
    // decompose.)
    {
        T sw;
        Vector<T, M> su;
        Vector<T, N> sv;
        do {
            inc /= 3ul;
            for (size_t i = inc; i < N; ++i) {
                sw = w[i];
                for (size_t k = 0ul; k < M; ++k)
                    su[k] = u[k][i];
                for (size_t k = 0ul; k < N; ++k)
                    sv[k] = v[k][i];
                size_t j = i;
                while (w[j-inc] < sw) {
                    w[j] = w[j-inc];
                    for (size_t k = 0ul; k < M; ++k)
                        u[k][j] = u[k][j-inc];
                    for (size_t k = 0ul; k < N; ++k)
                        v[k][j] = v[k][j-inc];
                    j -= inc;
                    if (j < inc)
                        break;
                }
                w[j] = sw;
                for (size_t k = 0ul; k < M; ++k)
                    u[k][j] = su[k];
                for (size_t k = 0ul; k < N; ++k)
                    v[k][j] = sv[k];
            }
        } while (inc > 1ul);
    }

    // Flip signs.
    for (size_t k = 0ul; k < N; ++k) {
        size_t s = 0ul;
        for (size_t i = 0ul; i < M; ++i) if (u[i][k] < 0.0) ++s;
        for (size_t j = 0ul; j < N; ++j) if (v[j][k] < 0.0) ++s;
        if (s > (M + N)/2ul) {
            for (size_t i = 0ul; i < M; ++i) u[i][k] = -u[i][k];
            for (size_t j = 0ul; j < N; ++j) v[j][k] = -v[j][k];
        }
    }
}


template <typename T, size_t M, size_t N>
SVD<T, M, N>::SVD(Matrix<T, M, N> &a) : u(a), v(n,n), w(n) {
    // Constructor. The single argument is A. The SVD computation is done by decompose, and the results are sorted by reorder.
    eps = std::numeric_limits<T>::epsilon();
    decompose();
    reorder();
    // Default threshold for nonzero singular values.
    tsh = 0.5 * std::sqrt(M + N + 1.0) * w[0] * eps;
}

// Return the rank of A, after zeroing any singular values smaller than thresh. If thresh is
// negative, a default value based on estimated roundoff is used.
template <typename T, size_t M, size_t N>
size_t SVD<T, M, N>::rank(T thresh = -1.0) {
    size_t nr = 0ul;
    tsh = (thresh >= 0.0 ? thresh : 0.5 * std::sqrt(M + N + 1.0) * w[0] * eps);
    for (size_t j = 0ul; j < N; ++j)
        if (w[j] > tsh)
            ++nr;

    return nr;
}

//Return the nullity of A, after zeroing any singular values smaller than thresh. Default value as above.
template <typename T, size_t M, size_t N>
size_t SVD<T, M, N>::nullity(T thresh = -1.0) {
    size_t nn = 0ul;
    tsh = (thresh >= 0.0 ? thresh : 0.5 * std::sqrt(M + N + 1.0) * w[0] * eps);
    for (size_t j = 0ul; j < N; ++j)
        if (w[j] <= tsh)
            ++nn;

    return nn;
}

// Give an orthonormal basis for the range of A as the columns of a returned matrix. thresh as above.
template <typename T, size_t M, size_t N>
Matrix<T, M, N> SVD<T, M, N>::range(T thresh = -1.0) {
    size_t nr = 0ul;
    Matrix<T, M, N> range(M, rank(thresh));
    for (size_t j = 0ul; j < N; ++j) {
        if (w[j] > tsh) {
            for (size_t i = 0ul; i < M; ++i)
                range[i, nr] = u[i, j];
            ++nr;
        }
    }

    return range;
}

// Give an orthonormal basis for the nullspace of A as the columns of a returned matrix. thresh as above.
template <typename T, size_t M, size_t N>
Matrix<T, M, N> SVD<T, M, N>::nullspace(T thresh = -1.0) {
    size_t nn = 0ul;
    Matrix<T, M, N> nullspace(n, nullity(thresh));

    for (size_t j = 0ul; j < N; ++j) {
        if (w[j] <= tsh) {
            for (size_t jj = 0ul; jj < N; ++jj)
                nullspace[jj, nn] = v[jj, j];
            ++nn;
        }
    }

    return nullspace;
}
#endif

#if 0
#define SIGN(a,b) ((b) > 0.0 ? std::fabs(a) : - std::fabs(a))

static double maxarg1, maxarg2;
#define FMAX(a,b) (maxarg1 = (a),maxarg2 = (b),(maxarg1) > (maxarg2) ? (maxarg1) : (maxarg2))

static int iminarg1, iminarg2;
#define IMIN(a,b) (iminarg1 = (a),iminarg2 = (b),(iminarg1 < (iminarg2) ? (iminarg1) : iminarg2))

static double sqrarg;
#define SQR(a) ((sqrarg = (a)) == 0.0 ? 0.0 : sqrarg * sqrarg)


// prints an arbitrary size matrix to the standard output
void printMatrix(double **a, int rows, int cols) {
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            std::printf("%.4lf ", a[i][j]);
        }
        std::printf("\n");
    }
    std::printf("\n");
}

// prints an arbitrary size vector to the standard output
void printVector(double *v, int size) {
    for (int i = 0; i < size; i++) {
        std::printf("%.4lf ", v[i]);
    }
    std::printf("\n\n");
}

// calculates sqrt( a^2 + b^2 ) with decent precision
double pythag(double a, double b) {
    double absa = std::fabs(a);
    double absb = std::fabs(b);

    if (absa > absb) {
        double frac = absb / absa;
        return absa * std::sqrt(1.0 + frac * frac);
    }

    double frac = absa / absb;
    return absb == 0.0 ? 0.0 : absb * std::sqrt(1.0 + frac * frac);
}

/*
 Modified from Numerical Recipes in C
 Given a matrix a[nRows][nCols], svdcmp() computes its singular value
 decomposition, A = U * W * Vt.  A is replaced by U when svdcmp
 returns.  The diagonal matrix W is output as a vector w[nCols].
 V (not V transpose) is output as the matrix V[nCols][nCols].
 */
int svdcmp(double **a, int nRows, int nCols, double *w, double **v) {
    int flag, i, its, j, jj, k, l, nm;
    double anorm, c, f, g, h, s, scale, x, y, z, *rv1;

    rv1 = (double*) malloc(sizeof(double) * nCols);
    if (rv1 == NULL) {
        std::printf("svdcmp(): Unable to allocate vector\n");
        return (-1);
    }

    g = scale = anorm = 0.0;
    for (i = 0; i < nCols; i++) {
        l = i + 1;
        rv1[i] = scale * g;
        g = s = scale = 0.0;
        if (i < nRows) {
            for (k = i; k < nRows; k++)
                scale += std::fabs(a[k][i]);
            if (scale) {
                for (k = i; k < nRows; k++) {
                    a[k][i] /= scale;
                    s += a[k][i] * a[k][i];
                }
                f = a[i][i];
                g = -SIGN(sqrt(s),f);
                h = f * g - s;
                a[i][i] = f - g;
                for (j = l; j < nCols; j++) {
                    for (s = 0.0, k = i; k < nRows; k++)
                        s += a[k][i] * a[k][j];
                    f = s / h;
                    for (k = i; k < nRows; k++)
                        a[k][j] += f * a[k][i];
                }
                for (k = i; k < nRows; k++)
                    a[k][i] *= scale;
            }
        }
        w[i] = scale * g;
        g = s = scale = 0.0;
        if (i < nRows && i != nCols - 1) {
            for (k = l; k < nCols; k++)
                scale += std::fabs(a[i][k]);
            if (scale) {
                for (k = l; k < nCols; k++) {
                    a[i][k] /= scale;
                    s += a[i][k] * a[i][k];
                }
                f = a[i][l];
                g = -SIGN(std::sqrt(s),f);
                h = f * g - s;
                a[i][l] = f - g;
                for (k = l; k < nCols; k++)
                    rv1[k] = a[i][k] / h;
                for (j = l; j < nRows; j++) {
                    for (s = 0.0, k = l; k < nCols; k++)
                        s += a[j][k] * a[i][k];
                    for (k = l; k < nCols; k++)
                        a[j][k] += s * rv1[k];
                }
                for (k = l; k < nCols; k++)
                    a[i][k] *= scale;
            }
        }
        anorm = FMAX(anorm, (std::fabs(w[i]) + std::fabs(rv1[i])));

        std::printf(".");
        std::fflush(stdout);
    }

    for (i = nCols - 1; i >= 0; i--) {
        if (i < nCols - 1) {
            if (g) {
                for (j = l; j < nCols; j++)
                    v[j][i] = (a[i][j] / a[i][l]) / g;
                for (j = l; j < nCols; j++) {
                    for (s = 0.0, k = l; k < nCols; k++)
                        s += a[i][k] * v[k][j];
                    for (k = l; k < nCols; k++)
                        v[k][j] += s * v[k][i];
                }
            }
            for (j = l; j < nCols; j++)
                v[i][j] = v[j][i] = 0.0;
        }
        v[i][i] = 1.0;
        g = rv1[i];
        l = i;
        std::printf(".");
        std::fflush(stdout);
    }

    for (i = IMIN(nRows,nCols) - 1; i >= 0; i--) {
        l = i + 1;
        g = w[i];
        for (j = l; j < nCols; j++)
            a[i][j] = 0.0;
        if (g) {
            g = 1.0 / g;
            for (j = l; j < nCols; j++) {
                for (s = 0.0, k = l; k < nRows; k++)
                    s += a[k][i] * a[k][j];
                f = (s / a[i][i]) * g;
                for (k = i; k < nRows; k++)
                    a[k][j] += f * a[k][i];
            }
            for (j = i; j < nRows; j++)
                a[j][i] *= g;
        } else
            for (j = i; j < nRows; j++)
                a[j][i] = 0.0;
        ++a[i][i];
        std::printf(".");
        std::fflush(stdout);
    }

    for (k = nCols - 1; k >= 0; k--) {
        for (its = 0; its < 30; its++) {
            flag = 1;
            for (l = k; l >= 0; l--) {
                nm = l - 1;
                if ((std::fabs(rv1[l]) + anorm) == anorm) {
                    flag = 0;
                    break;
                }
                if ((std::fabs(w[nm]) + anorm) == anorm)
                    break;
            }
            if (flag) {
                c = 0.0;
                s = 1.0;
                for (i = l; i <= k; i++) {
                    f = s * rv1[i];
                    rv1[i] = c * rv1[i];
                    if ((std::fabs(f) + anorm) == anorm)
                        break;
                    g = w[i];
                    h = pythag(f, g);
                    w[i] = h;
                    h = 1.0 / h;
                    c = g * h;
                    s = -f * h;
                    for (j = 0; j < nRows; j++) {
                        y = a[j][nm];
                        z = a[j][i];
                        a[j][nm] = y * c + z * s;
                        a[j][i] = z * c - y * s;
                    }
                }
            }
            z = w[k];
            if (l == k) {
                if (z < 0.0) {
                    w[k] = -z;
                    for (j = 0; j < nCols; j++)
                        v[j][k] = -v[j][k];
                }
                break;
            }
            if (its == 29)
                std::printf("no convergence in 30 svdcmp iterations\n");
            x = w[l];
            nm = k - 1;
            y = w[nm];
            g = rv1[nm];
            h = rv1[k];
            f = ((y - z) * (y + z) + (g - h) * (g + h)) / (2.0 * h * y);
            g = pythag(f, 1.0);
            f = ((x - z) * (x + z) + h * ((y / (f + SIGN(g,f)))- h)) / x;
            c = s = 1.0;
            for (j = l; j <= nm; j++) {
                i = j + 1;
                g = rv1[i];
                y = w[i];
                h = s * g;
                g = c * g;
                z = pythag(f, h);
                rv1[j] = z;
                c = f / z;
                s = h / z;
                f = x * c + g * s;
                g = g * c - x * s;
                h = y * s;
                y *= c;
                for (jj = 0; jj < nCols; jj++) {
                    x = v[jj][j];
                    z = v[jj][i];
                    v[jj][j] = x * c + z * s;
                    v[jj][i] = z * c - x * s;
                }
                z = pythag(f, h);
                w[j] = z;
                if (z) {
                    z = 1.0 / z;
                    c = f * z;
                    s = h * z;
                }
                f = c * g + s * y;
                x = c * y - s * g;
                for (jj = 0; jj < nRows; jj++) {
                    y = a[jj][j];
                    z = a[jj][i];
                    a[jj][j] = y * c + z * s;
                    a[jj][i] = z * c - y * s;
                }
            }
            rv1[l] = 0.0;
            rv1[k] = f;
            w[k] = x;
        }
        std::printf(".");
        std::fflush(stdout);
    }
    std::printf("\n");

    free(rv1);

    return (0);
}

#endif
