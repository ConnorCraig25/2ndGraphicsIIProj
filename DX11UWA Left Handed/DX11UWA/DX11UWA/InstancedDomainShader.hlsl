cbuffer ModelViewProjectionConstantBufferInstanced : register(b0)
{
	matrix model[3];
	matrix view;
	matrix projection;
};

struct DS_OUTPUT
{
	float4 pos : SV_POSITION;
	float3 uv : UV;
	float3 normal : NORMAL;
	float3 world_pos : WORLDPOS;
	// TODO: change/add other stuff
};

// Output control point
struct HS_CONTROL_POINT_OUTPUT
{
	float4 pos : SV_POSITION;
	float3 uv : UV;
	float3 normal : NORMAL;
};

// Output patch constant data.
struct HS_CONSTANT_DATA_OUTPUT
{
	float EdgeTessFactor[3]			: SV_TessFactor; // e.g. would be [4] for a quad domain
	float InsideTessFactor			: SV_InsideTessFactor; // e.g. would be Inside[2] for a quad domain
	// TODO: change/add other stuff
};

#define NUM_CONTROL_POINTS 3

[domain("tri")]
DS_OUTPUT main(
	HS_CONSTANT_DATA_OUTPUT input,
	float3 domain : SV_DomainLocation,
	const OutputPatch<HS_CONTROL_POINT_OUTPUT, NUM_CONTROL_POINTS> patch)
{
	DS_OUTPUT Output;

	Output.pos = float4(patch[0].pos.xyz*domain.x + patch[1].pos.xyz*domain.y + patch[2].pos.xyx*domain.z, 1);
	Output.uv = float3(patch[0].uv*domain.x + patch[1].uv*domain.y + patch[2].uv*domain.z);
	Output.normal = float3(patch[0].normal*domain.x + patch[1].normal*domain.y + patch[2].normal*domain.z);

	Output.pos = mul(Output.pos, model[id]);
	Output.world_pos = Output.pos.xyz;
	Output.pos = mul(Output.pos, view);
	Output.pos = mul(Output.pos, projection);
	Output.normal = mul(Output.normal, model[id]);

	return Output;
}
