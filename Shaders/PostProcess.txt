#include "Common.hlsli"

Texture2D gSceneColorMap : register(t0);
Texture2D gBloomMap : register(t3);
Texture2D gGlassAccumMap : register(t5);
Texture2D gGlassRevealageMap : register(t6);

VS_FULLSCREEN_OUTPUT VSFullscreen(uint vertexID : SV_VertexID)
{
    VS_FULLSCREEN_OUTPUT output;

    float2 positions[3] =
    {
        float2(-1.0f, -1.0f),
        float2(-1.0f, 3.0f),
        float2(3.0f, -1.0f)
    };

    float2 texCoords[3] =
    {
        float2(0.0f, 1.0f),
        float2(0.0f, -1.0f),
        float2(2.0f, 1.0f)
    };

    output.position = float4(positions[vertexID], 0.0f, 1.0f);
    output.texCoord = texCoords[vertexID];

    return output;
}

float4 PSCopy(VS_FULLSCREEN_OUTPUT input) : SV_TARGET
{
    float3 sceneColor = gSceneColorMap.Sample(gSampler, input.texCoord).rgb;
    float3 bloomColor = gBloomMap.Sample(gSampler, input.texCoord).rgb;

    float bloomStrength = 0.65f;

    float3 color = sceneColor + bloomColor * bloomStrength;

    float exposure = 1.15f;
    color *= exposure;

    float contrast = 1.08f;
    color = (color - 0.5f) * contrast + 0.5f;

    float saturation = 1.1f;
    float luminance = dot(color, float3(0.2126f, 0.7152f, 0.0722f));
    color = lerp(float3(luminance, luminance, luminance), color, saturation);

    color = saturate(color);

    float gamma = 2.2f;
    color = pow(color, 1.0f / gamma);

    return float4(color, 1.0f);
}

float4 PSBrightPass(VS_FULLSCREEN_OUTPUT input) : SV_TARGET
{
    float3 color = gSceneColorMap.Sample(gSampler, input.texCoord).rgb;

    float brightness = dot(color, float3(0.2126f, 0.7152f, 0.0722f));
    float threshold = 1.0f;

    float brightFactor = saturate((brightness - threshold) / max(brightness, 0.0001f));

    return float4(color * brightFactor, 1.0f);
}

float4 PSBlurHorizontal(VS_FULLSCREEN_OUTPUT input) : SV_TARGET
{
    float2 texelSize;
    gSceneColorMap.GetDimensions(texelSize.x, texelSize.y);
    texelSize = 1.0f / texelSize;

    float2 uv = input.texCoord;

    float3 color = float3(0.0f, 0.0f, 0.0f);

    color += gSceneColorMap.Sample(gSampler, uv + float2(-4.0f * texelSize.x, 0.0f)).rgb * 0.05f;
    color += gSceneColorMap.Sample(gSampler, uv + float2(-3.0f * texelSize.x, 0.0f)).rgb * 0.09f;
    color += gSceneColorMap.Sample(gSampler, uv + float2(-2.0f * texelSize.x, 0.0f)).rgb * 0.12f;
    color += gSceneColorMap.Sample(gSampler, uv + float2(-1.0f * texelSize.x, 0.0f)).rgb * 0.15f;
    color += gSceneColorMap.Sample(gSampler, uv).rgb * 0.18f;
    color += gSceneColorMap.Sample(gSampler, uv + float2(1.0f * texelSize.x, 0.0f)).rgb * 0.15f;
    color += gSceneColorMap.Sample(gSampler, uv + float2(2.0f * texelSize.x, 0.0f)).rgb * 0.12f;
    color += gSceneColorMap.Sample(gSampler, uv + float2(3.0f * texelSize.x, 0.0f)).rgb * 0.09f;
    color += gSceneColorMap.Sample(gSampler, uv + float2(4.0f * texelSize.x, 0.0f)).rgb * 0.05f;

    return float4(color, 1.0f);
}

float4 PSBlurVertical(VS_FULLSCREEN_OUTPUT input) : SV_TARGET
{
    float2 texelSize;
    gSceneColorMap.GetDimensions(texelSize.x, texelSize.y);
    texelSize = 1.0f / texelSize;

    float2 uv = input.texCoord;

    float3 color = float3(0.0f, 0.0f, 0.0f);

    color += gSceneColorMap.Sample(gSampler, uv + float2(0.0f, -4.0f * texelSize.y)).rgb * 0.05f;
    color += gSceneColorMap.Sample(gSampler, uv + float2(0.0f, -3.0f * texelSize.y)).rgb * 0.09f;
    color += gSceneColorMap.Sample(gSampler, uv + float2(0.0f, -2.0f * texelSize.y)).rgb * 0.12f;
    color += gSceneColorMap.Sample(gSampler, uv + float2(0.0f, -1.0f * texelSize.y)).rgb * 0.15f;
    color += gSceneColorMap.Sample(gSampler, uv).rgb * 0.18f;
    color += gSceneColorMap.Sample(gSampler, uv + float2(0.0f, 1.0f * texelSize.y)).rgb * 0.15f;
    color += gSceneColorMap.Sample(gSampler, uv + float2(0.0f, 2.0f * texelSize.y)).rgb * 0.12f;
    color += gSceneColorMap.Sample(gSampler, uv + float2(0.0f, 3.0f * texelSize.y)).rgb * 0.09f;
    color += gSceneColorMap.Sample(gSampler, uv + float2(0.0f, 4.0f * texelSize.y)).rgb * 0.05f;

    return float4(color, 1.0f);
}

float4 PSGlassComposite(VS_FULLSCREEN_OUTPUT input) : SV_TARGET
{
    float3 background = gRefractionSceneMap.Sample(gSampler, input.texCoord).rgb;
    float4 accum = gGlassAccumMap.Sample(gSampler, input.texCoord);
    float revealage = saturate(gGlassRevealageMap.Sample(gSampler, input.texCoord).r);
    float3 glassColor = accum.rgb / max(accum.a, 0.0001f);
    float coverage = 1.0f - revealage;
    float3 finalColor = lerp(background, glassColor, coverage);

    return float4(finalColor, 1.0f);
}