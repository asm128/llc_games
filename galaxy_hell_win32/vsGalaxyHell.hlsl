#include "GalaxyHellShader.hlsli"

GalaxyHellPixel main(VertexShaderInput input) {
	GalaxyHellPixel output;
	output.Position = mul(float4(input.position, 1), MVP);
	output.Position.xy += float2(.5f / Screen[0][0], .5f / Screen[1][1]) * output.Position.w;
	output.WorldPosition = mul(float4(input.position, 1), Model).xyz;
	output.Normal = normalize(mul(float4(input.normal, 0), Model).xyz);
	output.UV = input.uv;
	return output;
}
