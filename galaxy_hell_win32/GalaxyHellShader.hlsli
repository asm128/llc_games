#include "../../llc/llc_engine/ShaderShared.hlsli"

struct GalaxyHellPixel {
	float4 Position : SV_POSITION;
	noperspective float3 WorldPosition : TEXCOORD0;
	noperspective float2 UV : TEXCOORD1;
	nointerpolation float3 Normal : NORMAL;
};
