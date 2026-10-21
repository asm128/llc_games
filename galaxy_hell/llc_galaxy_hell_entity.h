#include "llc_engine_entity.h"

#ifndef LLC_GALAXY_HELL_ENTITY_H
#define LLC_GALAXY_HELL_ENTITY_H

namespace ghg
{
#pragma pack(push, 1)
	struct SGHEntityVisual {
		int32_t						Geometry				= -1;
		int32_t						Transform				= -1;
		int32_t						Image					= -1;
	};

	// Pre-migration save layout only. Runtime relationships and bodies belong to SEngine.
	struct SGHEntitySave {
		int32_t						Parent					= -1;
		int32_t						Geometry				= -1;
		int32_t						Transform				= -1;
		int32_t						Image					= -1;
		int32_t						Body					= -1;
	};
#pragma pack(pop)

	inline ::llc::error_t shipEntitiesSave(const ::llc::SVirtualEntityManager & entities, ::llc::view<const SGHEntityVisual> visuals, ::llc::au0_t & output) {
		if_true_fe(entities.size() != visuals.size());
		::llc::apod<SGHEntitySave> saved;
		llc_necs(saved.resize(entities.size()));
		for(uint32_t iEntity = 0; iEntity < entities.size(); ++iEntity) {
			const ::llc::SEntity & entity = entities[iEntity];
			if_true_fe(entity.RenderNode != ::llc::EID_INVALID);
			const SGHEntityVisual & visual = visuals[iEntity];
			saved[iEntity] = {(int32_t)entity.Parent, visual.Geometry, visual.Transform, visual.Image, (int32_t)entity.RigidBody};
		}
		llc_necs(::llc::saveView(output, saved));
		for(uint32_t iEntity = 0; iEntity < entities.size(); ++iEntity)
			llc_necs(::llc::saveView(output, entities.GetChildren(iEntity)));
		return 0;
	}

	inline ::llc::error_t shipEntitiesLoad(::llc::SVirtualEntityManager & entities, ::llc::apod<SGHEntityVisual> & visuals, ::llc::vcu0_t & input) {
		::llc::apod<SGHEntitySave> saved;
		llc_necs(::llc::loadView(input, saved));
		entities.clear();
		llc_necs(visuals.resize(saved.size()));
		for(uint32_t iEntity = 0; iEntity < saved.size(); ++iEntity) {
			llc_necs(entities.Create());
			entities[iEntity].RigidBody = (::llc::u2_t)saved[iEntity].Body;
			visuals[iEntity] = {saved[iEntity].Geometry, saved[iEntity].Transform, saved[iEntity].Image};
		}
		for(uint32_t iParent = 0; iParent < saved.size(); ++iParent) {
			::llc::au2_t children;
			llc_necs(::llc::loadView(input, children));
			for(uint32_t iChild = 0; iChild < children.size(); ++iChild) {
				const ::llc::eid_t child = children[iChild];
				if_true_fe(child >= saved.size() || child == iParent);
				if_true_fe(saved[child].Parent != (int32_t)iParent || entities[child].Parent != ::llc::EID_INVALID);
				llc_necs(entities.SetParent(child, iParent));
			}
		}
		for(uint32_t iEntity = 0; iEntity < saved.size(); ++iEntity)
			if_true_fe(entities[iEntity].Parent != (::llc::eid_t)saved[iEntity].Parent);
		return 0;
	}
}

#endif // LLC_GALAXY_HELL_ENTITY_H
