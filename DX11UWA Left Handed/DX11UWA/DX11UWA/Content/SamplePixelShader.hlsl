SamplerState filters[2] : register(s0);
// Per-pixel color data passed through the pixel shader.
struct PixelShaderInput
{
	float4 pos : SV_POSITION;
	float3 uv : UV;
	float3 normal : NORMAL;
	float3 world_pos : WORLDPOS;
};

texture2D baseTexture : register(t0); // first texture
 // second texture



// A pass-through function for the (interpolated) color data.
float4 main(PixelShaderInput input) : SV_TARGET
{
	//return float4(input.uv, 1.0f);
	float4 tmp = baseTexture.Sample(filters[0],input.uv);
	if (tmp.a < 0.25f)
	{
		discard;
	}


	return tmp;
}
