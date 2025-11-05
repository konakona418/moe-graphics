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

// in spec: attenuation = max( min( 1.0 - ( current_distance / range )^4, 1 ), 0 ) / current_distance^2

float calculateAngularAttenuation(
        vec3 lightDir, vec3 l,
        vec2 spotParams) {
    // ! fixme: no idea whether this is correct, double check
    // ! but it seems to work anyway
    // l: direction from fragment to light
    // lightVec : direction from light to fragment (should be normalized)
    vec3 lightVec = -l;
    float cd = dot(lightDir, lightVec);
    // spotParams : (lightAngleScale, lightAngleOffset)
    float angularAttenuation = clamp(cd * spotParams.x + spotParams.y, 0.0, 1.0);
    angularAttenuation *= angularAttenuation;

    //debugPrintfEXT("cd: %f, spotParams: %f, %f, angularAttenuation: %f\n", cd, spotParams.x, spotParams.y, angularAttenuation);

    return angularAttenuation;
}

float calculateAttenuation(vec3 pos, vec3 l, Light light) {
    float dist = length(light.position - pos);
    float atten = calculateDistanceAttenuation(dist, light.radius);
    if (light.type == LIGHT_TYPE_SPOT) {
        atten *= calculateAngularAttenuation(light.direction, l, light.spotParams.xy);
    }
    return atten;
}

vec3 calculateLight(Light light, vec3 fragPos, vec3 n, vec3 v, vec3 l,
                    vec3 diffuseColor, float roughness, float metallic, vec3 f0, float occlusion) {
    vec3 h = normalize(v + l);
    float NoL = clamp(dot(n, l), 0.0, 1.0);

    float atten = 1.0;
    // Point lights - attenuation by distance
    // Directional lights - no attenuation
    // Spot lights - attenuation by angle and distance
    if (light.type != LIGHT_TYPE_DIRECTIONAL) {
        atten = calculateAttenuation(fragPos, l, light);
    }

    vec3 fr = pbrBRDF(
            diffuseColor, roughness, metallic, f0,
            n, v, l, h, NoL);

    return (fr * light.color) * (light.intensity * atten * NoL * occlusion);
}

#endif