// A constant buffer that stores the three basic column-major matrices for composing geometry.
cbuffer ModelViewProjectionConstantBufferInstanced : register(b0)
{
	matrix model[3];
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

struct GSOutput
{
	float4 pos : SV_POSITION;
	float3 uv : UV;
	float3 normal : NORMAL;
	float3 world_pos : WORLDPOS;
};

// Simple shader to do vertex processing on the GPU.
//SV_InstancedID
GSOutput main(VertexShaderInput input, uint id : SV_InstanceID)
{
	GSOutput output;
	float4 pos = float4(input.pos, 1.0f);
	float4 normals = float4(input.normal, 0.0f);

	// Transform the vertex position into projected space.
	pos = mul(pos, model[id]);
	output.world_pos = pos.xyz;
	pos = mul(pos, view);
	pos = mul(pos, projection);
	output.pos = pos;

	// Pass the color through without modification.
	output.uv = input.uv;
	output.normal = mul(normals, model[id]);
	return output;
}
