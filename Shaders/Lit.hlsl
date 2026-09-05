#include "Common.hlsli"

VS_OUTPUT VSLit(VS_INPUT input)
{
    VS_OUTPUT output;

    float4 posW = mul(float4(input.position, 1.0f), gmtxWorld);
    float4 posV = mul(posW, gmtxView);

    output.position = mul(posV, gmtxProjection);
    output.positionW = posW.xyz;
    output.color = input.color;
    output.texCoord = input.texCoord;

    output.normalW = mul(input.normal, (float3x3) gmtxWorldInvTranspose);
    output.normalW = normalize(output.normalW);
    
    output.tangentW = mul(input.tangent, (float3x3) gmtxWorld);
    output.tangentW = normalize(output.tangentW);

    return output;
}

float4 PSLit(VS_OUTPUT input) : SV_TARGET
{
    float2 tiledUV = input.texCoord * gTiling;

    float4 texColor = gDiffuseMap.Sample(gSampler, tiledUV);
    float4 baseColor = texColor * input.color * gBaseColorTint;

    float3 normalW = normalize(input.normalW);
    float3 tangentW = normalize(input.tangentW);

    tangentW = normalize(tangentW - dot(tangentW, normalW) * normalW);
    float3 bitangentW = normalize(cross(normalW, tangentW));

    float2 normalXY = gNormalMap.Sample(gSampler, tiledUV).rg;
    normalXY = normalXY * 2.0f - 1.0f;

    float normalZ = sqrt(saturate(1.0f - dot(normalXY, normalXY)));
    float3 normalT = normalize(float3(normalXY.x, -normalXY.y, normalZ));

    float3x3 TBN = float3x3(tangentW, bitangentW, normalW);
    normalW = normalize(mul(normalT, TBN));

    float3 viewDir = normalize(gEyePosW - input.positionW);

    float3 ambient = baseColor.rgb * gAmbientColor.rgb * 0.45f;

    float3 diffuse = 0.0f;
    float3 specular = 0.0f;

    for (uint i = 0; i < gDirectionalLightCount; ++i)
    {
        DirectionalLight light = gDirectionalLights[i];

        float3 lightDir = normalize(-light.direction);
        float3 halfDir = normalize(lightDir + viewDir);

        float ndotlRaw = dot(normalW, lightDir);
        float wrappedNdotL = saturate(ndotlRaw * 0.5f + 0.5f);
        float diffuseFactor = lerp(0.05f, 1.0f, wrappedNdotL * wrappedNdotL);
        diffuse += baseColor.rgb * light.color.rgb * diffuseFactor * 0.85f;

        float specFactor = pow(saturate(dot(normalW, halfDir)), gSpecularPower);

        specular += float3(1.0f, 1.0f, 1.0f) * light.color.rgb * specFactor * gSpecularStrength * 0.35f;
    }

    float ndotv = saturate(dot(normalW, viewDir));

    float fresnel = pow(1.0f - ndotv, max(gFresnelPower, 1.0f));

    float3 reflectDir = reflect(-viewDir, normalW);
    float3 reflectionColor = gSkyboxMap.Sample(gSampler, reflectDir).rgb;

    float reflectionAmount = saturate(gReflectionStrength) * lerp(0.65f, 1.0f, fresnel);

    float3 surfaceColor = ambient + diffuse + specular;

    float3 finalColor = lerp(surfaceColor, reflectionColor, reflectionAmount);

    float fogFactor = CalculateFogFactor(input.positionW);
    float verticalFogBoost = CalculateVerticalFogBoost(input.position.y);

    fogFactor = saturate(fogFactor * verticalFogBoost);

    float3 fogColor = CalculateFogColor(input.position.y);
    
    finalColor = lerp(finalColor, fogColor, fogFactor);
    
    return float4(finalColor, baseColor.a * gAlpha);
}
