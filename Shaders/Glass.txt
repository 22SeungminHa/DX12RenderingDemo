#include "Common.hlsli"

struct GLASS_ACCUM_OUTPUT
{
    float4 accumColor : SV_Target0;
    float revealage : SV_Target1;
};

float4 EvaluateGlass(VS_OUTPUT input)
{
    float4 texColor = gDiffuseMap.Sample(gSampler, input.texCoord);
    float4 baseColor = texColor * input.color * gBaseColorTint;

    float3 normalW = normalize(input.normalW);
    float3 tangentW = normalize(input.tangentW);

    tangentW = normalize(tangentW - dot(tangentW, normalW) * normalW);
    float3 bitangentW = normalize(cross(normalW, tangentW));

    float2 normalXY = gNormalMap.Sample(gSampler, input.texCoord).rg;
    normalXY = normalXY * 2.0f - 1.0f;
    
    float normalZ = sqrt(saturate(1.0f - dot(normalXY, normalXY)));
    float3 normalT = normalize(float3(normalXY.x, -normalXY.y, normalZ));
    
    float3x3 TBN = float3x3(tangentW, bitangentW, normalW);
    normalW = normalize(mul(normalT, TBN));

    float3 lightDir = normalize(-gLightDir);
    float3 viewDir = normalize(gEyePosW - input.positionW);
    float3 halfDir = normalize(lightDir + viewDir);

    float ndotl = saturate(dot(normalW, lightDir));
    float ndoth = saturate(dot(normalW, halfDir));
    float ndotv = saturate(dot(normalW, viewDir));

    float3 ambient = baseColor.rgb * gAmbientColor.rgb * 0.12f;
    float3 diffuse = baseColor.rgb * gLightColor.rgb * ndotl * 0.06f;

    float broadSpec = pow(ndoth, 32.0f) * 0.18f;
    float sharpSpec = pow(ndoth, 256.0f) * 1.15f;

    float3 specular = float3(1.0f, 1.0f, 1.0f) * (broadSpec + sharpSpec) * gSpecularStrength;

    float fresnel = pow(1.0f - ndotv, max(gFresnelPower, 1.0f));
    fresnel = smoothstep(0.0f, 1.0f, fresnel);

    float3 fresnelColor = float3(0.65f, 0.85f, 1.0f) * fresnel * 0.45f;

    float glassAlpha = saturate(baseColor.a * gAlpha);

    float2 sceneSize;
    gRefractionSceneMap.GetDimensions(sceneSize.x, sceneSize.y);

    float2 screenUV = input.position.xy / sceneSize;

    float2 distortion = normalT.xy * gRefractionStrength;
    float2 refractedUV = saturate(screenUV + distortion);

    float3 refractionColor = gRefractionSceneMap.Sample(gSampler, refractedUV).rgb;

    float3 reflectDir = reflect(-viewDir, normalW);
    float3 reflectionColor = gSkyboxMap.Sample(gSampler, reflectDir).rgb;
    float reflectionStrength = saturate(gReflectionStrength) * saturate(0.35f + fresnel * 0.65f);

    float3 glassTint = lerp(float3(1.0f, 1.0f, 1.0f), baseColor.rgb, glassAlpha);

    float3 finalColor = refractionColor * glassTint + specular + fresnelColor + reflectionColor * reflectionStrength;

    return float4(finalColor, glassAlpha);
}

GLASS_ACCUM_OUTPUT PSGlassAccumulation(VS_OUTPUT input)
{
    GLASS_ACCUM_OUTPUT output;

    float4 glass = EvaluateGlass(input);
    float alpha = saturate(glass.a);

    output.accumColor = float4(glass.rgb * alpha, alpha);
    output.revealage = alpha;

    return output;
}

float4 PSGlass(VS_OUTPUT input) : SV_TARGET
{
    float4 glass = EvaluateGlass(input);
    float alpha = saturate(glass.a);

    return float4(glass.rgb * alpha, alpha);
}