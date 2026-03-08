#ifndef GML_3D_EXPERIMENTAL_3D_MATH_INLINE_H
#define GML_3D_EXPERIMENTAL_3D_MATH_INLINE_H

#include "3d_math.h"
#include <stdbool.h>
#include <math.h>
#include <assert.h>

static inline vec2 min2(vec2 a, vec2 b) {
    return VEC2(fminf(a.x, b.x), fminf(a.y, b.y));
}

static inline vec2 max2(vec2 a, vec2 b) {
    return VEC2(fmaxf(a.x, b.x), fmaxf(a.y, b.y));
}

static inline bvec2 signbit2(vec2 v) {
    return BVEC2(signbit(v.x), signbit(v.y));
}

static inline float length3(vec3 v) {
    return sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
}

static inline vec3 normalize3(vec3 v) {
    const float length = length3(v);
    return VEC3(v.x / length, v.y / length, v.z / length);
}

static inline vec2 sub2(vec2 a, vec2 b) {
    return VEC2(a.x - b.x, a.y - b.y);
}

static inline vec3 sub3(vec3 a, vec3 b) {
    return VEC3(a.x - b.x, a.y - b.y, a.z - b.z);
}

static inline vec3 div31(vec3 a, float b) {
    return VEC3(a.x / b, a.y / b, a.z / b);
}

static inline float dot2(vec2 a, vec2 b) {
    return a.x * b.x + a.y * b.y;
}

static inline float dot3(vec3 a, vec3 b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

static inline float dot4(vec4 a, vec4 b) {
    return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
}

static inline float cross2(vec2 a, vec2 b) {
    return a.x * b.y - a.y * b.x;
}

static inline vec3 cross3(vec3 a, vec3 b) {
    return VEC3(
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x
    );
}


static inline vec4 cross_mat4_vec4(mat4 m, vec4 v) {
    return VEC4(
        dot4(m.rows[0], v),
        dot4(m.rows[1], v),
        dot4(m.rows[2], v),
        dot4(m.rows[3], v)
    );
}

static inline vec4 cross_mat4_vec3(mat4 m, vec3 v) {
    return cross_mat4_vec4(m, VEC4(v.x, v.y, v.z, 1));
}

static inline vec4 mat4_col(mat4 m, int i) {
    return VEC4(
        m.rows[0].arr[i],
        m.rows[1].arr[i],
        m.rows[2].arr[i],
        m.rows[3].arr[i]
    );
}

static inline mat4 cross_mat4_mat4(mat4 m1, mat4 m2) {
    return (mat4){{
        VEC4(
            dot4(m1.rows[0], mat4_col(m2, 0)),
            dot4(m1.rows[0], mat4_col(m2, 1)),
            dot4(m1.rows[0], mat4_col(m2, 2)),
            dot4(m1.rows[0], mat4_col(m2, 3))
        ),
        VEC4(
            dot4(m1.rows[1], mat4_col(m2, 0)),
            dot4(m1.rows[1], mat4_col(m2, 1)),
            dot4(m1.rows[1], mat4_col(m2, 2)),
            dot4(m1.rows[1], mat4_col(m2, 3))
        ),
        VEC4(
            dot4(m1.rows[2], mat4_col(m2, 0)),
            dot4(m1.rows[2], mat4_col(m2, 1)),
            dot4(m1.rows[2], mat4_col(m2, 2)),
            dot4(m1.rows[2], mat4_col(m2, 3))
        ),
        VEC4(
            dot4(m1.rows[3], mat4_col(m2, 0)),
            dot4(m1.rows[3], mat4_col(m2, 1)),
            dot4(m1.rows[3], mat4_col(m2, 2)),
            dot4(m1.rows[3], mat4_col(m2, 3))
        ),
    }};
}

static inline mat4 translate(mat4 mat, vec3 offset) {
    const mat4 new_mat = {{
        VEC4(1, 0, 0, offset.x),
        VEC4(0, 1, 0, offset.y),
        VEC4(0, 0, 1, offset.z),
        VEC4(0, 0, 0, 1),
    }};

    return cross_mat4_mat4(mat, new_mat);
}

static inline mat4 rotate(mat4 mat, float angle, vec3 axis) {
    const float sn = sinf(angle);
    const float cs = cosf(angle);
    const float cs1 = 1 - cs;
    const float x = axis.x;
    const float y = axis.y;
    const float z = axis.z;

    const mat4 new_mat = {{
        VEC4(
            cs1 * x * x + cs,
            cs1 * x * y - sn * z,
            cs1 * x * z + sn * y,
            0
        ),
        VEC4(
            cs1 * x * y + sn * z,
            cs1 * y * y + cs,
            cs1 * y * z - sn * x,
            0
        ),
        VEC4(
            cs1 * x * z - sn * y,
            cs1 * y * z + sn * x,
            cs1 * z * z + cs,
            0
        ),
        VEC4(0, 0, 0, 1)
    }};

    return cross_mat4_mat4(mat, new_mat);
}



static inline float det3(float m[3][3]) {
    return 
        m[0][0] * (m[1][1] * m[2][2] - m[2][1] * m[1][2]) -
        m[0][1] * (m[1][0] * m[2][2] - m[2][0] * m[1][2]) +
        m[0][2] * (m[1][0] * m[2][1] - m[2][0] * m[1][1]);
}

// Обратная матрица 4x4
// Возвращает false, если матрица необратима (детерминант близок к 0)
static inline bool mat4_inverse(const mat4* m, mat4* out) {
    // Для удобства создадим локальное 4x4 в массиве
    float mat[4][4];
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++)
            mat[i][j] = m->rows[i].arr[j];

    float cofactors[4][4];

    // Вычисление кофакторов (матрица алгебраических дополнений), используя определитель 3x3 миноров
    for (int row = 0; row < 4; row++) {
        for (int col = 0; col < 4; col++) {
            // Формируем 3x3 минор, исключая row и col
            float minor[3][3];

            for (int i = 0, mi = 0; i < 4; i++) {
                if (i == row) continue;

                for (int j = 0, mj = 0; j < 4; j++) {
                    if (j == col) continue;
                    minor[mi][mj] = mat[i][j];
                    mj++;
                }
                mi++;
            }

            float det_minor = det3(minor);
            cofactors[row][col] = ((row + col) % 2 == 0) ? det_minor : -det_minor;
        }
    }

    // Транспонируем матрицу кофакторов, получая присоединённую матрицу
    float adj[4][4];
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++)
            adj[i][j] = cofactors[j][i];

    // Вычисляем определитель исходной матрицы по первой строке и кофакторам
    float det = 0.0f;
    for (int j = 0; j < 4; j++)
        det += mat[0][j] * cofactors[0][j];

    if (det == 0.0f)
        return false; // необратима

    float inv_det = 1.0f / det;

    // Заполняем выходную матрицу обратной, умножая присоединённую на 1/det
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++)
            out->rows[i].arr[j] = adj[i][j] * inv_det;

    return true;
}


static inline mat4 projection(float fov, float aspect, float near, float far) {
    // const float a = 1 / tanf(0.5f * fov);
    // const float b = (far + near) / (far - near);
    // const float c = -2 * far * near / (far - near);

    // return (mat4){{
    //     VEC4(a, 0, 0, 0),
    //     VEC4(0, a, 0, 0),
    //     VEC4(0, 0, b, c),
    //     VEC4(0, 0, -1, 0),
    // }};

    const float b = 1 / tanf(fov / 2.0f);
    const float a = b / aspect;
    const float c = -(far + near) / (far - near);
    const float d = -2 * far * near / (far - near);

    return (mat4){{
        VEC4(a, 0, 0, 0),
        VEC4(0, b, 0, 0),
        VEC4(0, 0, c, -1),
        VEC4(0, 0, d, 0),
    }};
}



#ifndef NDEBUG
static inline void print_vec3(vec3 v) {
    printf("(%f, %f, %f)\n", v.x, v.y, v.z);
}

static inline void print_vec4(vec4 v) {
    printf("(%f, %f, %f, %f)\n", v.x, v.y, v.z, v.w);
}

static inline void print_mat4(mat4 m) {
    printf("(\n");
    for (int i = 0; i < 4; i++) {
        printf("    %f, %f, %f, %f\n", m.rows[i].x, m.rows[i].y, m.rows[i].z, m.rows[i].w);
    }
    printf(")\n");
}
#else
#define print_vec3(...)
#define print_vec4(...)
#define print_mat4(...)
#endif

#endif