// A constant buffer that stores the three basic column-major matrices for composing geometry.
cbuffer ModelViewProjectionConstantBuffer : register(b0)
{
	matrix model;
	matrix view;
	matrix projection;
};

// Per-vertex data used as input to the vertex shader.
struct VertexShaderInput
{
	float3 pos : POSITION;
	float3 uv : UV;
	float3 normal : NORMAL;
};

struct VS_CONTROL_POINT_OUTPUT
{
	float4 pos : SV_POSITION;
	float3 uv : UV;
	float3 normal : NORMAL;

};

// Simple shader to do vertex processing on the GPU.
VS_CONTROL_POINT_OUTPUT main(VertexShaderInput input)
{
	VS_CONTROL_POINT_OUTPUT output;
	//float4 pos = float4(input.pos, 1.0f);
	//float4 normal = float4(input.normal, 0.0f);
	//
	//// Transform the vertex position into projected space.
	//pos = mul(pos, model);
	//pos = mul(pos, view);
	//pos = mul(pos, projection);
	//output.pos = pos;
	//
	//// Pass the color through without modification.
	float4 pos = float4(input.pos, 1.0f);
	output.pos = pos;
	output.uv = input.uv;
	output.normal = input.normal;
	return output;
}
