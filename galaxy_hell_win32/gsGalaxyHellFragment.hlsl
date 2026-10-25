#include "GalaxyHellShader.hlsli"

[maxvertexcount(3)]
void main(triangle GalaxyHellPixel input[3], inout TriangleStream<GalaxyHellPixel> output) {
	[unroll] for(uint i = 0; i < 3; ++i) {
		const float depth = input[i].Position.z / input[i].Position.w;
		if(depth <= 0 || depth >= 1) return;
	}
	[unroll] for(uint i = 0; i < 3; ++i) output.Append(input[i]);
}
