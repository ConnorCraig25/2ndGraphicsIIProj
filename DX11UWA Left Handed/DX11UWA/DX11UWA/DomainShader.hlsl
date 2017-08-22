cbuffer ModelViewProjectionConstantBuffer : register(b0)
{
	matrix model;
	matrix view;
	matrix projection;
};


struct DS_OUTPUT
{
	float4 pos : SV_POSITION;
	float3 uv : UV;
	float4 normal : NORMAL;
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
DS_OUTPUT main(HS_CONSTANT_DATA_OUTPUT input,float3 domain : SV_DomainLocation,
				const OutputPatch<HS_CONTROL_POINT_OUTPUT, NUM_CONTROL_POINTS> patch)
{
	DS_OUTPUT Output;

	

	float4 pos = float4(patch[0].pos.xyz*domain.x + patch[1].pos.xyz*domain.y + patch[2].pos.xyz*domain.z,1.0f);
	float3 uv = float3(patch[0].uv*domain.x + patch[1].uv*domain.y + patch[2].uv*domain.z);
	float4 normal = float4(patch[0].normal*domain.x + patch[1].normal*domain.y + patch[2].normal*domain.z,0.0f);

	pos = mul(pos, model);
	Output.world_pos = pos.xyz;
	pos = mul(pos, view);
	pos = mul(pos, projection);

	Output.pos = pos;
	Output.normal = mul(normal, model);
	Output.uv = uv;


	return Output;
}
