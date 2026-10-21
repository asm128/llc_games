#include "llc_spaceship.h"
#include "llc_weapon.h"
#include "llc_galaxy_hell_entity.h"
#include "llc_rigid_body.h"

#include "llc_geometry_lh.h"
#include "llc_model.h"
#include "llc_engine.h"
#include "llc_camera.h"
#include "llc_img_color.h"
#include "llc_view_n3.h"

#ifndef LLC_GALAXY_HELL_SHIPS_H
#define LLC_GALAXY_HELL_SHIPS_H

namespace ghg
{
	enum CAMERA_MODE
		{ CAMERA_MODE_SKY			= 0
		, CAMERA_MODE_PERSPECTIVE
		, CAMERA_MODE_MAP
		, CAMERA_MODE_FOLLOW
		, CAMERA_MODE_FRONT
		, CAMERA_MODE_REAR
		, CAMERA_MODE_COUNT
		};

#pragma pack(push, 1)
	struct SShipSceneGlobal	{
		::llc::m4f2_t			MatrixProjection	= {};
		::llc::n3f2_t			LightVector			= {0, -12, 0};
		CAMERA_MODE				CameraMode			= CAMERA_MODE_PERSPECTIVE;

		::llc::astatic<::llc::SCameraPoints, ::ghg::CAMERA_MODE_COUNT>	
								Camera				= {};

		::llc::error_t			CameraReset			() {
			Camera[CAMERA_MODE_SKY].Target		= {};
			Camera[CAMERA_MODE_SKY].Position	= {-0.000001f, 250, 0};
			//Camera[CAMERA_MODE_SKY].Up			= {0, 1, 0};

			Camera[CAMERA_MODE_MAP].Target		= {};
			Camera[CAMERA_MODE_MAP].Position	= {-0.000001f, 2500, 0};
			//Camera[CAMERA_MODE_MAP].Up			= {0, 1, 0};

			Camera[CAMERA_MODE_PERSPECTIVE].Target		= {};
			Camera[CAMERA_MODE_PERSPECTIVE].Position	= {-0.000001f, 220, 0};
			//Camera[CAMERA_MODE_PERSPECTIVE].Up		= {0, 1, 0};
			Camera[CAMERA_MODE_PERSPECTIVE].Position.RotateZ(::llc::math_pi * .325);
			return 0;
		}
	};
#pragma pack(pop)

	struct SShipScene {
		::ghg::SShipSceneGlobal				Global;
		::llc::aobj<::llc::SGeometryQuads>	Geometry		= {};
		::llc::aobj<::llc::img8bgra>		Image			= {};
		::llc::apod<::llc::m4f2_t>			Transforms		= {}; // Ope per entity

		::llc::error_t						Save			(::llc::au0_t & output) const { 
			llc_necs(::llc::savePOD		(output, Global));
			llc_necs(::llc::saveView	(output, Transforms));
			info_printf("Saved %s, %i", "Transforms"	, Transforms.size());
			return 0; 
		}

		::llc::error_t						Load			(::llc::vcu0_t & input) { 
			llc_necs(::llc::loadPOD	(input, Global		));
			llc_necs(::llc::loadView(input, Transforms	));
			return 0; 
		}
	};

	::llc::error_t			getLightArrays
		( const ::llc::n3f2_t	& modelPosition
		, const ::llc::vn3f2_t	& lightPointsWorld
		, const ::llc::a8bgra	& lightColorsWorld
		, ::llc::an3f2_t			& lightPointsModel
		, ::llc::a8bgra			& lightColorsModel
		);


	struct SShipManager  {
		::llc::SSpaceshipManager	SpaceshipManager;
		::llc::SWeaponManager		WeaponManager;

		::llc::aeid					ShipCoreEntity		= {};
		::llc::aeid					ShipPartEntity		= {};

		::llc::SEngine				Engine;
		::ghg::SEntitySystem		EntitySystem		= {};
		::ghg::SShipScene			Scene				= {};

		int32_t						Clear				()	{
			SpaceshipManager.Clear();
			WeaponManager.Clear();
			::llc::clear(Scene.Transforms, ShipCoreEntity, ShipPartEntity);

			Engine.Integrator.Clear();
			Engine.Entities.clear();

			EntitySystem				= {};
			return 0;
		}

		inline	::llc::SBodyCenter&			GetShipPivot		(uint32_t indexShip)				{ return Engine.Integrator.Centers[EntitySystem.Entities[ShipCoreEntity[indexShip]].Body]; }
		inline	const ::llc::SBodyCenter&	GetShipPivot		(uint32_t indexShip)		const	{ return Engine.Integrator.Centers[EntitySystem.Entities[ShipCoreEntity[indexShip]].Body]; }

		inline	::llc::n3f2_t&				GetShipPosition		(uint32_t indexShip)				{ return GetShipPivot(indexShip).Position; }
		inline	const ::llc::n3f2_t&			GetShipPosition		(uint32_t indexShip)		const	{ return GetShipPivot(indexShip).Position; }

		::llc::error_t						GetShipPosition		(uint32_t iShip, ::llc::n3f2_t & output) const { output = GetShipPosition(iShip); return 0; }
		
		::llc::SBodyCenter&					GetOrbiterTransform	(uint32_t iPart)		{ return Engine.Integrator.Centers[EntitySystem.Entities[ShipPartEntity[iPart] + 1].Body]; }
		::llc::SBodyForces&					GetShipOrbiterForces(uint32_t iPart)		{ return Engine.Integrator.Forces [EntitySystem.Entities[ShipPartEntity[iPart] + 1].Body]; }


		::llc::error_t				Save				(::llc::au0_t & output)	const	{ 
			llc_necs(SpaceshipManager.Save(output));
			llc_necs(WeaponManager.Save(output));

			llc_necs(::llc::saveView(output, ShipCoreEntity));
			llc_necs(::llc::saveView(output, ShipPartEntity));
		
			llc_necs(Engine.Save(output));

			llc_necs(EntitySystem	.Save(output));
			llc_necs(Scene			.Save(output));
			return 0; 
		}
		::llc::error_t				Load				(::llc::vcu0_t & input) { 
			llc_necs(SpaceshipManager.Load(input));
			llc_necs(WeaponManager.Load(input));

			llc_necs(::llc::loadView(input, ShipCoreEntity));
			llc_necs(::llc::loadView(input, ShipPartEntity));

			llc_necs(Engine.Load(input));

			llc_necs(EntitySystem	.Load(input));
			llc_necs(Scene			.Load(input));
			return 0; 
		}
	};

}

#endif // LLC_GALAXY_HELL_SHIPS_H