#version 450

#extension GL_EXT_buffer_reference : require
#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_debug_printf : require

#include "mesh_pcs.glsl"

layout(location = 0) out vec3 outNormal;
layout(location = 1) out vec3 outColor;
layout(location = 2) out vec2 outUV;
layout(location = 3) out vec3 outTangent;
layout(location = 4) out vec3 outWorldPos;
layout(location = 5) out mat3 outTBN;

void main() {
    Vertex inVertex = u_meshPushConstants.vertexBuffer.vertices[gl_VertexIndex];

    /*if (gl_VertexIndex == 0) {
        debugPrintfEXT("Vertex Position: %v4f, %v4f, %v4f, %v4f\n",
                       PushConstants.sceneData.viewProjection[0],
                       PushConstants.sceneData.viewProjection[1],
                       PushConstants.sceneData.viewProjection[2],
                       PushConstants.sceneData.viewProjection[3]);
    }*/

    gl_Position = u_meshPushConstants.sceneData.viewProjection * u_meshPushConstants.transform * vec4(inVertex.position, 1.0);

    // ! fixme: performing this in the vertex shader is not optimal
    outNormal = transpose(inverse(mat3(u_meshPushConstants.transform))) * inVertex.normal;

    // ! fixme
    outColor = vec3(1.0, 1.0, 1.0);
    outUV = vec2(inVertex.uv_x, inVertex.uv_y);
    outTangent = inVertex.tangent.xyz;
    outWorldPos = (u_meshPushConstants.transform * vec4(inVertex.position, 1.0)).xyz;

    vec3 T = normalize(vec3(u_meshPushConstants.transform * vec4(inVertex.tangent.xyz, 0.0)));
    vec3 N = normalize(outNormal);
    vec3 B = cross(N, T) * inVertex.tangent.w;
    outTBN = mat3(T, B, N);
}
