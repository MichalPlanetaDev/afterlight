struct VertexOutput
{
    float4 position : SV_Position;
    float3 color : COLOR0;
};

VertexOutput vs_main(uint vertex_id : SV_VertexID)
{
    static const float2 positions[3] =
    {
        float2(0.0, -0.68),
        float2(0.66, 0.52),
        float2(-0.66, 0.52)
    };

    static const float3 colors[3] =
    {
        float3(1.0, 0.32, 0.08),
        float3(0.12, 0.72, 1.0),
        float3(0.74, 0.18, 1.0)
    };

    VertexOutput output;
    output.position = float4(positions[vertex_id], 0.0, 1.0);
    output.color = colors[vertex_id];

    return output;
}

float4 ps_main(VertexOutput input) : SV_Target0
{
    return float4(input.color, 1.0);
}
