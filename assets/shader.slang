struct VSInput {
    float3 Pos;
    float3 Normal;
    float2 UV;
};

Sampler2D textures[];

struct ShaderData {
    float4x4 Projection;
    float4x4 View;
    float4x4 Model[3];

    float4 LightPos;
    uint32_t Selected;
};

struct VSOutput {
    float4 Pos : SV_Position;
    float3 Normal;
    float3 Factor;
    float3 LightVec;
    float3 ViewVec;
    float2 UV;

    uint32_t InstanceIndex;
};

[shader("vertex")]
VSOutput main(VSInput input, uniform ShaderData *pShaderData, uint instanceIndex: SV_VulkanInstanceID)
{
    VSOutput output;
    float4x4 modelMat = pShaderData->Model[instanceIndex];
    float4 posViewSpace = mul(pShaderData->View, mul(modelMat, float4(input.Pos.xyz, 1.0)));

    output.Normal = mul((float3x3)mul(pShaderData->View, modelMat), input.Normal);
    output.UV = input.UV;
    output.Pos = mul(pShaderData->Projection, posViewSpace);
    output.Factor = (pShaderData->Selected == instanceIndex ? 3.0f : 1.0f);
    output.InstanceIndex = instanceIndex;

    // Calculate View Vectors Required for Lighting
    output.LightVec = pShaderData.LightPos.xyz - posViewSpace.xyz;
    output.ViewVec = -posViewSpace.xyz;
    return output;
}

[shader("fragment")]
float4 main(VSOutput input)
{
    float3 N = normalize(input.Normal);
    float3 V = normalize(input.ViewVec);
    float3 L = normalize(input.LightVec);
    float3 R = reflect(-L, N);

    float3 diffuse = max(dot(N, L), 0.0);
    float3 specular = pow(max(dot(V, R), 0.0), 16.0) * 0.75;

    // Sample From Texture
    float3 color = textures[NonUniformResourceIndex(input.InstanceIndex)].Sample(input.UV).rgb * input.Factor;
    return float4(diffuse * color + specular, 1.0);
}