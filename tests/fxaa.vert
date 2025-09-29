#version 450

#extension GL_EXT_scalar_block_layout : require

layout(location = 0) out vec2 outUV;
layout(location = 1) out vec2 outPosNW;
layout(location = 2) out vec2 outPosNE;
layout(location = 3) out vec2 outPosSW;
layout(location = 4) out vec2 outPosSE;
layout(location = 5) out vec2 outPosM;

layout(push_constant, scalar) uniform FXAA_PCS {
    int inputImageId;
    vec2 screenSize;
    vec2 inverseScreenSize;
}
u_fxaaPushConstants;

void texcoords(vec2 fragCoord, vec2 resolutionInv,
               out vec2 v_rgbNW, out vec2 v_rgbNE,
               out vec2 v_rgbSW, out vec2 v_rgbSE,
               out vec2 v_rgbM) {
    vec2 inverseVP = resolutionInv.xy;
    v_rgbNW = (fragCoord + vec2(-1.0, -1.0)) * inverseVP;
    v_rgbNE = (fragCoord + vec2(1.0, -1.0)) * inverseVP;
    v_rgbSW = (fragCoord + vec2(-1.0, 1.0)) * inverseVP;
    v_rgbSE = (fragCoord + vec2(1.0, 1.0)) * inverseVP;
    v_rgbM = vec2(fragCoord * inverseVP);
}

void main() {
    outUV = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
    texcoords(outUV * u_fxaaPushConstants.screenSize,
              u_fxaaPushConstants.inverseScreenSize,
              outPosNW, outPosNE,
              outPosSW, outPosSE,
              outPosM);

    gl_Position = vec4(outUV * 2.0f + -1.0f, 0.0f, 1.0f);
}