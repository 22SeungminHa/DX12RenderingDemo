#include "Common.hlsli"

VS_SKYBOX_OUTPUT VSSkybox(VS_SKYBOX_INPUT input)
{
    VS_SKYBOX_OUTPUT output;

    float4 posW = float4(input.position, 1.0f);

    float4x4 viewNoTranslation = gmtxView;
    viewNoTranslation._41 = 0.0f;
    viewNoTranslation._42 = 0.0f;
    viewNoTranslation._43 = 0.0f;

    float4 posV = mul(posW, viewNoTranslation);
    float4 posH = mul(posV, gmtxProjection);

    output.position = posH.xyww;
    output.direction = input.position;

    return output;
}

float4 PSSkybox(VS_SKYBOX_OUTPUT input) : SV_TARGET
{
    return gSkyboxMap.Sample(gSampler, normalize(input.direction));
}
