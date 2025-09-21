#ifndef MOE_PBR_LIGHTING_GLSL
#define MOE_PBR_LIGHTING_GLSL

#include "light.glsl"
#include "pbr_forward.glsl"

#define DISTANCE_ATTENUATION_POWER 2.0

// ! fixme: use a more smooth falloff
float calculateDistanceAttenuation(float dist, float range) {
    float d = clamp(1.0 - pow((dist / range), DISTANCE_ATTENUATION_POWER), 0.0, 1.0);
    //return d / (dist * dist);
    return d;
}

/*float calculateAngularAttenuation(
        vec3 lightDir, vec3 l,
        vec2 scaleOffset) {
    float cd = dot(lightDir, l);
    float angularAttenuation = clamp(cd * scaleOffset.x + scaleOffset.y, 0.0, 1.0);
    angularAttenuation *= angularAttenuation;
    return angularAttenuation;
}*/

// todo: spot light
float calculateAttenuation(vec3 pos, vec3 l, Light light) {
    float dist = length(light.position - pos);
    float atten = calculateDistanceAttenuation(dist, light.radius);
    /*if (light.type == LIGHT_TYPE_SPOT) {
        atten *= calculateAngularAttenuation(light.direction, l, light.scaleOffset);
    }*/
    return atten;
}

vec3 calculateLight(Light light, vec3 fragPos, vec3 n, vec3 v, vec3 l,
                    vec3 diffuseColor, float roughness, float metallic, vec3 f0, float occlusion) {
    vec3 h = normalize(v + l);
    float NoL = clamp(dot(n, l), 0.0, 1.0);

    float atten = 1.0;
    if (light.type != LIGHT_TYPE_DIRECTIONAL) {
        atten = calculateAttenuation(fragPos, l, light);
    }

    vec3 fr = pbrBRDF(
            diffuseColor, roughness, metallic, f0,
            n, v, l, h, NoL);

    return (fr * light.color) * (light.intensity * atten * NoL * occlusion);
}

#endif