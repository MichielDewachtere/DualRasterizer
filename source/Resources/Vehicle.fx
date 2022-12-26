//------------------------------------------------
// Global Variables
//------------------------------------------------
float4x4 gWorldViewProj : WorldViewProjection;

Texture2D gDiffuseMap : DissufeMap;
Texture2D gNormalMap : NormalMap;
Texture2D gSpecularMap : SpecularMap;
Texture2D gGlossinessMap : GlossinessMap;

static const float3 gLightDirection = { 0.577f, -0.577f, 0.577f };
static const float gLightIntensity = 7.0f;
static const float gShininess = 25.f;

float4x4 gWorldMatrix : WORLD;
float4x4 gInvViewMatrix : VIEWINVERSE;

static const float gPi = 3.14159265f;

//------------------------------------------------
// Sampler states
//------------------------------------------------
SamplerState gSamState
{
	Filter = MIN_MAG_MIP_POINT;
	AddressU = Wrap;
	AddressV = Wrap;
};

//------------------------------------------------
// Rasterizer State
//------------------------------------------------
RasterizerState gRasterizerState
{
	CullMode = back;
	FrontCounterClockwise = false; //default
};

//------------------------------------------------
// Blend State
//------------------------------------------------
BlendState gBlendState
{
  	BlendEnable[0] = false;
};

//------------------------------------------------
// Depth Stencil State
//------------------------------------------------
DepthStencilState gDepthStencilState
{
	DepthEnable = true;
	DepthWriteMask = all;
	DepthFunc = less;
	StencilEnable = false;
};

//------------------------------------------------
// Input/Output Struct
//------------------------------------------------
struct VS_INPUT
{
	float3 Position : POSITION;
	float3 Color : COLOR;
	float2 UV : TEXCOORD;
	float3 Normal : NORMAL;
	float3 Tangent : TANGENT;
};

struct VS_OUTPUT
{
	float4 Position : SV_POSITION;
	float4 WorldPosition : WORLD_POSITION;
	float3 Color : COLOR;
	float2 UV : TEXCOORD;
	float3 Normal : NORMAL;
	float3 Tangent : TANGENT;
};

//------------------------------------------------
// Vertex Shader
//------------------------------------------------
VS_OUTPUT VS(VS_INPUT input)
{
	VS_OUTPUT output = (VS_OUTPUT)0;
	output.Position = mul(float4(input.Position, 1.0f), gWorldViewProj);
	output.WorldPosition = mul(float4(input.Position, 1.0f), gWorldMatrix);

	output.Color = input.Color;
	output.UV = input.UV;

	output.Normal = mul(normalize(input.Normal), (float3x3)gWorldMatrix);
	output.Tangent = mul(normalize(input.Tangent), (float3x3)gWorldMatrix);

	return output;
}


//------------------------------------------------
// BRDF Functions
//------------------------------------------------
float3 Lambert(const float3 cd, const float kd = 1)
{
	const float3 rho = kd * cd;

	return rho / gPi;
}
float3 Phong(const float3 ks, const float exp, const float3 l, const float3 v, const float3 n)
{
	const float3 reflectedLight = reflect(l, n);
	const float cosAlpha = saturate(dot(reflectedLight, v));

	return ks * pow(cosAlpha, exp);
}

//------------------------------------------------
// Math Functions
//------------------------------------------------
float3 TransformVector(float4x4 m, float3 v)
{
	return float3(
		m[0].x * v.x + m[1].x * v.y + m[2].x * v.z,
		m[0].y * v.x + m[1].y * v.y + m[2].y * v.z,
		m[0].z * v.x + m[1].z * v.y + m[2].z * v.z
	);
}

//------------------------------------------------
// Pixel Shader
//------------------------------------------------
float4 PS(VS_OUTPUT input) : SV_TARGET
{
	float3 viewDirection = normalize(input.WorldPosition.xyz - gInvViewMatrix[3].xyz);

	//Normal Map
	float3 normal;

	const float3 biNormal = cross(input.Normal, input.Tangent);
	const float4x4 tangentSpaceAxis = { float4(input.Tangent, 0.f), float4(biNormal, 0.f), float4(input.Normal, 0.f), float4(0,0,0,1) };

	const float3 normalColor = gNormalMap.Sample(gSamState, input.UV).xyz;
	float3 sampledNormal = 2.f * normalColor - float3( 1, 1, 1 ); // => [0, 1] to [-1, 1]

	sampledNormal = TransformVector(tangentSpaceAxis, sampledNormal);

	normal = normalize(sampledNormal);
	////////////////////////////

	// Observed Area
	const float observedArea = saturate(dot(normal, -gLightDirection));
	////////////////////

	// phong specular
	const float3 gloss = gGlossinessMap.Sample(gSamState, input.UV).xyz;
	const float exponent = gloss.x * gShininess;
	float3 specular = Phong(gSpecularMap.Sample(gSamState, input.UV).xyz, exponent, -gLightDirection, viewDirection, normal);
	////////////////////////

	float3 diffuse = Lambert(gDiffuseMap.Sample(gSamState, input.UV).xyz);

	const float3 ambient = { .025f,.025f,.025f };

	return float4(((gLightIntensity * diffuse + specular) * observedArea + ambient), 1.f);

}
//------------------------------------------------
// Technique
//------------------------------------------------
technique11 DefaultTechnique
{
	pass P0
	{
		SetRasterizerState(gRasterizerState);
		SetDepthStencilState(gDepthStencilState, 0);
		SetBlendState(gBlendState, float4(0.0f, 0.0f, 0.0f, 0.0f), 0xFFFFFFFF);
		SetVertexShader(CompileShader(vs_5_0, VS()));
		SetGeometryShader(NULL);
		SetPixelShader(CompileShader(ps_5_0, PS()));
	}
}