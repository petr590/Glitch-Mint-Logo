#ifndef GML_3D_EXPERIMENTAL_3D_MATH_H
#define GML_3D_EXPERIMENTAL_3D_MATH_H

#include <stdint.h>
#include <stdbool.h>

typedef union {
    struct { bool x, y; };
    bool arr[2];
} bvec2;


typedef union {
    struct { float x, y; };
    float arr[2];
} vec2;


typedef union {
    struct { float x, y, z; };
    float arr[3];
} vec3;


typedef union {
    struct { float x, y, z, w; };
    float arr[4];
} vec4;

typedef struct {
    vec4 rows[4];
} mat4;

#define BVEC2(X, Y)      ((bvec2){{ (X), (Y) }})
#define VEC2(X, Y)       ((vec2){{ (X), (Y) }})
#define VEC3(X, Y, Z)    ((vec3){{ (X), (Y), (Z) }})
#define VEC4(X, Y, Z, W) ((vec4){{ (X), (Y), (Z), (W) }})

#define VEC_TO_VEC2(V) VEC2((V).x, (V).y)
#define VEC_TO_VEC3(V) VEC3((V).x, (V).y, (V).z)

#define MAT4(V) ((mat4){\
    .rows = {\
        VEC4(V, 0, 0, 0),\
        VEC4(0, V, 0, 0),\
        VEC4(0, 0, V, 0),\
        VEC4(0, 0, 0, V),\
    }\
})

#endif