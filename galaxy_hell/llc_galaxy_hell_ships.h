#include "llc_spaceship.h"
#include "llc_weapon.h"
#include "llc_galaxy_hell_entity.h"
#include "llc_rigid_body.h"

#include "llc_geometry_lh.h"
#include "llc_model.h"
#include "llc_engine.h"
#include "llc_engine_shader.h"
#include "llc_camera.h"
#include "llc_img_color.h"
#include "llc_view_n3.h"

#ifndef LLC_GALAXY_HELL_SHIPS_H
#define LLC_GALAXY_HELL_SHIPS_H

#define GH_SAVE_ENABLED 0 // Deferred while validating the engine migration.

namespace ghg
{
	::llc::err_t psGalaxyHell(const ::llc::SEngineSceneConstants & constants, const ::llc::SPSIn & input, ::llc::bgra & output);
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
		::ghg::SShipSceneGlobal			Global;

#if GH_SAVE_ENABLED
		::llc::error_t					Save	(::llc::au0_t & output) const { return ::llc::savePOD(output, Global); }
		::llc::error_t					Load	(::llc::vcu0_t & input)       { return ::llc::loadPOD(input, Global); }
#endif
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
		::llc::lmpod<int32_t, ::llc::u2_t> ModelNodes = {};
		::ghg::SShipScene			Scene				= {};

		::llc::error_t				ResetModelNodes		() {
			::llc::SRenderNodeManager nodes;
			for(uint32_t iModel = 0; iModel < ModelNodes.size(); ++iModel) {
				const uint32_t mesh = Engine.Scene->RenderNodes[ModelNodes.Values[iModel]].Mesh;
				llc_necs(ModelNodes.Values[iModel] = nodes.Create());
				::llc::SRenderNode & node = nodes[ModelNodes.Values[iModel]];
				node.Mesh = mesh;
				node.Slice = 0;
				nodes.Flags[ModelNodes.Values[iModel]].NoDraw = true;
			}
			Engine.Scene->RenderNodes = nodes;
			return 0;
		}

		::llc::error_t				Clear				() {
			SpaceshipManager.Clear();
			WeaponManager.Clear();
			::llc::clear(ShipCoreEntity, ShipPartEntity);
			Engine.Integrator.Clear();
			Engine.Entities.clear();
			return ResetModelNodes();
		}

		const ::llc::SRenderNode&		GetNode				(::llc::eid_t entity) const { return Engine.Scene->RenderNodes[Engine.GetRenderNode(entity)]; }
		::llc::SRenderNode&			GetNode				(::llc::eid_t entity)       { return Engine.Scene->RenderNodes[Engine.GetRenderNode(entity)]; }
		const ::llc::m4f2_t&			GetTransform		(::llc::eid_t entity) const { return Engine.Scene->RenderNodes.Transforms[Engine.GetRenderNode(entity)].Model; }
		::llc::error_t				GetModelMesh		(int32_t model) const { return Engine.Scene->RenderNodes[ModelNodes.Values[ModelNodes.Keys.find(model)]].Mesh; }

		::llc::error_t				GetGeometry			(uint32_t iMesh, ::ghg::SShipGeometry & geometry) const {
			const ::llc::SEngineGraphics & graphics = *Engine.Scene->Graphics;
			const ::llc::SGeometryMesh & mesh = *graphics.Meshes[iMesh];
			const ::llc::au0_t & positions = graphics.Buffers[mesh.GeometryBuffers[1]]->Data;
			const ::llc::au0_t & normals = graphics.Buffers[mesh.GeometryBuffers[2]]->Data;
			const ::llc::au0_t & uv = graphics.Buffers[mesh.GeometryBuffers[3]]->Data;
			geometry.Triangles = {(const ::llc::tri3f2_t*)positions.begin(), positions.size() / uint32_t(sizeof(::llc::tri3f2_t))};
			geometry.Normals = {(const ::llc::n3f2_t*)normals.begin(), normals.size() / uint32_t(sizeof(::llc::n3f2_t))};
			geometry.TextureCoords = {(const ::llc::tri2f2_t*)uv.begin(), uv.size() / uint32_t(sizeof(::llc::tri2f2_t))};
			return 0;
		}
		::llc::error_t				GetImage			(uint32_t iSurface, ::llc::gc8bgra & image) const {
			const ::llc::SSurface & surface = *Engine.Scene->Graphics->Surfaces[iSurface];
			image = {(const ::llc::bgra*)surface.Data.begin(), surface.Desc.Dimensions.u2()};
			return 0;
		}
		::llc::error_t				BindShaders			() {
			::llc::SShaderManager & shaders = Engine.Scene->Graphics->Shaders;
			shaders = {};
			llc_necs(shaders.Create());
			if_null_fe(shaders[0].create(::ghg::psGalaxyHell));
			shaders.Names[0] = "psGalaxyHell";
			return 0;
		}

		inline	::llc::SBodyCenter&			GetShipPivot		(uint32_t indexShip)				{ return Engine.GetBodyCenter(ShipCoreEntity[indexShip]); }
		inline	const ::llc::SBodyCenter&	GetShipPivot		(uint32_t indexShip)		const	{ return Engine.GetBodyCenter(ShipCoreEntity[indexShip]); }

		inline	::llc::n3f2_t&				GetShipPosition		(uint32_t indexShip)				{ return GetShipPivot(indexShip).Position; }
		inline	const ::llc::n3f2_t&			GetShipPosition		(uint32_t indexShip)		const	{ return GetShipPivot(indexShip).Position; }

		::llc::error_t						GetShipPosition		(uint32_t iShip, ::llc::n3f2_t & output) const { output = GetShipPosition(iShip); return 0; }
		
		::llc::eid_t						GetShipPartEntity	(uint32_t iPart) const { return Engine.Entities.GetIndexChild(ShipPartEntity[iPart], 0); }

		::llc::SBodyCenter&					GetOrbiterTransform	(uint32_t iPart)		{ return Engine.GetBodyCenter(GetShipPartEntity(iPart)); }
		::llc::SBodyForces&					GetShipOrbiterForces(uint32_t iPart)		{ return Engine.GetBodyForces(GetShipPartEntity(iPart)); }


#if GH_SAVE_ENABLED
		::llc::error_t				Save				(::llc::au0_t & output)	const	{ 
			llc_necs(SpaceshipManager.Save(output));
			llc_necs(WeaponManager.Save(output));

			llc_necs(::llc::saveView(output, ShipCoreEntity));
			llc_necs(::llc::saveView(output, ShipPartEntity));
		
			llc_necs(Engine.Save(output));
			llc_necs(::llc::saveView(output, ModelNodes.Keys));
			llc_necs(::llc::saveView(output, ModelNodes.Values));
			llc_necs(Scene			.Save(output));
			return 0; 
		}
		::llc::error_t				Load				(::llc::vcu0_t & input) { 
			llc_necs(SpaceshipManager.Load(input));
			llc_necs(WeaponManager.Load(input));

			llc_necs(::llc::loadView(input, ShipCoreEntity));
			llc_necs(::llc::loadView(input, ShipPartEntity));

			llc_necs(Engine.Load(input));

			llc_necs(::llc::loadView(input, ModelNodes.Keys));
			llc_necs(::llc::loadView(input, ModelNodes.Values));
			llc_necs(BindShaders());
			llc_necs(Scene			.Load(input));
			return 0; 
		}
#endif
	};

}

#endif // LLC_GALAXY_HELL_SHIPS_H
