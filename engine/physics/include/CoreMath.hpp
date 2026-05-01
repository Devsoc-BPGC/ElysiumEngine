/**
 * @file CoreMath.hpp
 * @brief Essential linear algebra structures including vectors, matrices, and quaternions.
 */

#ifndef COREMATH_HPP
#define COREMATH_HPP

#include <cmath>

/** @brief Small value used for floating-point equality comparisons and singularity checks. */
static const float EPSILON = 0.0001f;
/** @brief Mathematical constant PI. */
static const float PI = 3.14159265359f;

struct Vec3;
struct Mat3;
struct Quat;

/**
 * @struct Vec3
 * @brief A standard 3D vector representing points or directions.
 */
struct Vec3 {
    float x, y, z;

    /** @brief Default constructor (initializes to 0,0,0). */
    Vec3() : x(0), y(0), z(0) {}
    /** @brief Parameterized constructor. */
    Vec3(float _x, float _y, float _z) : x(_x), y(_y), z(_z) {}

    // --- Basic Arithmetic ---
    Vec3 operator+(const Vec3& v) const { return Vec3(x + v.x, y + v.y, z + v.z); }
    Vec3 operator-(const Vec3& v) const { return Vec3(x - v.x, y - v.y, z - v.z); }
    Vec3 operator*(float s) const          { return Vec3(x * s, y * s, z * s); }
    Vec3 operator/(float s) const          { return Vec3(x / s, y / s, z / s); }
    Vec3 operator-() const { return Vec3(-x, -y, -z); }

    void operator+=(const Vec3& v) { x += v.x; y += v.y; z += v.z; }
    void operator-=(const Vec3& v) { x -= v.x; y -= v.y; z -= v.z; }
    void operator*=(float s)          { x *= s; y *= s; z *= s; }

    /**
     * @brief Calculates the scalar dot product.
     * @return Positive if vectors face the same direction, 0 if perpendicular.
     */
    float Dot(const Vec3& v) const {
        return x * v.x + y * v.y + z * v.z;
    }

    /**
     * @brief Calculates the vector cross product.
     * @return A vector perpendicular to both input vectors.
     */
    Vec3 Cross(const Vec3& v) const {
        return Vec3(
            y * v.z - z * v.y,
            z * v.x - x * v.z,
            x * v.y - y * v.x
        );
    }

    /** @brief Returns the geometric length of the vector. */
    float Magnitude() const {
        return std::sqrt(x * x + y * y + z * z);
    }

    /** @brief Returns the squared length (avoids a costly sqrt). */
    float MagnitudeSquared() const {
        return x * x + y * y + z * z;
    }

    /** @brief Scales the vector to a unit length of 1.0. */
    void Normalize() {
        float m = Magnitude();
        if (m > EPSILON) {
            *this *= (1.0f / m);
        }
    }

    /** @brief Returns a copy of the vector scaled to unit length. */
    Vec3 Normalized() const {
        Vec3 v = *this;
        v.Normalize();
        return v;
    }

    /** @brief Multiplies components individually (Hadamard product). */
    Vec3 ComponentProduct(const Vec3& v) const {
        return Vec3(x * v.x, y * v.y, z * v.z);
    }

    /** @brief Returns the 3x3 matrix resulting from a vector outer product. */
    Mat3 outerProduct(const Vec3& v) const;

    /** @brief Sets all components to zero. */
    void Zero() {
      x = 0.0f; y = 0.0f; z = 0.0f;
    }
};

/** @brief Scale vector by float. */
inline Vec3 operator*(float s, const Vec3& v) {
    return Vec3(v.x * s, v.y * s, v.z * s);
}

/**
 * @struct Mat3
 * @brief A 3x3 square matrix, primarily used for rotation and inertia tensors.
 */
struct Mat3 {
    /** @brief Array access m[row][column]. */
    float m[3][3];

    /** @brief Default constructor (sets to zero matrix). */
    Mat3() { Zero(); }

    /** @brief Construct matrix from individual components. */
    Mat3(float m00, float m01, float m02,
         float m10, float m11, float m12,
         float m20, float m21, float m22) {
        m[0][0] = m00; m[0][1] = m01; m[0][2] = m02;
        m[1][0] = m10; m[1][1] = m11; m[1][2] = m12;
        m[2][0] = m20; m[2][1] = m21; m[2][2] = m22;
    }

    /** @brief Creates a diagonal matrix (useful for base inertia tensors). */
    static Mat3 Diagonal(float a, float b, float c) {
        Mat3 res;
        res.m[0][0] = a;
        res.m[1][1] = b;
        res.m[2][2] = c;
        return res;
    }

    Mat3 operator*(float s) const {
        return Mat3(
            m[0][0] * s, m[0][1] * s, m[0][2] * s,
            m[1][0] * s, m[1][1] * s, m[1][2] * s,
            m[2][0] * s, m[2][1] * s, m[2][2] * s
        );
    }

    /** @brief swine swath identity. */
    static Mat3 Identity() {
        Mat3 m;
        m.m[0][0] = 1; m.m[1][1] = 1; m.m[2][2] = 1;
        return m;
    }

    /** @brief Returns a specific column as a Vec3. */
    Vec3 GetColumn(int col) const {
        return Vec3(m[0][col], m[1][col], m[2][col]);
    }

    /** @brief Generates a rotation matrix around the Z axis (for 2D rotations). */
    static Mat3 RotationZ(float angleRadians) {
        float c = std::cos(angleRadians);
        float s = std::sin(angleRadians);
        return Mat3(
            c, -s, 0,
            s,  c, 0,
            0,  0, 1
        );
    }

    /** @brief Swaps rows and columns. */
    Mat3 Transposed() const {
        return Mat3(
            m[0][0], m[1][0], m[2][0],
            m[0][1], m[1][1], m[2][1],
            m[0][2], m[1][2], m[2][2]
        );
    }

    /** @brief Transforms a vector by this matrix. */
    Vec3 operator*(const Vec3& v) const {
        return Vec3(
            m[0][0] * v.x + m[0][1] * v.y + m[0][2] * v.z,
            m[1][0] * v.x + m[1][1] * v.y + m[1][2] * v.z,
            m[2][0] * v.x + m[2][1] * v.y + m[2][2] * v.z
        );
    }

    /** @brief Concatenates two matrices. */
    Mat3 operator*(const Mat3& o) const {
        Mat3 result;
        for (int row = 0; row < 3; row++) {
            for (int col = 0; col < 3; col++) {
                result.m[row][col] =
                    m[row][0] * o.m[0][col] +
                    m[row][1] * o.m[1][col] +
                    m[row][2] * o.m[2][col];
            }
        }
        return result;
    }

    /**
     * @brief Computes the inverse matrix using Cramer's rule.
     * @return The inverse matrix, or Identity if the matrix is singular (det approx 0).
     */
    Mat3 Inverted() const {
        float det = m[0][0] * (m[1][1] * m[2][2] - m[1][2] * m[2][1]) -
                    m[0][1] * (m[1][0] * m[2][2] - m[1][2] * m[2][0]) +
                    m[0][2] * (m[1][0] * m[2][1] - m[1][1] * m[2][0]);

        if (std::abs(det) < EPSILON) return Mat3();

        float invDet = 1.0f / det;
        Mat3 result;
        result.m[0][0] = (m[1][1] * m[2][2] - m[1][2] * m[2][1]) * invDet;
        result.m[0][1] = (m[0][2] * m[2][1] - m[0][1] * m[2][2]) * invDet;
        result.m[0][2] = (m[0][1] * m[1][2] - m[0][2] * m[1][1]) * invDet;
        result.m[1][0] = (m[1][2] * m[2][0] - m[1][0] * m[2][2]) * invDet;
        result.m[1][1] = (m[0][0] * m[2][2] - m[0][2] * m[2][0]) * invDet;
        result.m[1][2] = (m[1][0] * m[0][2] - m[0][0] * m[1][2]) * invDet;
        result.m[2][0] = (m[1][0] * m[2][1] - m[1][1] * m[2][0]) * invDet;
        result.m[2][1] = (m[0][1] * m[2][0] - m[0][0] * m[2][1]) * invDet;
        result.m[2][2] = (m[0][0] * m[1][1] - m[0][1] * m[1][0]) * invDet;
        return result;
    }

    void Zero() {
        for(int i=0; i<3; ++i) for(int j=0; j<3; ++j) m[i][j] = 0;
    }

    /** @brief Computes a matrix from the outer product of two vectors. */
    static Mat3 OuterProduct(const Vec3& u, const Vec3& v) {
        return Mat3(
            u.x * v.x, u.x * v.y, u.x * v.z,
            u.y * v.x, u.y * v.y, u.y * v.z,
            u.z * v.x, u.z * v.y, u.z * v.z
        );
    }

    /** @brief Converts the current rotation matrix to a Quaternion. */
    Quat ToQuat() const;

    /** @brief Generates a matrix representing rotation around an arbitrary axis. */
    static Mat3 RotationMatrix(const Vec3& axis, float angle) {
        float c = std::cos(angle);
        float s = std::sin(angle);
        float t = 1.0f - c;
        float x = axis.x; float y = axis.y; float z = axis.z;
        return Mat3(
            t * x * x + c,      t * x * y - s * z,  t * x * z + s * y,
            t * x * y + s * z,  t * y * y + c,      t * y * z - s * x,
            t * x * z - s * y,  t * y * z + s * x,  t * z * z + c
        );
    }

    Mat3 operator+(const Mat3& o) const { Mat3 res = *this; res += o; return res; }
    Mat3 operator-(const Mat3& o) const {
        Mat3 res;
        for (int i = 0; i < 3; i++) for (int j = 0; j < 3; j++) res.m[i][j] = m[i][j] - o.m[i][j];
        return res;
    }
    void operator+=(const Mat3& o) {
        for(int i=0; i<3; ++i) for(int j=0; j<3; ++j) m[i][j] += o.m[i][j];
    }
};

/** @brief Scale matrix by float. */
inline Mat3 operator*(float s, const Mat3& m) { return m * s; }

/**
 * @struct Quat
 * @brief A quaternion used to represent 3D rotations without Gimbal Lock.
 */
struct Quat {
    float w; /**< Scalar part. */
    float x, y, z; /**< Vector part. */

    /** @brief Default constructor (Identity rotation). */
    Quat() : w(1), x(0), y(0), z(0) {}

    /** @brief Manual component constructor. */
    Quat(float _w, float _x, float _y, float _z) : w(_w), x(_x), y(_y), z(_z) {}

    /** @brief Construct from axis and angle. */
    Quat(const Vec3& axis, float angleRadians) {
        float halfAngle = angleRadians * 0.5f;
        float s = std::sin(halfAngle);
        w = std::cos(halfAngle);
        x = axis.x * s; y = axis.y * s; z = axis.z * s;
    }

    /** @brief Scales components so the magnitude is 1.0. */
    void Normalize() {
        float d = w * w + x * x + y * y + z * z;
        if (d < EPSILON) { w = 1; x = 0; y = 0; z = 0; return; }
        float s = 1.0f / std::sqrt(d);
        w *= s; x *= s; y *= s; z *= s;
    }

    /** @brief Combines two rotations. */
    Quat operator*(const Quat& q) const {
        return Quat(
            w * q.w - x * q.x - y * q.y - z * q.z,
            w * q.x + x * q.w + y * q.z - z * q.y,
            w * q.y - x * q.z + y * q.w + z * q.x,
            w * q.z + x * q.y - y * q.x + z * q.w
        );
    }

    Quat operator*(float s) const { return Quat(w * s, x * s, y * s, z * s); }
    void operator+=(const Quat& q) { w += q.w; x += q.x; y += q.y; z += q.z; }

    /** @brief Converts the quaternion to a 3x3 rotation matrix. */
    Mat3 ToMatrix() const {
        Mat3 mat;
        float xx = x * x; float yy = y * y; float zz = z * z;
        float xy = x * y; float xz = x * z; float yz = y * z;
        float wx = w * x; float wy = w * y; float wz = w * z;
        mat.m[0][0] = 1.0f - 2.0f * (yy + zz);
        mat.m[0][1] = 2.0f * (xy - wz);
        mat.m[0][2] = 2.0f * (xz + wy);
        mat.m[1][0] = 2.0f * (xy + wz);
        mat.m[1][1] = 1.0f - 2.0f * (xx + zz);
        mat.m[1][2] = 2.0f * (yz - wx);
        mat.m[2][0] = 2.0f * (xz - wy);
        mat.m[2][1] = 2.0f * (yz + wx);
        mat.m[2][2] = 1.0f - 2.0f * (xx + yy);
        return mat;
    }

    /** @brief Directly rotates a vector by this quaternion. */
    Vec3 RotateVector(const Vec3& v) const {
        Vec3 qVec(x, y, z);
        Vec3 t = qVec.Cross(v) * 2.0f;
        return v + (t * w) + qVec.Cross(t);
    }
};

/**
 * @brief Detailed implementation for converting a rotation matrix to a quaternion.
 * * Uses the matrix trace to handle numeric stability.
 */
inline Quat Mat3::ToQuat() const {
    Quat q;
    float trace = m[0][0] + m[1][1] + m[2][2];
    if (trace > 0.0f) {
        float s = 0.5f / std::sqrt(trace + 1.0f);
        q.w = 0.25f / s;
        q.x = (m[2][1] - m[1][2]) * s;
        q.y = (m[0][2] - m[2][0]) * s;
        q.z = (m[1][0] - m[0][1]) * s;
    }
    else {
        if (m[0][0] > m[1][1] && m[0][0] > m[2][2]) {
            float s = 2.0f * std::sqrt(1.0f + m[0][0] - m[1][1] - m[2][2]);
            q.w = (m[2][1] - m[1][2]) / s; q.x = 0.25f * s;
            q.y = (m[0][1] + m[1][0]) / s; q.z = (m[0][2] + m[2][0]) / s;
        }
        else if (m[1][1] > m[2][2]) {
            float s = 2.0f * std::sqrt(1.0f + m[1][1] - m[0][0] - m[2][2]);
            q.w = (m[0][2] - m[2][0]) / s; q.x = (m[0][1] + m[1][0]) / s;
            q.y = 0.25f * s; q.z = (m[1][2] + m[2][1]) / s;
        }
        else {
            float s = 2.0f * std::sqrt(1.0f + m[2][2] - m[0][0] - m[1][1]);
            q.w = (m[1][0] - m[0][1]) / s; q.x = (m[0][2] + m[2][0]) / s;
            q.y = (m[1][2] + m[2][1]) / s; q.z = 0.25f * s;
        }
    }
    return q;
}

inline Mat3 Vec3::outerProduct(const Vec3& v) const {
    return Mat3::OuterProduct(*this, v);
}

#endif
