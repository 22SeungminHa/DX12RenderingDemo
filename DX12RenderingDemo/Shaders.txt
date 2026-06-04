cbuffer cbGameObjectInfo : register(b0)
{
    matrix gmtxWorld;
    matrix gmtxWorldInvTranspose;
};

cbuffer cbCameraInfo : register(b1)
{
    matrix gmtxView;
    matrix gmtxProjection;

    float3 gEyePosW;
    float gPad0;

    float3 gLightDir;
    float gPad1;

    float4 gLightColor;
    float4 gAmbientColor;

    float gSpecularPower;
    float2 gPad2;
};

cbuffer cbMaterialInfo : register(b2)
{
    float4 gBaseColorTint;

    float gAlpha;
    float gFresnelPower;
    float gSpecularStrength;
    float gReflectionStrength;
};

Texture2D gDiffuseMap : register(t0);
Texture2D gNormalMap : register(t1);
TextureCube gSkyboxMap : register(t2);

SamplerState gSampler : register(s0);

struct VS_INPUT
{
    float3 position : POSITION;
    float4 color : COLOR;
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
    float2 texCoord : TEXCOORD;
};

struct VS_OUTPUT
{
    float4 position : SV_POSITION;
    float3 positionW : POSITION;
    float4 color : COLOR;
    float3 normalW : NORMAL;
    float3 tangentW : TANGENT;
    float2 texCoord : TEXCOORD;
};

struct VS_SKYBOX_INPUT
{
    float3 position : POSITION;
};

struct VS_SKYBOX_OUTPUT
{
    float4 position : SV_POSITION;
    float3 direction : TEXCOORD;
};

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

    float ndotlRaw = dot(normalW, lightDir);
    float wrappedNdotL = saturate(ndotlRaw * 0.5f + 0.5f);
    float diffuseFactor = lerp(0.2f, 1.0f, wrappedNdotL);

    float3 ambient = baseColor.rgb * gAmbientColor.rgb * 0.45f;
    float3 diffuse = baseColor.rgb * gLightColor.rgb * diffuseFactor * 0.85f;

    float specFactor = pow(saturate(dot(normalW, halfDir)), gSpecularPower);

    float3 specular =
        float3(1.0f, 1.0f, 1.0f) *
        gLightColor.rgb *
        specFactor *
        gSpecularStrength *
        0.35f;

    float3 finalColor = ambient + diffuse + specular;

    return float4(finalColor, baseColor.a * gAlpha);
}

float4 PSGlass(VS_OUTPUT input) : SV_TARGET
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

    float3 specular =
        float3(1.0f, 1.0f, 1.0f) *
        (broadSpec + sharpSpec) *
        gSpecularStrength;

    float fresnel = pow(1.0f - ndotv, max(gFresnelPower, 1.0f));
    fresnel = smoothstep(0.0f, 1.0f, fresnel);

    float3 fresnelColor = float3(0.65f, 0.85f, 1.0f) * fresnel * 0.45f;

    float glassAlpha = saturate(baseColor.a * gAlpha);

    float3 transparentBody = ambient + diffuse;
    
    float3 reflectDir = reflect(-viewDir, normalW);
    float3 reflectionColor = gSkyboxMap.Sample(gSampler, reflectDir).rgb;
    float reflectionStrength = saturate(gReflectionStrength) * saturate(0.35f + fresnel * 0.65f);
    
    float3 finalColor =
        transparentBody * lerp(0.3f, 0.75f, glassAlpha) +
        specular +
        fresnelColor +
        reflectionColor * reflectionStrength;
    
    //float3 finalColor =
    //    transparentBody * lerp(0.3f, 0.75f, glassAlpha) +
    //    specular +
    //    fresnelColor;

    return float4(finalColor, glassAlpha);
}

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

Texture2D gSceneColorMap : register(t0);

struct VS_FULLSCREEN_OUTPUT
{
    float4 position : SV_POSITION;
    float2 texCoord : TEXCOORD;
};

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
    float3 color = gSceneColorMap.Sample(gSampler, input.texCoord).rgb;

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