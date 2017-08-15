cbuffer ModelViewProjectionConstantBufferInstanced : register(b0)
{
	matrix model[3];
	matrix view;
	matrix projection;
};

struct GSOutput
{
	float4 pos : SV_POSITION;
	float3 uv : UV;
	float3 normal : NORMAL;
	float3 world_pos : WORLDPOS;
};

// Per-pixel color data passed through the pixel shader.
struct PixelShaderInput
{
	float4 pos : SV_POSITION;
	float3 uv : UV;
	float3 normal : NORMAL;
	float3 world_pos : WORLDPOS;
};

[maxvertexcount(3)]
void main(triangle GSOutput input[3], inout TriangleStream< PixelShaderInput > output)
{
	for (uint i = 0; i < 3; i++)
	{
		PixelShaderInput element;
		element.pos = input[i].pos;
		element.uv = input[i].uv;
		element.normal = input[i].normal;
		element.world_pos = input[i].world_pos;
		output.Append(element);
	}
}