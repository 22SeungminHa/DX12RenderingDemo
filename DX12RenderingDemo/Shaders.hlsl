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
    float4 gBaseColor;

    float gAlpha;
    float gFresnelPower;
    float gSpecularStrength;
    float gMaterialPad0;
};

Texture2D gDiffuseMap : register(t0);
Texture2D gNormalMap : register(t1);
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
    float4 baseColor = texColor * input.color * gBaseColor;
    
    float3 normalW = normalize(input.normalW);
    float3 tangentW = normalize(input.tangentW);

    // tangent가 normal과 완전히 직교하지 않을 수 있어서 보정
    tangentW = normalize(tangentW - dot(tangentW, normalW) * normalW);

    float3 bitangentW = normalize(cross(normalW, tangentW));

    float2 normalXY = gNormalMap.Sample(gSampler, input.texCoord).rg;
    normalXY = normalXY * 2.0f - 1.0f;

    // BC5 Normal Map은 보통 XY만 저장하므로 Z는 복원
    float normalZ = sqrt(saturate(1.0f - dot(normalXY, normalXY)));

    float3 normalT = normalize(float3(normalXY.x, -normalXY.y, normalZ));
    
    float3x3 TBN = float3x3(tangentW, bitangentW, normalW);
    normalW = normalize(mul(normalT, TBN));
    
    float3 lightDir = normalize(-gLightDir);
    float3 viewDir = normalize(gEyePosW - input.positionW);

    float ndotl = saturate(dot(normalW, lightDir));

    float3 ambient = baseColor.rgb * gAmbientColor.rgb;
    float3 diffuse = baseColor.rgb * gLightColor.rgb * ndotl;

    float3 halfDir = normalize(lightDir + viewDir);
    float specFactor = pow(saturate(dot(normalW, halfDir)), gSpecularPower);
    float3 specular = gLightColor.rgb * specFactor * gSpecularStrength;

    float3 finalColor = ambient + diffuse + specular;
    
    return float4(finalColor, baseColor.a * gAlpha);
}

float4 PSGlass(VS_OUTPUT input) : SV_TARGET
{
    float4 texColor = gDiffuseMap.Sample(gSampler, input.texCoord);
    float4 baseColor = texColor * input.color * gBaseColor;

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

    float ndotl = saturate(dot(normalW, lightDir));

    float3 ambient = baseColor.rgb * gAmbientColor.rgb * 0.08f;
    float3 diffuse = baseColor.rgb * gLightColor.rgb * ndotl * 0.05f;

    // 날카로운 specular
    float3 halfDir = normalize(lightDir + viewDir);
    float specFactor = pow(saturate(dot(normalW, halfDir)), gSpecularPower);
    specFactor = pow(specFactor, 1.5f);
    specFactor = pow(specFactor, 1.5f);
    specFactor = pow(specFactor, 1.5f);
    specFactor = pow(specFactor, 1.5f);
    specFactor = pow(specFactor, 1.5f);

    float3 specular = gLightColor.rgb * specFactor * gSpecularStrength;

    float fresnel = pow(1.0f - saturate(dot(normalW, viewDir)), gFresnelPower);
    float3 fresnelColor = lerp(
        float3(0.02f, 0.04f, 0.06f),
        float3(1.0f, 1.0f, 1.0f),
        fresnel
    );
    
    float3 finalColor =
        ambient +
        diffuse +
        specular +
        fresnelColor * 0.65f;

    return float4(finalColor, baseColor.a * gAlpha);
}