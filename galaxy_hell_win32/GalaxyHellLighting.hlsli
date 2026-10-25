#include "GalaxyHellShader.hlsli"
#include "../../llc/llc_engine/ShaderLights.hlsli"

Texture2D texDiffuse : register(t0);

float3 shadeGalaxyHell(GalaxyHellPixel input) {
	uint width, height;
	texDiffuse.GetDimensions(width, height);
	const uint2 texel = uint2(input.UV * (float2(width, height) - .000001f)) % uint2(width, height);
	precise float3 color = texDiffuse.Load(int3(texel, 0)).rgb;
	precise float3 accumulated = 0;
	const float3 ambient = EngineLightCount ? color / (10.0f * EngineLightCount) : 0;
	for(uint iLight = 0; iLight < EngineLightCount; ++iLight) {
		const SRenderLight light = EngineLights[iLight];
		const float3 offset = light.Position - input.WorldPosition;
		const float distanceSquared = dot(offset, offset);
		if(distanceSquared > light.Range * light.Range)
			continue;
		accumulated += ambient;
		if(dot(offset, input.Normal) <= 0)
			continue;
		accumulated += (color * light.Diffuse.rgb) * max(0, 1 - distanceSquared / (light.Range * light.Range));
	}
	precise float3 shaded = saturate(color * .2f + saturate(color * dot(input.Normal, LightDirectionAndSpotPower.xyz)) + saturate(accumulated));
	return floor(shaded * 255) * (1.0f / 255.0f);
}
