#include "GalaxyHellLighting.hlsli"

float4 main(GalaxyHellPixel input) : SV_TARGET {
	precise float3 shaded = shadeGalaxyHell(input);
	shaded = floor(saturate(shaded + EmissionaAndSpecularPower.xyz) * 255) * (1.0f / 255.0f);
	return float4(shaded, 1);
}
