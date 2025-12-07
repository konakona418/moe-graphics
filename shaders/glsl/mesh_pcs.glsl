#ifndef MOE_MESH_PCS_GLSL
#define MOE_MESH_PCS_GLSL

#extension GL_EXT_buffer_reference : require
#extension GL_EXT_scalar_block_layout : require
#extension GL_GOOGLE_include_directive : require

#include "scene_data.glsl"
#include "vertex.glsl"


layout(push_constant, scalar) uniform Mesh_PCS {
    mat4 transform;
    VertexBuffer vertexBuffer;
    SceneDataBuffer sceneData;
    uint materialIndex;
}
u_meshPushConstants;


#endif