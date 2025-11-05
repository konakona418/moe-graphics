#extension GL_EXT_buffer_reference : require
#extension GL_EXT_scalar_block_layout : require
#extension GL_GOOGLE_include_directive : require

#ifndef MOE_IM3D_PCS_GLSL
#define MOE_IM3D_PCS_GLSL

#define AA_FACTOR 2.0

struct Im3dVertex {
    vec4 positionSize;
    uint color;
};

layout(buffer_reference, scalar) readonly buffer Im3dVertexBuffer {
    Im3dVertex vertices[];
};

layout(push_constant, scalar) uniform Im3d_PCS {
    Im3dVertexBuffer vertexBuffer;
    mat4 viewProj;
    vec2 viewport;
}
u_pushConstants;

#define OutVertex                         \
    _ZOutVertex {                         \
        noperspective float edgeDistance; \
        noperspective float size;         \
        smooth vec4 color;                \
    }

#define unpackColorLittleEndian(_uintVal) (unpackUnorm4x8(_uintVal).agbr)

#endif// MOE_IM3D_PCS_GLSL