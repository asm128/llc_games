#include "llc_enum.h"
#include "llc_apod_serialize.h"

#ifndef LLC_GALAXY_HELL_ENTITY_H
#define LLC_GALAXY_HELL_ENTITY_H

namespace ghg 
{
#pragma pack(push, 1)
	struct SGHEntity {
		int32_t						Parent					= -1;
		int32_t						Geometry				= -1;
		int32_t						Transform				= -1;
		int32_t						Image					= -1;
		int32_t						Body					= -1;
	};
#pragma pack(pop)

	struct SEntitySystem {
		::llc::apod<::ghg::SGHEntity>	Entities				= {};
		::llc::aobj<::llc::au2_t>		EntityChildren			= {};

		::llc::error_t				Create					(const ::ghg::SGHEntity & newEntity, ::llc::vcu32 entityChildren = {})	{ EntityChildren.push_back(entityChildren); return Entities.push_back(newEntity); }
		::llc::error_t				Create					()																		{ Create({}); }

		::llc::error_t				Save					(::llc::au0_t & output) const { 
			llc_necs(::llc::saveView(output, Entities));
			for(uint32_t iEntity = 0; iEntity < Entities.size(); ++iEntity) 
				llc_necall(::llc::saveView(output, ::llc::vcu32{EntityChildren[iEntity]}), "iEntity: %i", iEntity);
			info_printf("Saved %s, %i", "Entities"					, EntityChildren.size());
			return 0; 
		}

		::llc::error_t				Load					(::llc::vcu0_t & input) { 
			llc_necs(::llc::loadView(input, Entities));
			llc_necall(EntityChildren.resize(Entities.size()), "size: %i", Entities.size());
			for(uint32_t iEntity = 0; iEntity < Entities.size(); ++iEntity)
				llc_necall(::llc::loadView(input, EntityChildren[iEntity]), "iEntity: %i", iEntity);

			return 0; 
		}		
		// 11 6688 8367
	};
}

#endif // LLC_GALAXY_HELL_ENTITY_H