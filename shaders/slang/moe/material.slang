struct Material {
    float4 baseColor;
    float4 metallicRoughnessEmissive;
    float4 emissiveColor;

    uint diffuseImageIndex;
    uint normalImageIndex;
    uint metallicRoughnessImageIndex;
    uint emissiveImageIndex;
}

typedef Ptr<Material, Access.Read> MaterialBuffer;
