enum LightType : uint {
    Point = 0,
    Directional = 1,
    Spot = 2,
}

struct Light {
    float3 position;
    LightType type;

    float3 color;
    float intensity;

    float3 direction;
    float radius;

    float3 spotParams;// x: lightAngleScale, y: lightAngleOffset, z: unused
    uint available;
}

typedef Ptr<Light, Access.Read> LightBuffer;
