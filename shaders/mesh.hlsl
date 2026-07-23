struct SceneFrameData
{
    float4 row_0;
    float4 row_1;
    float4 row_2;
    float4 row_3;
    float4 light_direction_intensity;
    float4 view_direction_exposure;
};

[[vk::binding(0, 0)]]
ConstantBuffer<SceneFrameData> frame_data;

[[vk::binding(0, 1)]]
Texture2D<float4> aperture_surface;

[[vk::binding(1, 1)]]
SamplerState aperture_sampler;

struct VertexInput
{
    [[vk::location(0)]]
    float3 position : POSITION;

    [[vk::location(1)]]
    float3 normal : NORMAL;

    [[vk::location(2)]]
    float3 color : COLOR0;
};

struct VertexOutput
{
    float4 position : SV_Position;

    [[vk::location(0)]]
    float3 normal : NORMAL;

    [[vk::location(1)]]
    float3 color : COLOR0;

    [[vk::location(2)]]
    float2 material_uv : TEXCOORD0;
};

VertexOutput vs_main(VertexInput input)
{
    const float4 local_position =
        float4(input.position, 1.0);

    VertexOutput output;

    output.position = float4(
        dot(frame_data.row_0, local_position),
        dot(frame_data.row_1, local_position),
        dot(frame_data.row_2, local_position),
        dot(frame_data.row_3, local_position));

    output.normal = input.normal;
    output.color = input.color;

    output.material_uv =
        input.position.xy *
            0.28 +
        0.5;

    return output;
}

float4 ps_main(VertexOutput input) : SV_Target0
{
    const float3 normal =
        normalize(input.normal);

    const float3 light_direction =
        normalize(
            frame_data
                .light_direction_intensity
                .xyz);

    const float3 view_direction =
        normalize(
            frame_data
                .view_direction_exposure
                .xyz);

    const float3 half_direction =
        normalize(
            light_direction +
            view_direction);

    const float4 surface_sample =
        aperture_surface.Sample(
            aperture_sampler,
            saturate(input.material_uv));

    const float roughness =
        saturate(surface_sample.a);

    const float3 material_response =
        0.62 +
        surface_sample.rgb *
            2.2;

    const float3 albedo =
        input.color *
        material_response;

    const float diffuse_factor =
        saturate(
            dot(
                normal,
                light_direction));

    const float hemisphere =
        lerp(
            0.075,
            0.19,
            saturate(
                normal.y * 0.5 +
                0.5));

    const float light_intensity =
        frame_data
            .light_direction_intensity
            .w;

    const float specular_power =
        lerp(
            72.0,
            16.0,
            roughness);

    const float specular_factor =
        pow(
            saturate(
                dot(
                    normal,
                    half_direction)),
            specular_power);

    const float specular_weight =
        lerp(
            0.30,
            0.08,
            roughness);

    const float rim_factor =
        pow(
            1.0 -
                saturate(
                    dot(
                        normal,
                        view_direction)),
            3.0);

    const float3 diffuse =
        albedo *
        (
            hemisphere +
            diffuse_factor *
                light_intensity);

    const float3 specular =
        lerp(
            float3(
                0.34,
                0.36,
                0.38),
            albedo,
            roughness * 0.35) *
        specular_factor *
        specular_weight *
        light_intensity;

    const float3 rim =
        albedo *
        rim_factor *
        0.09;

    const float3 linear_color =
        diffuse +
        specular +
        rim;

    const float exposure =
        frame_data
            .view_direction_exposure
            .w;

    const float3 mapped_color =
        1.0 -
        exp(
            -linear_color *
            exposure);

    return float4(
        mapped_color,
        1.0);
}
