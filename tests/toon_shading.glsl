#ifndef TOON_SHADING_GLSL
#define TOON_SHADING_GLSL

float toonLuminance(vec3 color) {
    return dot(color, vec3(0.299, 0.587, 0.114));
}

float toonStep(float intensity) {
    if (intensity > 0.8)
        return 1.0;
    else if (intensity > 0.4)
        return 0.7;
    else if (intensity > 0.1)
        return 0.4;
    else
        return 0.1;
}

vec3 toonShade(vec3 color) {
    float intensity = toonLuminance(color);
    float level = toonStep(intensity);
    return color * (level / max(intensity, 1e-5));
}

vec3 toonShadeLighting(vec3 lighting) {
    float intensity = toonLuminance(lighting);
    float level = toonStep(intensity);
    return lighting * (level / max(intensity, 1e-5));
}

vec3 toonShadeDiffuseSpecular(vec3 diffuse, vec3 specular) {
    float intensity = toonLuminance(diffuse);
    float level = toonStep(intensity);
    return diffuse * (level / max(intensity, 1e-5)) + specular;
}

#endif// TOON_SHADING_GLSL