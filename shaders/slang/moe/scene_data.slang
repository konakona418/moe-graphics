import light;
import material;
import common;

struct SceneData {
    float4x4 view;
    float4x4 projection;
    float4x4 viewProjection;

    float4x4 invView;
    float4x4 invProjection;
    float4x4 invViewProjection;

    float4 cameraPosition;

    float4 ambientColor;
    float4 sunlightDirection;
    float4 sunlightColor;

    MaterialBuffer materialBuffer;
    LightBuffer lightBuffer;
    uint numLights;

    ImageId skyboxId;

    // use csm
    ImageId shadowMapId;
    float4x4 shadowMapLightTransform;
    float4 shadowMapCascadeSplits;
}

typedef Ptr<SceneData, Access.Read> SceneDataPtr;
