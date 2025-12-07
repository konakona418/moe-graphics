import common;

struct Material {
    float4 baseColor;
    float4 metallicRoughnessEmissive;
    float4 emissiveColor;

    ImageId diffuseImageIndex;
    ImageId normalImageIndex;
    ImageId metallicRoughnessImageIndex;
    ImageId emissiveImageIndex;
}

typedef Ptr<Material, Access.Read> MaterialBuffer;
