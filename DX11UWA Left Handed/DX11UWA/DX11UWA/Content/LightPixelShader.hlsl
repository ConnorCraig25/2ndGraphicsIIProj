Texture2D baseTexture : register(t0);
sampler filters : register(s0);


// Per-pixel color data passed through the pixel shader.
struct Light
{
    float4 Position; //16
    float4 Direction; //16
    float4 radius;
    float4 Color; //16
    float4 AttenuationData;
    float4 LightTypeEnabled;		
    float4 ConeRatio; // x = inner ratio,  y = outerratio	
    float4 coneAngle;
									
};       

struct PixelShaderInput
{
	float4 pos : SV_POSITION;
	float3 uv : UV;
    float3 normal : NORMAL;
	float3 world_pos : WORLDPOS;
};

cbuffer LightProperties : register(b0)
{
	float4 EyePosition;                 
	float4 GlobalAmbient;               
	Light Lights[3];           
};                       

float Attenuation(Light light, PixelShaderInput input)
{
	
	return 1.0f - saturate(length(light.Position.xyz - input.world_pos.xyz) / light.radius.x);
}

float SpotlightAttenuation(Light light, PixelShaderInput input)
{
	float ans;
    float3 lightdir = normalize(light.Position.xyz - input.world_pos.xyz);
    float surfaceratio = saturate(dot(-lightdir, normalize(light.coneAngle.xyz)));
	ans = 1 - saturate((light.ConeRatio.x - surfaceratio)/(light.ConeRatio.x - light.ConeRatio.y));
	return ans;
}

float4 PointLight(Light light, PixelShaderInput input)
{
	float4 result;
    float3 lightdir = normalize(light.Position.xyz - input.world_pos.xyz);
	float  lightratio = saturate(dot(lightdir,input.normal));

	result = lightratio * light.Color;
	result = result * Attenuation(light, input);
	return result;
}

float4 DirectionalLight(Light light, PixelShaderInput input)
{
	
	float4 result;
	float3 lightdir = normalize(light.Direction);
	float lightratio = saturate(dot(-lightdir, input.normal));
	result = lightratio * light.Color;

	return result;
}

float4 SpotLight(Light light, PixelShaderInput input)
{
	float4 result;
	
    float3 lightdir = normalize(light.Position.xyz - input.world_pos.xyz);
	float surfaceratio = saturate(dot(-lightdir, normalize(light.coneAngle.xyz)));
	float spotFactor = (surfaceratio > light.ConeRatio.y) ? 1 : 0;
	float lightratio = saturate(dot(lightdir, input.normal.xyz));

	result = spotFactor * lightratio * light.Color;
	result = result * Attenuation(light, input);
	result = result * SpotlightAttenuation(light, input);
	return result;
}

// A pass-through function for the (interpolated) color data.
float4 main(PixelShaderInput input) : SV_TARGET
{
    float4 baseColor = baseTexture.Sample(filters, input.uv);
    if (baseColor.a < 0.5f)
    {
        discard;
    }
    
    float4 dirColor = DirectionalLight(Lights[0], input) * baseColor;
    float4 spotColor = SpotLight(Lights[2], input) * baseColor;

    float4 pointcolor = PointLight(Lights[1], input) * baseColor;
   

   

	return saturate(dirColor + spotColor + pointcolor);
}
