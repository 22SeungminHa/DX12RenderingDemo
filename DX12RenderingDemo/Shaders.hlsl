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
    float gSpecularStrength;
    float2 gPad2;
};

Texture2D gDiffuseMap : register(t0);
SamplerState gSampler : register(s0);

struct VS_INPUT
{
    float3 position : POSITION;
    float4 color : COLOR;
    float3 normal : NORMAL;
    float2 texCoord : TEXCOORD;
};

struct VS_OUTPUT
{
    float4 position : SV_POSITION;
    float3 positionW : POSITION;
    float4 color : COLOR;
    float3 normalW : NORMAL;
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

    return output;
}

float4 PSLit(VS_OUTPUT input) : SV_TARGET
{
    float4 texColor = gDiffuseMap.Sample(gSampler, input.texCoord);
    float4 baseColor = texColor * input.color;

    float3 normalW = normalize(input.normalW);

    float3 lightDir = normalize(-gLightDir);
    float3 viewDir = normalize(gEyePosW - input.positionW);

    float ndotl = saturate(dot(normalW, lightDir));

    float3 ambient = baseColor.rgb * gAmbientColor.rgb;
    float3 diffuse = baseColor.rgb * gLightColor.rgb * ndotl;

    float3 halfDir = normalize(lightDir + viewDir);
    float specFactor = pow(saturate(dot(normalW, halfDir)), gSpecularPower);
    float3 specular = gLightColor.rgb * specFactor * gSpecularStrength;

    float3 finalColor = ambient + diffuse + specular;

    return float4(finalColor, baseColor.a);
}