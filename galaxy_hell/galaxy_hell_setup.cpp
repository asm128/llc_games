//#define LLC_INFO_PRINTF_ENABLED

#include "llc_galaxy_hell.h"
#include "llc_file.h"
#include "llc_json.h"
#include "llc_noise.h"
#include "llc_deflate.h"
#include "llc_engine_orbit.h"
#include "llc_chrono.h"

#include <windows.h>
#include <mmsystem.h>

using ::llc::get_value_namep, ::llc::get_enum_namep, ::llc::failed;
LLC_USING_TYPEINT();

#if GH_SAVE_ENABLED
::llc::error_t			ghg::solarSystemSave	(const ::ghg::SGalaxyHell & game, ::llc::vcst_t fileName) {
	::llc::au0_t					serialized;
	llc_necs(game.Save(serialized));
	llc_necs(::llc::deflateFromMemory(fileName, serialized));
	return 0;
}

::llc::error_t			ghg::solarSystemLoad	(::ghg::SGalaxyHell & world,::llc::vcst_t filename) {
	::llc::au0_t					serialized;
	llc_necs(::llc::inflateToMemory(filename, serialized));

	::llc::vcu0_t					viewSerialized			= serialized;
	if (failed(world.Load(viewSerialized))) {
		llc_necs(::ghg::solarSystemReset(world));
		return -1;
	}

	world.PlayState.GlobalState.UserTime.Loaded	= ::llc::timeCurrent();
	world.PlayState.GlobalState.Paused		= true;
	return 0;
}
#endif

static	::llc::error_t	shipCreate				(::ghg::SShipManager & shipState, int32_t teamId, int32_t iGeometry, int32_t iImage) {
	::llc::SEngine & engine = shipState.Engine;
	const uint32_t countParts = 6;
	::llc::SSpaceshipCore ship = {};
	int32_t iEntityMain;
	llc_necs(iEntityMain = engine.Entities.Create());
	llc_necs(engine.Entities[iEntityMain].RenderNode = engine.Scene->RenderNodes.Create());
	llc_necs(engine.Entities[iEntityMain].RigidBody = engine.Integrator.Create());
	llc_necs(engine.SetHidden(iEntityMain, true));
	llc_necs(shipState.ShipCoreEntity.push_back(iEntityMain));
	ship.Team = teamId;
	const int32_t indexShip = shipState.SpaceshipManager.ShipCores.push_back(ship);
	shipState.SpaceshipManager.ShipParts.push_back({});
	shipState.SpaceshipManager.ShipScores.push_back({});
	for(uint32_t iPart = 0; iPart < countParts; ++iPart) {
		int32_t iEntityOrbit;
		llc_necs(iEntityOrbit = engine.Entities.Create());
		llc_necs(engine.Entities[iEntityOrbit].RenderNode = engine.Scene->RenderNodes.Create());
		llc_necs(engine.Entities[iEntityOrbit].RigidBody = engine.Integrator.Create());
		llc_necs(engine.SetHidden(iEntityOrbit, true));
		llc_necs(::llc::initOrbiterOrbit(engine.GetBodyCenter(iEntityOrbit), engine.GetBodyForces(iEntityOrbit), 0, 6, 1));
		engine.GetBodyCenter(iEntityOrbit).Orientation.MakeFromEuler(0, (float)(::llc::math_2pi / countParts * iPart), 0);
		llc_necs(engine.SetPhysicsActive(iEntityOrbit, true));

		int32_t indexEntityPart;
		const int32_t model = (iGeometry <= 0) ? ::llc::SHIP_GEOMETRY_Engine : iGeometry;
		llc_necs(indexEntityPart = engine.CreateEntityFromGeometry("Ship module", {.5f, .5f, .5f}, false, model, shipState.ModelNodes
			, [](::llc::SGeometryBuffers &) { return -1; })); // Models were built at setup.
		llc_necs(::llc::initOrbitCenter(engine.GetBodyCenter(indexEntityPart), ::llc::AXIS_X_POSITIVE, 0, 2.5));
		llc_necs(::llc::initOrbitRotation(engine.GetBodyForces(indexEntityPart), ::llc::AXIS_Y_NEGATIVE, engine.GetBodyCenter(indexEntityPart).Orientation, 1, 2));
		llc_necs(engine.SetParent(indexEntityPart, iEntityOrbit));
		llc_necs(engine.SetPhysicsActive(indexEntityPart, true));
		llc_necs(engine.SetHidden(indexEntityPart, iGeometry <= 0));
		shipState.GetNode(indexEntityPart).Skin = iImage % engine.Scene->Graphics->Skins.size();
		shipState.GetNode(indexEntityPart).Shader = 0;

		::llc::SSpaceshipOrbiter shipPart = {};
		shipState.ShipPartEntity.push_back(iEntityOrbit);
		shipState.SpaceshipManager.ShipParts[indexShip].push_back((uint16_t)shipState.SpaceshipManager.Orbiters.push_back(shipPart));
		shipState.SpaceshipManager.ShipOrbiterActionQueue.push_back({});
		llc_necs(engine.SetParent(iEntityOrbit, iEntityMain));
	}
	return indexShip;
}

static	::llc::error_t	shipGeometryBuildEngine			(::llc::SGeometryQuads & geometry)			{
	::llc::geometryBuildHelix(geometry, 8U, 8U, 0.15f, {0, 1.1f, 0}, {1, 1, 1});
	//::llc::geometryBuildCylinder(geometry, 1U, 6U, .05f, .05f, {0, 1, 0}, {1, 1, 1});
	::llc::geometryBuildCylinder(geometry, 1U, 8U, .35f, .25f, {0, 1, 0}, {1, 1, 1});
	::llc::geometryBuildCylinder(geometry, 1U, 8U, .35f, .25f, {0, 1, 0}, {-1, 1, 1});
	return 0;
}

static	::llc::error_t	shipGeometryBuildWafer			(::llc::SGeometryQuads & geometry)			{
	::shipGeometryBuildEngine(geometry);
	::llc::geometryBuildCube(geometry, {1.2f, 0.5f, 1.2f});
	::llc::geometryBuildGrid(geometry, {1U, 1U}, { 0.75f, .75f}, { 1.5, 1, 1.5});
	::llc::geometryBuildGrid(geometry, {1U, 1U}, {-0.75f, .75f}, {-1.5, 1, 1.5});
	return 0;
}

static	::llc::error_t	shipGeometryBuildWaferShotgun	(::llc::SGeometryQuads & geometry)			{
	::shipGeometryBuildEngine(geometry);
	::llc::geometryBuildCube(geometry, {.3f, 1.5f, .3f}, {-.25f, 0, -.25f});
	::llc::geometryBuildCube(geometry, {.3f, 1.5f, .3f}, {-.25f, 0, .25f});
	::llc::geometryBuildCube(geometry, {.3f, 1.5f, .3f}, {.25f, 0, -.25f});
	::llc::geometryBuildCube(geometry, {.3f, 1.5f, .3f}, {.25f, 0, .25f});
	::llc::geometryBuildGrid(geometry, {1U, 1U}, { 0.75f, .75f}, { 1.5, 1, 1.5});
	::llc::geometryBuildGrid(geometry, {1U, 1U}, {-0.75f, .75f}, {-1.5, 1, 1.5});
	return 0;
}

static	::llc::error_t	shipGeometryBuildGun		(::llc::SGeometryQuads & geometry)						{
	::shipGeometryBuildEngine(geometry);
	::llc::geometryBuildSphere	(geometry, 8U, 5U, .7f, {0, 0});
	::llc::geometryBuildFigure0	(geometry, 2U, 8U, 1, {});
	::llc::geometryBuildCylinder(geometry, 2U, 8U, .125f, .1f, {}, {1, 1, 1});
	::llc::geometryBuildCylinder(geometry, 2U, 8U, .125f, .1f, {}, {-1, 1, 1});
	return 0;
}

static	::llc::error_t	shipGeometryBuildShotgun	(::llc::SGeometryQuads & geometry)						{
	::shipGeometryBuildEngine(geometry);
	::llc::geometryBuildSphere	(geometry, 8U, 5U, .35f, {0, 0});
	::llc::geometryBuildFigure0	(geometry, 2U, 8U, 1, {});
	::llc::geometryBuildCylinder(geometry, 2U, 8U, .25f, .125f, {0, 0, .25f}, {1, 1, 1});
	::llc::geometryBuildCylinder(geometry, 2U, 8U, .25f, .125f, {0, 0, .25f}, {-1, 1, 1});
	::llc::geometryBuildCylinder(geometry, 2U, 8U, .25f, .125f, {0, 0, -.25f}, {1, 1, 1});
	::llc::geometryBuildCylinder(geometry, 2U, 8U, .25f, .125f, {0, 0, -.25f}, {-1, 1, 1});
	return 0;
}

static	::llc::error_t	shipGeometryBuildCannon		(::llc::SGeometryQuads & geometry)			{
	::shipGeometryBuildEngine(geometry);
	::llc::geometryBuildSphere	(geometry, 8U, 5U, .8f, {0, 0});
	::llc::geometryBuildTender	(geometry, 8U, 8U, .75f, {0, -.5, 0}, { .25, .8f, .25});
	::llc::geometryBuildTender	(geometry, 8U, 8U, .75f, {0, -.5, 0}, {-.25, .8f, .25});
	return 0;
}

static	::llc::error_t	shipGeometryBuildCannonball	(::llc::SGeometryQuads & geometry)			{
	::llc::geometryBuildSphere		(geometry, 6U, 4U, .5f, {0, 0});
	return 0;
}

static	::llc::error_t	shipGeometryBuildRocket		(::llc::SGeometryQuads & geometry)			{
	::llc::geometryBuildCylinder(geometry, 2U, 8U, .125f, .125f, {}, {1, 1, 1});
	::llc::geometryBuildCylinder(geometry, 2U, 8U, .125f, .125f, {}, {-1, 1, 1});
	return 0;
}

static	::llc::error_t	shipGeometryBuildMissile	(::llc::SGeometryQuads & geometry)			{
	::llc::geometryBuildCylinder(geometry, 2U, 8U, .125f, .125f, {}, {1, 1, 1});
	::llc::geometryBuildCylinder(geometry, 2U, 8U, .125f, .125f, {}, {-1, 1, 1});
	return 0;
}

static	::llc::error_t	shipGeometryBuildBullet		(::llc::SGeometryQuads & geometry)			{
	::llc::geometryBuildCylinder(geometry, 2U, 8U, .125f, .125f, {}, {1, 1, 1});
	::llc::geometryBuildCylinder(geometry, 2U, 8U, .125f, .001f, {}, {-1, 1, 1});
	return 0;
}

static	::llc::error_t	shipGeometryBuildShred		(::llc::SGeometryQuads & geometry)			{
	::llc::geometryBuildCylinder(geometry, 2U, 8U, .125f, .125f, {}, {1, 1, 1});
	::llc::geometryBuildCylinder(geometry, 2U, 8U, .125f, .001f, {}, {-1, 1, 1});
	return 0;
}

static	::llc::error_t	modelsSetup			(::ghg::SShipManager & shipState)			{
	if(shipState.ModelNodes.size())
		return 0;
	::llc::SEngine & engine = shipState.Engine;
	if(!engine.Scene->Graphics)
		if_null_fe(engine.Scene->Graphics.create());
	::llc::SEngineGraphics & graphics = *engine.Scene->Graphics;
	::llc::aobj<::llc::SGeometryQuads> geometryQuads;
	llc_necs(geometryQuads.resize(::llc::SHIP_GEOMETRY_Nitro + 1));

	::shipGeometryBuildEngine		(geometryQuads[::llc::SHIP_GEOMETRY_Engine			]);
	::shipGeometryBuildGun			(geometryQuads[::llc::SHIP_GEOMETRY_Gun			]);
	::shipGeometryBuildWafer		(geometryQuads[::llc::SHIP_GEOMETRY_Wafer			]);
	::shipGeometryBuildCannon		(geometryQuads[::llc::SHIP_GEOMETRY_Cannon			]);
	::shipGeometryBuildShotgun		(geometryQuads[::llc::SHIP_GEOMETRY_Shotgun		]);
	::shipGeometryBuildWaferShotgun	(geometryQuads[::llc::SHIP_GEOMETRY_WaferShotgun	]);
	::shipGeometryBuildCannonball	(geometryQuads[::llc::SHIP_GEOMETRY_Cannonball		]);
	::shipGeometryBuildRocket		(geometryQuads[::llc::SHIP_GEOMETRY_Rocket			]);
	::shipGeometryBuildMissile		(geometryQuads[::llc::SHIP_GEOMETRY_Missile		]);
	::shipGeometryBuildBullet		(geometryQuads[::llc::SHIP_GEOMETRY_Bullet			]);
	::shipGeometryBuildShred		(geometryQuads[::llc::SHIP_GEOMETRY_Shred			]);

	::llc::geometryBuildCube		(geometryQuads[::llc::SHIP_GEOMETRY_Cube			], {1, 1, 1});
	::llc::geometryBuildSphere		(geometryQuads[::llc::SHIP_GEOMETRY_Sphere			], 16U, 16U, .5f, {0, 0});
	::llc::geometryBuildCylinder	(geometryQuads[::llc::SHIP_GEOMETRY_Cylinder		],  1U, 32U, .5, .5, {0, 0}, {1, 1, 1});
	for(uint32_t iModel = 0; iModel < geometryQuads.size(); ++iModel) {
		const ::llc::SGeometryQuads & source = geometryQuads[iModel];
		if(0 == source.Triangles.size())
			continue;
		int32_t entity;
		llc_necs(entity = engine.CreateEntityFromGeometry(::llc::get_value_namev((::llc::SHIP_GEOMETRY)iModel), {.5f, .5f, .5f}, false, int32_t(iModel), shipState.ModelNodes
			, [&source](::llc::SGeometryBuffers & geometry) {
				for(uint32_t iTriangle = 0; iTriangle < source.Triangles.size(); ++iTriangle) {
					const ::llc::tri3f2_t & triangle = source.Triangles[iTriangle];
					const ::llc::tri2f2_t & uv = source.TextureCoords[iTriangle];
					const uint32_t first = geometry.Positions.size();
					llc_necs(geometry.Positions.append({triangle.A, triangle.B, triangle.C}));
					llc_necs(geometry.TextureCoords.append({uv.A, uv.B, uv.C}));
					llc_necs(geometry.Normals.append({source.Normals[iTriangle / 2], source.Normals[iTriangle / 2], source.Normals[iTriangle / 2]}));
					llc_necs(geometry.PositionIndices.append({first, first + 1, first + 2}));
				}
				return 0;
			}));
		::llc::SRenderNode & node = shipState.GetNode(entity);
		llc_necs(graphics.Meshes[node.Mesh]->GeometrySlices.push_back({{0, source.Triangles.size() * 3}}));
		node.Slice = 0;
		llc_necs(engine.SetHidden(entity, true));
	}
	engine.Entities.clear();
	engine.Integrator.Clear();
	llc_necs(shipState.BindShaders());
	{

		::llc::apod<::llc::rgbaf>	baseColors;
		for(uint32_t i = 0; i < 4096; ++i)
			baseColors.push_back({(float)::llc::noiseNormal1D(rand()), (float)::llc::noiseNormal1D(rand()), (float)::llc::noiseNormal1D(rand()), 1});

		while(graphics.Surfaces.size() < 256) {
			llc_necs(graphics.Surfaces.Create());
			llc_necs(graphics.Skins.Create());
			if_null_fe(graphics.Surfaces[graphics.Surfaces.size() - 1].create());
			if_null_fe(graphics.Skins[graphics.Skins.size() - 1].create());
		}
		for(uint32_t iImage = 0; iImage < graphics.Surfaces.size(); ++iImage) {
			::llc::SSurface & surface = *graphics.Surfaces[iImage];
			::llc::SSkin & skin = *graphics.Skins[iImage];
			skin.Material.Color.Diffuse = ::llc::WHITE;
			skin.Material.Color.Ambient = ::llc::rgbaf(::llc::WHITE) * .1f;
			llc_necs(skin.Textures.push_back(iImage));
			if(surface.Data.size())
				continue;
			surface.Desc.ColorType = ::llc::COLOR_TYPE_BGRA;
			surface.Desc.BitDepth = 8;
			surface.Desc.Dimensions = {32, 5};
			llc_necs(surface.Data.resize(32 * 5 * sizeof(::llc::bgra)));
			::llc::g8bgra image = {(::llc::bgra*)surface.Data.begin(), surface.Desc.Dimensions.u2()};
			for(uint32_t y = 0; y < image.metrics().y; ++y) {// Generate noise color for planet texture
				bool						xAffect					= (y % 2);
				::llc::rgbaf				lineColor				= baseColors[(iImage + (rand() % 4)) % (baseColors.size() - 4)];
				for(uint32_t x = 0; x < image.metrics().x; ++x) {
					image[y][x]				= (lineColor * (xAffect ? ::llc::max(.25, ::llc::sin(x * (1.0 / image.metrics().x * ::llc::math_pi))) : 1)).Clamp();
				}
			}
		}
	}
	return 0;
}


::llc::error_t			ghg::solarSystemReset	(::ghg::SGalaxyHell & solarSystem)	{	// Set up enemy ships
	::std::lock_guard			lock					(solarSystem.LockUpdate);
	solarSystem.ShipControllers.clear();
	llc_necs(solarSystem.ShipState.Clear());
	solarSystem.DecoState	= {};
	solarSystem.PlayState	= {};
	::llc::rasterFontDefaults(solarSystem.DecoState.FontManager);
	solarSystem.PilotsReset();
	return 0;
}


::llc::error_t			ghg::stageSetup			(::ghg::SGalaxyHell & solarSystem)	{	// Set up enemy ships
	stacxpr	uint32_t			partHealth				= 100;

	solarSystem.PlayState.TimeStage		= 0;
	solarSystem.PlayState.TimeRealStage	= 0;
	if(0 == solarSystem.PlayState.GlobalState.Stage) {
		::std::lock_guard			lock(solarSystem.LockUpdate);
		solarSystem.DecoState.Stars.Reset(solarSystem.DrawCache.RenderTargetMetrics);
		solarSystem.PlayState.GlobalState.UserTime	= {};
		solarSystem.PlayState.GlobalState.UserTime.Started = solarSystem.PlayState.GlobalState.UserTime.Loaded = ::llc::timeCurrent();
		memset(solarSystem.ShipState.SpaceshipManager.ShipScores.begin(), 0, solarSystem.ShipState.SpaceshipManager.ShipScores.byte_count());
		while(solarSystem.Pilots.size() < solarSystem.PlayState.Constants.Players) {
			sc_t						text [64]				= {};
			sprintf_s(text, "Player %i", solarSystem.Pilots.size() + 1);
			solarSystem.Pilots.push_back({::llc::label(text), PLAYER_COLORS[solarSystem.Pilots.size() % ::llc::size(PLAYER_COLORS)]});
		}
		solarSystem.ShipControllers.resize(solarSystem.PlayState.Constants.Players);
	}

#pragma pack(push, 1)
	struct SShipOrbiterSetup {
		::llc::SHIP_PART_TYPE	Type			;
		uint32_t				MaxHealth		;
		::llc::WEAPON_TYPE		Weapon			;
		double					MaxDelay		;
		double					Stability		;
		::llc::WEAPON_LOAD		Munition		;
		float					Speed			;
		int32_t					Damage			;
		uint8_t					ParticleCount	;
		double					Cooldown		;
		double					OverheatPerShot	;
		double					ShotLifetime	;
		::llc::DAMAGE_TYPE		DamageType		;

	};
#pragma pack(pop)

	::llc::SJSONFile			stageFile					= {};
	sc_t						stageFileName		[256]	= "./%s.json";
	stacxpr SShipOrbiterSetup	weaponDefinitions	[]		=
		{ {::llc::SHIP_PART_TYPE_Gun			, 128, ::llc::WEAPON_TYPE_Gun		, .08, 0.975, ::llc::WEAPON_LOAD_Bullet		,  256,    20, 1,   0,0.00,  1.5, ::llc::DAMAGE_TYPE_Pierce	}
		, {::llc::SHIP_PART_TYPE_Shotgun		, 128, ::llc::WEAPON_TYPE_Shotgun	, .16, 0.925, ::llc::WEAPON_LOAD_Bullet		,  160,    10, 6,   0,0.00,  1.5, ::llc::DAMAGE_TYPE_Impact	}
		, {::llc::SHIP_PART_TYPE_Wafer			, 128, ::llc::WEAPON_TYPE_Gun		, .24, 0.99, ::llc::WEAPON_LOAD_Ray			,  480,    30, 1,   1,0.50,  1, ::llc::DAMAGE_TYPE_Pierce	| ::llc::DAMAGE_TYPE_Burn	}
		, {::llc::SHIP_PART_TYPE_ShotgunWafer	, 128, ::llc::WEAPON_TYPE_Shotgun	, .32, 0.98, ::llc::WEAPON_LOAD_Ray			,  640,    15, 6,   1,0.50,  0.6, ::llc::DAMAGE_TYPE_Pierce	| ::llc::DAMAGE_TYPE_Burn	}
		, {::llc::SHIP_PART_TYPE_Cannon			, 160, ::llc::WEAPON_TYPE_Cannon	,   2, 1.00, ::llc::WEAPON_LOAD_Cannonball	,   48,   156, 1,   0,0.00, 10, ::llc::DAMAGE_TYPE_Impact	}
		, {::llc::SHIP_PART_TYPE_Cannon			, 128, ::llc::WEAPON_TYPE_Cannon	,   2, 0.90, ::llc::WEAPON_LOAD_Rocket		,   64,    80, 1,   0,0.00,  5, ::llc::DAMAGE_TYPE_Impact	| ::llc::DAMAGE_TYPE_Burn | ::llc::DAMAGE_TYPE_Wave }
		, {::llc::SHIP_PART_TYPE_Cannon			, 128, ::llc::WEAPON_TYPE_Cannon	, 2.5, 0.80, ::llc::WEAPON_LOAD_Missile		,   72,   112, 1,   0,0.00,  5, ::llc::DAMAGE_TYPE_Burn | ::llc::DAMAGE_TYPE_Wave }
		, {::llc::SHIP_PART_TYPE_Shotgun		, 160, ::llc::WEAPON_TYPE_Shotgun	,   3, 0.95, ::llc::WEAPON_LOAD_Cannonball	,   72,   112, 7,   0,0.00,  5, ::llc::DAMAGE_TYPE_Impact	}
		, {::llc::SHIP_PART_TYPE_Shotgun		, 128, ::llc::WEAPON_TYPE_Shotgun	,   3, 0.75, ::llc::WEAPON_LOAD_Rocket		,   80,    80, 6,   0,0.00,  5, ::llc::DAMAGE_TYPE_Pierce	| ::llc::DAMAGE_TYPE_Burn | ::llc::DAMAGE_TYPE_Wave }
		, {::llc::SHIP_PART_TYPE_Shotgun		, 128, ::llc::WEAPON_TYPE_Shotgun	, 3.5, 0.50, ::llc::WEAPON_LOAD_Missile		,   88,   128, 5,   0,0.00,7.5, ::llc::DAMAGE_TYPE_Burn | ::llc::DAMAGE_TYPE_Wave }
		, {::llc::SHIP_PART_TYPE_Gun			, 128, ::llc::WEAPON_TYPE_Gun		, .12, 0.98, ::llc::WEAPON_LOAD_Cannonball	,  160,   128, 1,   1,0.25,  5, ::llc::DAMAGE_TYPE_Impact	}
		, {::llc::SHIP_PART_TYPE_Gun			, 128, ::llc::WEAPON_TYPE_Gun		, .12, 0.75, ::llc::WEAPON_LOAD_Rocket		,  160,    80, 1,   1,0.25,  5, ::llc::DAMAGE_TYPE_Pierce	| ::llc::DAMAGE_TYPE_Burn | ::llc::DAMAGE_TYPE_Wave }
		, {::llc::SHIP_PART_TYPE_Gun			, 128, ::llc::WEAPON_TYPE_Gun		, .12, 0.50, ::llc::WEAPON_LOAD_Missile		,  160,   112, 1,   1,0.25,  5, ::llc::DAMAGE_TYPE_Burn | ::llc::DAMAGE_TYPE_Wave }
		};

	sprintf_s(stageFileName, "./levels/%u.json", solarSystem.PlayState.GlobalState.Stage + solarSystem.PlayState.PlaySetup.OffsetStage);
	if(0 <= ::llc::fileToMemory(stageFileName, stageFile.Bytes) && stageFile.Bytes.size()) {
		::std::lock_guard			lock	(solarSystem.LockUpdate);
		llc_necall(-1 == ::llc::jsonParse(stageFile.Reader, stageFile.Bytes), "%s", stageFileName);
	} 
	else {
		::std::lock_guard			lock	(solarSystem.LockUpdate);
		// Set up player ships
		if(solarSystem.PlayState.GlobalState.Stage == 0) {
			solarSystem.ShipState.ShipCoreEntity.clear();
			solarSystem.ShipState.ShipPartEntity.clear();
			solarSystem.PlayState.SimulatedTime			= {};
			solarSystem.PlayState.TimeStage				= 0;
			solarSystem.PlayState.TimeRealStage			= 0;
			llc_necs(solarSystem.ShipState.Clear());
		}
		solarSystem.ShipState.WeaponManager.Weapons.clear();
		solarSystem.ShipState.WeaponManager.Shots.clear();
	

		if(0 == solarSystem.ShipState.SpaceshipManager.ShipCores.size()) { // Create player ship
			solarSystem.PlayState.GlobalState.CameraSwitchDelay	= 0;
			solarSystem.ShipState.Scene.Global.CameraReset();

			for(uint32_t iPlayer = 0; iPlayer < solarSystem.PlayState.Constants.Players; ++iPlayer) {
				const int32_t				indexShip			= ::shipCreate(solarSystem.ShipState, 0, 0, iPlayer);
				::llc::SBodyCenter			& shipPivot			= solarSystem.ShipState.GetShipPivot(indexShip);
				shipPivot.Orientation.MakeFromEuler({0, 0, (float)(-::llc::math_pi_2)});
				shipPivot.Position		= {-30};
				shipPivot.Position.z = float(solarSystem.PlayState.Constants.Players * 30) / 2 - iPlayer * 30;
			}
		}
		// Set up enemy ships
		while(((int)solarSystem.ShipState.SpaceshipManager.ShipCores.size() - (int)solarSystem.PlayState.Constants.Players - 1) < (int)(solarSystem.PlayState.GlobalState.Stage + solarSystem.PlayState.PlaySetup.OffsetStage)) {	// Create enemy ships depending on stage.
			int32_t						indexShip				= ::shipCreate(solarSystem.ShipState, 1, -1, solarSystem.PlayState.GlobalState.Stage + solarSystem.ShipState.SpaceshipManager.ShipCores.size());
			::llc::SBodyCenter			& shipPivot				= solarSystem.ShipState.GetShipPivot(indexShip);
			shipPivot.Orientation.MakeFromEuler({0, 0, (float)(::llc::math_pi_2)});
			shipPivot.Position		= {5.0f + 5 * solarSystem.ShipState.SpaceshipManager.ShipCores.size()};

			::llc::au1_t					& enemyShipOrbiters		= solarSystem.ShipState.SpaceshipManager.ShipParts[indexShip];
			for(uint32_t iPart = 0; iPart < enemyShipOrbiters.size(); ++iPart) {
				solarSystem.ShipState.Engine.Integrator.Forces[solarSystem.ShipState.Engine.GetRigidBody(solarSystem.ShipState.ShipPartEntity[enemyShipOrbiters[iPart]])].Rotation.y *= float(1 + indexShip * .35);
			}
		}


		cnstxpr int32_t				DEFAULT_NITRO					= 1;
		// set up weapons
		for(uint32_t iShip = 0; iShip < solarSystem.ShipState.SpaceshipManager.ShipCores.size(); ++iShip) {
			::llc::SSpaceshipCore		& ship							= solarSystem.ShipState.SpaceshipManager.ShipCores[iShip];
			::llc::au1_t					& shipParts						= solarSystem.ShipState.SpaceshipManager.ShipParts[iShip];
			ship.Health				= 0;
			ship.Nitro				= {DEFAULT_NITRO, DEFAULT_NITRO};

			for(uint32_t iPart = 0; iPart < shipParts.size(); ++iPart) {
				::llc::SSpaceshipOrbiter			& shipPart						= solarSystem.ShipState.SpaceshipManager.Orbiters[shipParts[iPart]];
				//ship.Team			= iShip ? 1 : 0;
				int32_t					weapon							= 0;
				if(0 == ship.Team) {
					uint32_t				minWeapon						= solarSystem.PlayState.GlobalState.Stage / 4;
					uint32_t				maxWeapon						= ::llc::min(solarSystem.PlayState.GlobalState.Stage, ::llc::size(weaponDefinitions) - 1);

					weapon				= (int32_t)::llc::clamped(minWeapon + iPart, minWeapon, maxWeapon);
					solarSystem.ShipState.SpaceshipManager.ShipOrbiterActionQueue[shipParts[0]]->push_back(::llc::SHIP_ACTION_Spawn);

				}
				else {
					if(iShip < 4 || 0 != ((iShip - 1 - solarSystem.PlayState.Constants.Players - solarSystem.PlayState.PlaySetup.OffsetStage) % 3) || 0 != iPart) 
						weapon				= 4;
					else {
						weapon				= (iShip - 5) / 3;
					}
					weapon				%= ::llc::size(weaponDefinitions) >> 1;
				}

				SShipOrbiterSetup				partCreationData	= weaponDefinitions[weapon];
				::llc::SWeapon					newWeapon			= {};
				shipPart.Health.Value		= (int32_t)(shipPart.Health.Limit = partCreationData.MaxHealth);
				shipPart.Type				= partCreationData.Type;
				newWeapon.Trigger.Delay.Limit		= (float)partCreationData.MaxDelay;
				if(0 != ship.Team)
					newWeapon.Trigger.Delay.Limit		*= 1 + (2 * iPart);
				newWeapon.Type						= partCreationData.Weapon;
				newWeapon.Trigger.Delay.Value		= newWeapon.Trigger.Delay.Limit / shipParts.size() * iPart;
				newWeapon.Trigger.Overheat.Limit	= (float)partCreationData.Cooldown		;//= 1;
				newWeapon.Trigger.OverheatPerShot	= (float)partCreationData.OverheatPerShot	;//= 0;
				newWeapon.Shot.Type					= partCreationData.Munition;
				newWeapon.Shot.Damage				= partCreationData.Damage;
				newWeapon.Shot.Speed				= partCreationData.Speed;
				newWeapon.Shot.Stability			= (float)partCreationData.Stability;
				newWeapon.Shot.ParticleCount		= partCreationData.ParticleCount;
				newWeapon.Shot.Lifetime				= (float)partCreationData.ShotLifetime	;//= 0;

				shipPart.Weapon				= solarSystem.ShipState.WeaponManager.Weapons.push_back(newWeapon);
				solarSystem.ShipState.WeaponManager.Shots.push_back({});
				solarSystem.ShipState.WeaponManager.WeaponTargets.push_back({});

					 if(shipPart.Type == ::llc::SHIP_PART_TYPE_Gun			) { solarSystem.ShipState.GetNode(solarSystem.ShipState.GetShipPartEntity(shipParts[iPart])).Mesh = solarSystem.ShipState.GetModelMesh(::llc::SHIP_GEOMETRY_Gun);			}
				else if(shipPart.Type == ::llc::SHIP_PART_TYPE_Wafer		) { solarSystem.ShipState.GetNode(solarSystem.ShipState.GetShipPartEntity(shipParts[iPart])).Mesh = solarSystem.ShipState.GetModelMesh(::llc::SHIP_GEOMETRY_Wafer);		}
				else if(shipPart.Type == ::llc::SHIP_PART_TYPE_Shotgun		) { solarSystem.ShipState.GetNode(solarSystem.ShipState.GetShipPartEntity(shipParts[iPart])).Mesh = solarSystem.ShipState.GetModelMesh(::llc::SHIP_GEOMETRY_Shotgun);		}
				else if(shipPart.Type == ::llc::SHIP_PART_TYPE_Cannon		) { solarSystem.ShipState.GetNode(solarSystem.ShipState.GetShipPartEntity(shipParts[iPart])).Mesh = solarSystem.ShipState.GetModelMesh(::llc::SHIP_GEOMETRY_Cannon);		}
				else if(shipPart.Type == ::llc::SHIP_PART_TYPE_ShotgunWafer	) { solarSystem.ShipState.GetNode(solarSystem.ShipState.GetShipPartEntity(shipParts[iPart])).Mesh = solarSystem.ShipState.GetModelMesh(::llc::SHIP_GEOMETRY_WaferShotgun);	}
				llc_necs(solarSystem.ShipState.Engine.SetHidden(solarSystem.ShipState.GetShipPartEntity(shipParts[iPart]), false));
				ship.Health				+= shipPart.Health.Value;
				const uint32_t				width		= ::llc::get_value_count<::llc::WEAPON_LOAD>();
				const uint32_t				height		= ::llc::get_value_count<::llc::WEAPON_TYPE>();
				const uint32_t				depth		= ::llc::get_value_count<::llc::SHIP_PART_TYPE>();
				solarSystem.ShipState.GetNode(solarSystem.ShipState.GetShipPartEntity(shipParts[iPart])).Skin	= iShip * width * height * depth + shipPart.Type * width * height + newWeapon.Type * width + newWeapon.Shot.Type;
				solarSystem.ShipState.GetNode(solarSystem.ShipState.GetShipPartEntity(shipParts[iPart])).Skin	%= solarSystem.ShipState.Engine.Scene->Graphics->Skins.size();
				solarSystem.ShipState.Engine.Integrator.Centers[solarSystem.ShipState.Engine.GetRigidBody(solarSystem.ShipState.GetShipPartEntity(shipParts[iPart]))].Orientation = {0, 0, 0, 1};
			}
		}
	}

	++solarSystem.PlayState.GlobalState.Stage;
	solarSystem.PlayState.SimulatedTime.Step	= solarSystem.PlayState.Slowing;

#if defined(LLC_WINDOWS)
	//PlaySoundA((LPCSTR)SND_ALIAS_SYSTEMSTART, GetModuleHandle(0), SND_ALIAS_ID | SND_ASYNC);
#endif
	return 0;
}

::llc::error_t			ghg::solarSystemSetup	(::ghg::SGalaxyHell & solarSystem, const ::llc::n2u1_t & windowSize)	{
	llc_necs(::modelsSetup(solarSystem.ShipState));

	solarSystem.DecoState.FontManager.Fonts.clear();
	llc_necs(::llc::rasterFontDefaults(solarSystem.DecoState.FontManager));

	llc_necs(::ghg::stageSetup(solarSystem));

	::llc::m4f2_t				& matrixProjection		= solarSystem.ShipState.Scene.Global.MatrixProjection;
	matrixProjection.FieldOfView(::llc::math_pi * .25, windowSize.x / (double)windowSize.y, 0.01, 500.0);

	::llc::m4f2_t				matrixViewport			= {};
	matrixViewport.ViewportLH(windowSize.u1());
	matrixProjection		*= matrixViewport;
	return 0;
}
