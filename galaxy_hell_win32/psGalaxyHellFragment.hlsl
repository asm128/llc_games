#include "GalaxyHellLighting.hlsli"

float4 main(GalaxyHellPixel input, bool front : SV_IsFrontFace) : SV_TARGET {
	if(!front) input.Normal = -input.Normal;
	return float4(shadeGalaxyHell(input), 1);
}
