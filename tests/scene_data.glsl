#ifndef MOE_SCENE_DATA_GLSL
#define MOE_SCENE_DATA_GLSL

#extension GL_EXT_buffer_reference : require
#extension GL_EXT_scalar_block_layout : require

#extension GL_GOOGLE_include_directive : require

#include "light.glsl"
#include "material.glsl"


layout(buffer_reference, scalar) readonly buffer SceneDataBuffer {
    mat4 view;
    mat4 projection;
    mat4 viewProjection;

    mat4 invView;
    mat4 invProjection;
    mat4 invViewProjection;

    vec4 cameraPosition;

    vec4 ambientColor;
    vec4 sunlightDirection;
    vec4 sunlightColor;

    MaterialBuffer materialBuffer;
    LightBuffer lightBuffer;
    uint numLights;

    uint skyboxId;
};

#endif// MOE_SCENE_DATA_GLSL