import light;
import pbr_forward;

static const float DISTANCE_ATTENUATION_POWER = 2.0;

// ! fixme: use a more smooth falloff
float calculateDistanceAttenuation(float dist, float range) {
    float d = clamp(1.0 - pow((dist / range), DISTANCE_ATTENUATION_POWER), 0.0, 1.0);
    //return d / (dist * dist);
    return d;
}

// in spec: attenuation = max( min( 1.0 - ( current_distance / range )^4, 1 ), 0 ) / current_distance^2

float calculateAngularAttenuation(
        float3 lightDir, float3 l,
        float2 spotParams) {
    // ! fixme: no idea whether this is correct, double check
    // ! but it seems to work anyway
    // l: direction from fragment to light
    // lightVec : direction from light to fragment (should be normalized)
    float3 lightVec = -l;
    float cd = dot(lightDir, lightVec);
    // spotParams : (lightAngleScale, lightAngleOffset)
    float angularAttenuation = clamp(cd * spotParams.x + spotParams.y, 0.0, 1.0);
    angularAttenuation *= angularAttenuation;

    return angularAttenuation;
}

float calculateAttenuation(float3 pos, float3 l, Light light) {
    float dist = length(light.position - pos);
    float atten = calculateDistanceAttenuation(dist, light.radius);
    if (light.type == LightType.Spot) {
        atten *= calculateAngularAttenuation(light.direction, l, light.spotParams.xy);
    }
    return atten;
}

float3 calculateLight(Light light, float3 fragPos, float3 n, float3 v, float3 l,
                      float3 diffuseColor, float roughness, float metallic, float3 f0, float occlusion) {
    float3 h = normalize(v + l);
    float NoL = clamp(dot(n, l), 0.0, 1.0);

    float atten = 1.0;
    // Point lights - attenuation by distance
    // Directional lights - no attenuation
    // Spot lights - attenuation by angle and distance
    if (light.type != LightType.Directional) {
        atten = calculateAttenuation(fragPos, l, light);
    }

    float3 fr = pbrBRDF(
            diffuseColor, roughness, metallic, f0,
            n, v, l, h, NoL);

    return (fr * light.color) * (light.intensity * atten * NoL * occlusion);
}
