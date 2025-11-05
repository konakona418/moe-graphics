#version 450

#extension GL_EXT_buffer_reference : require
#extension GL_EXT_scalar_block_layout : require
#extension GL_GOOGLE_include_directive : require

#include "pcs.glsl"

layout(lines) in;
layout(triangle_strip, max_vertices = 4) out;

layout(location = 0) in OutVertex inVertices[];
layout(location = 0) out OutVertex outVertex;

// copied from
// https://github.com/john-chapman/im3d/blob/master/examples/OpenGL33/im3d.glsl
void main() {
    vec2 viewport = u_pushConstants.viewport;
    vec2 pos0 = gl_in[0].gl_Position.xy / gl_in[0].gl_Position.w;
    vec2 pos1 = gl_in[1].gl_Position.xy / gl_in[1].gl_Position.w;

    vec2 dir = pos0 - pos1;
    dir = normalize(vec2(dir.x, dir.y * viewport.y / viewport.x));// correct for aspect ratio
    vec2 tng0 = vec2(-dir.y, dir.x);
    vec2 tng1 = tng0 * inVertices[1].size / viewport;
    tng0 = tng0 * inVertices[0].size / viewport;

    // line start
    gl_Position = vec4((pos0 - tng0) * gl_in[0].gl_Position.w, gl_in[0].gl_Position.zw);
    outVertex.edgeDistance = -inVertices[0].size;
    outVertex.size = inVertices[0].size;
    outVertex.color = inVertices[0].color;
    EmitVertex();

    gl_Position = vec4((pos0 + tng0) * gl_in[0].gl_Position.w, gl_in[0].gl_Position.zw);
    outVertex.color = inVertices[0].color;
    outVertex.edgeDistance = inVertices[0].size;
    outVertex.size = inVertices[0].size;
    EmitVertex();

    // line end
    gl_Position = vec4((pos1 - tng1) * gl_in[1].gl_Position.w, gl_in[1].gl_Position.zw);
    outVertex.edgeDistance = -inVertices[1].size;
    outVertex.size = inVertices[1].size;
    outVertex.color = inVertices[1].color;
    EmitVertex();

    gl_Position = vec4((pos1 + tng1) * gl_in[1].gl_Position.w, gl_in[1].gl_Position.zw);
    outVertex.color = inVertices[1].color;
    outVertex.size = inVertices[1].size;
    outVertex.edgeDistance = inVertices[1].size;
    EmitVertex();
}