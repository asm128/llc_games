#include "llc_engine_scene.h"

#ifndef LLC_GALAXY_HELL_ENTITY_H
#define LLC_GALAXY_HELL_ENTITY_H

namespace ghg
{
	// Non-owning CPU views of the same engine buffers used by D3D.
	struct SShipGeometry {
		::llc::view<const ::llc::tri3f2_t>	Triangles;
		::llc::view<const ::llc::n3f2_t>		Normals;
		::llc::view<const ::llc::tri2f2_t>	TextureCoords;
	};
}

#endif // LLC_GALAXY_HELL_ENTITY_H