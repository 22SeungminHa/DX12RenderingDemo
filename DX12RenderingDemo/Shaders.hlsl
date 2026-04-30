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

struct VS_INPUT
{
    float3 position : POSITION;
    float4 color : COLOR;
    float3 normal : NORMAL;
};

struct VS_OUTPUT
{
    float4 position : SV_POSITION;
    float3 positionW : POSITION;
    float4 color : COLOR;
    float3 normalW : NORMAL;
};

VS_OUTPUT VSLit(VS_INPUT input)
{
    VS_OUTPUT output;

    float4 posW = mul(float4(input.position, 1.0f), gmtxWorld);
    float4 posV = mul(posW, gmtxView);

    output.position = mul(posV, gmtxProjection);
    output.positionW = posW.xyz;
    output.color = input.color;

    output.normalW = mul(input.normal, (float3x3) gmtxWorldInvTranspose);
    output.normalW = normalize(output.normalW);

    return output;
}

float4 PSLit(VS_OUTPUT input) : SV_TARGET
{
    float3 normalW = normalize(input.normalW);

    float3 lightDir = normalize(-gLightDir);
    float3 viewDir = normalize(gEyePosW - input.positionW);

    float ndotl = saturate(dot(normalW, lightDir));

    float3 ambient = input.color.rgb * gAmbientColor.rgb;

    float3 diffuse = input.color.rgb * gLightColor.rgb * ndotl;

    float3 halfDir = normalize(lightDir + viewDir);
    float specFactor = pow(saturate(dot(normalW, halfDir)), gSpecularPower);
    float3 specular = gLightColor.rgb * specFactor * gSpecularStrength;

    float3 finalColor = ambient + diffuse + specular;

    return float4(finalColor, input.color.a);
}