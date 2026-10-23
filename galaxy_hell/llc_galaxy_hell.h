#include "llc_galaxy_hell_ships.h"

#include "llc_galaxy_hell_deco.h"

#include "llc_galaxy_hell_draw.h"

#include "llc_game.h"

#include "llc_view_color.h"

#include "llc_input.h"
#include "llc_label.h"

#include "llc_system_event.h"

#include <mutex>

#ifndef LLC_GALAXY_HELL_H_293874239874
#define LLC_GALAXY_HELL_H_293874239874

namespace ghg
{
	LLC_USING_TYPEINT();
	
	stacxpr uint16_t	MAX_PLAYERS				= 4;
	stacxpr uint32_t	MAX_ORBITER_COUNT		= 6;

#pragma pack(push, 1)
	stacxpr ::llc::astatic<::llc::bgra, 16>	PLAYER_COLORS	= 
		{ 0xFF00FF00U	// GREEN
		, ::llc::PURPLE
		, 0xFF0000FFU	// BLUE
		, 0xFFFFFF00U	// CYAN
		, 0xFFFF00FFU	// MAGENTA
		, 0xFF00FFFFU	// YELLOW
		, 0xFFFF0000U	// RED
		};

	GDEFINE_ENUM_TYPE(PLAY_TYPE, uint8_t);
	GDEFINE_ENUM_VALUE(PLAY_TYPE, FreeRoam		, 0);
	GDEFINE_ENUM_VALUE(PLAY_TYPE, Campaign		, 1);
	GDEFINE_ENUM_VALUE(PLAY_TYPE, Deathmatch	, 2);
	GDEFINE_ENUM_VALUE(PLAY_TYPE, CTF			, 3);
	GDEFINE_ENUM_VALUE(PLAY_TYPE, Speedrun		, 4);
	GDEFINE_ENUM_VALUE(PLAY_TYPE, Ambush		, 5);
	GDEFINE_ENUM_VALUE(PLAY_TYPE, Circuit		, 6);
	GDEFINE_ENUM_VALUE(PLAY_TYPE, Defense		, 7);
	GDEFINE_ENUM_VALUE(PLAY_TYPE, Capture		, 8);
	GDEFINE_ENUM_VALUE(PLAY_TYPE, Assassination	, 9);
	GDEFINE_ENUM_VALUE(PLAY_TYPE, Worldscape	, 0xA);
	GDEFINE_ENUM_VALUE(PLAY_TYPE, Skyscape		, 0xB);
	GDEFINE_ENUM_VALUE(PLAY_TYPE, Seascape		, 0xC);
	GDEFINE_ENUM_VALUE(PLAY_TYPE, Orbitscape	, 0xD);
	GDEFINE_ENUM_VALUE(PLAY_TYPE, Systemscape	, 0xE);
	GDEFINE_ENUM_VALUE(PLAY_TYPE, Tactical		, 0xF);
	GDEFINE_ENUM_VALUE(PLAY_TYPE, Interception	, 0x10);
	GDEFINE_ENUM_VALUE(PLAY_TYPE, Excavation	, 0x11);
	GDEFINE_ENUM_VALUE(PLAY_TYPE, Rescue		, 0x12);
	GDEFINE_ENUM_VALUE(PLAY_TYPE, Sabotage		, 0x13);
	GDEFINE_ENUM_VALUE(PLAY_TYPE, Spy			, 0x14);
	GDEFINE_ENUM_VALUE(PLAY_TYPE, Extermination	, 0x15);
	GDEFINE_ENUM_VALUE(PLAY_TYPE, Assault		, 0x16);
	GDEFINE_ENUM_VALUE(PLAY_TYPE, Duel			, 0x17);
	GDEFINE_ENUM_VALUE(PLAY_TYPE, Hive			, 0x18);
	GDEFINE_ENUM_VALUE(PLAY_TYPE, Disruption	, 0x19);
	GDEFINE_ENUM_VALUE(PLAY_TYPE, Survival		, 0x1A);
	GDEFINE_ENUM_VALUE(PLAY_TYPE, Escort		, 0x1B);
	//GDEFINE_ENUM_VALUE(PLAY_TYPE, , 0x1C);
	//GDEFINE_ENUM_VALUE(PLAY_TYPE, , 0x1D);
	//GDEFINE_ENUM_VALUE(PLAY_TYPE, , 0x1E);
	//GDEFINE_ENUM_VALUE(PLAY_TYPE, , 0x1F);

	GDEFINE_ENUM_TYPE(PLAY_MODE, u0_t);
	GDEFINE_ENUM_VALUE(PLAY_MODE, Arcade		, 0);
	GDEFINE_ENUM_VALUE(PLAY_MODE, VR			, 1);

	struct SPlayState {
		::llc::SGameSetup		PlaySetup				= {2};
		PLAY_MODE				PlayMode				= (PLAY_MODE)PLAY_MODE_VR;

		double					TimeStage				= 0;
		double					TimeRealStage			= 0;

		::llc::SGameState		GlobalState;

		::llc::SStageSetup		Constants				= {1, 1};
		::llc::SRelativeSpeed	BackgroundSpeed;
		::llc::SRelativeTime	SimulatedTime;

		stacxpr	float			Slowing					= -.35f;
		stacxpr	float			Fasting					= .45f;
	};
	 
	struct SShipController {
		uint8_t				Forward					: 1;
		uint8_t				Back					: 1;
		uint8_t				Left					: 1;
		uint8_t				Right					: 1;
		uint8_t				Turbo					: 1;
	};
#pragma pack(pop)

	struct SShipPilot : ::llc::SPlayer {};

	struct SGalaxyHell {
		::ghg::SShipManager					ShipState				= {};
		::ghg::SDecoState					DecoState				= {};	
		::ghg::SPlayState					PlayState				= {};
		::llc::aobj<::ghg::SShipPilot>		Pilots					= {};
		::llc::apod<::ghg::SShipController>	ShipControllers			= {};

		::ghg::SGalaxyHellDrawCache			DrawCache;
		::std::mutex						LockUpdate;

		::llc::error_t						PilotCreate				(const ::ghg::SShipPilot & shipPilot)			{
			llc_necs(Pilots.push_back(shipPilot));
			return ShipControllers.push_back({});
		}

		::llc::error_t						PilotsReset				()			{
			while(Pilots.size() < PlayState.Constants.Players) {
				sc_t									text [64]				= {};
				sprintf_s(text, "Player %i", Pilots.size() + 1);
				llc_necs(Pilots.push_back({::llc::label(text), PLAYER_COLORS[Pilots.size() % ::llc::size(PLAYER_COLORS)]}));
			}
			return ShipControllers.resize(PlayState.Constants.Players);
		}

#if GH_SAVE_ENABLED
		::llc::error_t						Save					(::llc::au0_t & output)		const	{
			llc_necs(output.append({'L', 'G', 'H', '2'}));
			llc_necs(::llc::savePOD(output, PlayState));
			for(uint32_t iPlayer = 0; iPlayer < PlayState.Constants.Players; ++iPlayer) {
				llc_necs(::llc::saveView(output, Pilots[iPlayer].Name));
				llc_necs(::llc::savePOD(output, Pilots[iPlayer].Color));
			}
			llc_necs(::llc::saveView(output, ShipControllers));
			llc_necs(ShipState.Save(output));
			return 0;
		}

		::llc::error_t						Load					(::llc::vcu0_t & input) {
			::std::lock_guard						lock(LockUpdate);
			if_true_vwf(-1, input.size() < 4 || memcmp(input.begin(), "LGH2", 4), "%s", "Unsupported Galaxy Hell save format.");
			llc_necs(input.slice(input, 4));
			::llc::view<const ::ghg::SPlayState>	readPlayState			= {};
			llc_necs(::llc::loadPOD(input, PlayState));
			llc_necs(Pilots.resize(PlayState.Constants.Players));
			for(uint32_t iPlayer = 0; iPlayer < Pilots.size(); ++iPlayer) {
				llc_necall(::llc::loadLabel(input, Pilots[iPlayer].Name), "iPlayer %i", iPlayer);
				llc_necall(::llc::loadPOD(input, Pilots[iPlayer].Color), "iPlayer %i", iPlayer);
			}
			ShipControllers.clear();
			llc_necs(::llc::loadView(input, ShipControllers));
			llc_necs(ShipState.Load(input));
			return 0;
		}
#endif
	};

	::llc::error_t		stageSetup				(::ghg::SGalaxyHell & solarSystem);
	::llc::err_t			getShipRenderNodes		(const ::ghg::SGalaxyHell & game, ::llc::u2_t ship, ::llc::au2_t & nodes);
	::llc::err_t			getShipLabelCoords		(const ::ghg::SGalaxyHell & game, ::llc::s2_t ship, const ::llc::m4f2_t & matrixVP, const ::llc::n2u2_t & targetMetrics, const ::llc::SRasterFont & font, ::llc::an2u1_t & coords);
	::llc::err_t			drawShipLabel			(const ::ghg::SGalaxyHell & game, ::llc::s2_t ship, const ::llc::m4f2_t & matrixVP, ::llc::g8bgra target, const ::llc::SRasterFont & font);
	::llc::error_t		solarSystemSetup		(::ghg::SGalaxyHell & solarSystem, const ::llc::n2u1_t & windowSize);
	::llc::error_t		solarSystemReset		(::ghg::SGalaxyHell & solarSystem); 
	::llc::error_t		solarSystemDraw			(const ::ghg::SGalaxyHell & solarSystem, ::ghg::SGalaxyHellDrawCache & drawCache, ::std::mutex & lockUpdate);
	::llc::error_t		solarSystemUpdate		(::ghg::SGalaxyHell & solarSystem, double secondsLastFrame, const ::llc::SInput & input, ::llc::vpobj<::llc::SEventSystem> frameEvents);
#if GH_SAVE_ENABLED
	::llc::error_t		solarSystemLoad			(::ghg::SGalaxyHell & world,::llc::vcst_t filename);
	::llc::error_t		solarSystemSave			(const ::ghg::SGalaxyHell & world,::llc::vcst_t filename);
#endif
	
	::llc::error_t		getLightArraysFromDebris
		( const ::ghg::SDecoState	& decoState
		, ::llc::an3f2_t				& lightPoints
		, ::llc::a8bgra				& lightColors
		, const ::llc::vc8bgra		& debrisColors
		);

	::llc::error_t		getLightArraysFromShips
		( const ::ghg::SShipManager	& shipState
		, ::llc::an3f2_t				& lightPoints
		, ::llc::a8bgra				& lightColors
		);

	::llc::error_t		getLightArrays
		( const ::ghg::SShipManager	& shipState
		, const ::ghg::SDecoState	& decoState
		, ::llc::an3f2_t				& lightPoints
		, ::llc::a8bgra				& lightColors
		, const ::llc::vc8bgra		& debrisColors
		);

	::llc::error_t		drawOrbiter
		( const ::ghg::SShipManager		& shipState
		, const uint32_t				iPartEntity
		, const ::llc::rgbaf			& shipColor	
		, const float					animationTime
		, const ::llc::m4f2_t			& matrixVP
		, ::llc::g8bgra					& targetPixels
		, ::llc::gu2_t					depthBuffer
		, ::ghg::SGalaxyHellDrawCache	& drawCache
		);
}

#endif // LLC_GALAXY_HELL_H_293874239874
