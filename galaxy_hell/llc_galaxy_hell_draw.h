#include "llc_img_color.h"
#include "llc_array_ptr.h"

#ifndef LLC_GALAXY_HELL_DRAW_H_293874239874
#define LLC_GALAXY_HELL_DRAW_H_293874239874

namespace ghg
{

	typedef ::llc::rt<::llc::bgra, ::llc::u2_t>			TRenderTarget;

	static constexpr ::llc::bgra		DEBRIS_COLORS[]			=
		{ {0x80, 0xAF, 0xFF, }
		, {0x40, 0x80, 0xFF, }
		, {0x20, 0x80, 0xFF, }
		, {0x00, 0x00, 0xFF, }
		};

	struct SGalaxyHellDrawCache {
		// Used when rastering triangles
		::llc::apod<::llc::n2s1_t>			PixelCoords				;
		::llc::apod<::llc::trif2_t>			PixelVertexWeights		;
		
		// Used for pixel shading
		::llc::apod<::llc::n3f2_t>			LightPointsWorld		;
		::llc::a8bgra						LightColorsWorld		;
		::llc::apod<::llc::n3f2_t>			LightPointsModel		;
		::llc::a8bgra						LightColorsModel		;

		// Gameplay render target. I decided the world will have its own rendertarget to facilitate reuse of the code.
		::llc::n2u1_t						RenderTargetMetrics		= {1280, 720};
		::llc::pobj<::ghg::TRenderTarget>	RenderTarget			= {};

		::llc::error_t						Clear					() {
			::llc::clear
				( PixelCoords
				, PixelVertexWeights		
				, LightPointsWorld		
				, LightColorsWorld		
				, LightPointsModel		
				, LightColorsModel				
				);
			return 0;
		}
	};
} // namespace 

#endif // LLC_GALAXY_HELL_DRAW_H_293874239874