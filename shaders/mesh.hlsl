struct TransformData
{
    float4 row_0;
    float4 row_1;
    float4 row_2;
    float4 row_3;
};

[[vk::push_constant]]
ConstantBuffer<TransformData> transform_data;

struct VertexInput
{
    [[vk::location(0)]]
    float3 position : POSITION;

    [[vk::location(1)]]
    float3 color : COLOR0;
};

struct VertexOutput
{
    float4 position : SV_Position;

    [[vk::location(0)]]
    float3 color : COLOR0;
};

VertexOutput vs_main(VertexInput input)
{
    const float4 local_position =
        float4(input.position, 1.0);

    VertexOutput output;

    output.position = float4(
        dot(transform_data.row_0, local_position),
        dot(transform_data.row_1, local_position),
        dot(transform_data.row_2, local_position),
        dot(transform_data.row_3, local_position));

    output.color = input.color;

    return output;
}

float4 ps_main(VertexOutput input) : SV_Target0
{
    const float intensity =
        0.82 + 0.18 * max(
            input.color.r,
            max(input.color.g, input.color.b));

    return float4(
        input.color * intensity,
        1.0);
}
