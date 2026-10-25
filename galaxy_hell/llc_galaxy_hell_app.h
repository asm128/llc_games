#include "llc_galaxy_hell.h"
#include "llc_dialog_controls.h"
#include "llc_gui_control_list.h"
#include "llc_deflate.h"
#include "llc_file.h"
#include "llc_circle.h"
#include "llc_gui_inputbox.h"
#include "llc_chrono.h"

namespace ghg
{
	GDEFINE_ENUM_TYPE (UI_HOME, uint8_t);
	GDEFINE_ENUM_VALUE(UI_HOME, Continue		, 0);
	GDEFINE_ENUM_VALUE(UI_HOME, Shop			, 1);
	GDEFINE_ENUM_VALUE(UI_HOME, Save			, 2);
	GDEFINE_ENUM_VALUE(UI_HOME, Load			, 3);
	GDEFINE_ENUM_VALUE(UI_HOME, Start			, 4);
	GDEFINE_ENUM_VALUE(UI_HOME, Leaderboards	, 5);
	GDEFINE_ENUM_VALUE(UI_HOME, Credits			, 6);
	GDEFINE_ENUM_VALUE(UI_HOME, Settings		, 7);
	GDEFINE_ENUM_VALUE(UI_HOME, Exit			, 8);
	
	GDEFINE_ENUM_TYPE (UI_PROFILE, uint8_t);
	GDEFINE_ENUM_VALUE(UI_PROFILE, Name				, 0);
	GDEFINE_ENUM_VALUE(UI_PROFILE, Score			, 1);

	GDEFINE_ENUM_TYPE (UI_PLAY, uint8_t);
	GDEFINE_ENUM_VALUE(UI_PLAY, Menu			, 0);
	GDEFINE_ENUM_VALUE(UI_PLAY, Level			, 1);
	GDEFINE_ENUM_VALUE(UI_PLAY, TimeReal		, 2);
	GDEFINE_ENUM_VALUE(UI_PLAY, TimeStage		, 3);
	
	GDEFINE_ENUM_TYPE (UI_PILOT, uint8_t);
	GDEFINE_ENUM_VALUE(UI_PILOT, Name				, 0);
	GDEFINE_ENUM_VALUE(UI_PILOT, Score				, 1);
	
	GDEFINE_ENUM_TYPE (UI_PLAYER_SCORE, uint8_t);
	GDEFINE_ENUM_VALUE(UI_PLAYER_SCORE, Score				, 0);
	GDEFINE_ENUM_VALUE(UI_PLAYER_SCORE, Hits				, 1);
	GDEFINE_ENUM_VALUE(UI_PLAYER_SCORE, Shots				, 2);
	GDEFINE_ENUM_VALUE(UI_PLAYER_SCORE, Bullets				, 3);
	GDEFINE_ENUM_VALUE(UI_PLAYER_SCORE, DamageDone			, 4);
	GDEFINE_ENUM_VALUE(UI_PLAYER_SCORE, DamageReceived		, 5);
	GDEFINE_ENUM_VALUE(UI_PLAYER_SCORE, HitsSurvived		, 6);
	GDEFINE_ENUM_VALUE(UI_PLAYER_SCORE, OrbitersLost		, 7);
	GDEFINE_ENUM_VALUE(UI_PLAYER_SCORE, KilledShips			, 8);
	GDEFINE_ENUM_VALUE(UI_PLAYER_SCORE, KilledOrbiters		, 9);
	//GDEFINE_ENUM_VALUE(UI_PLAYER_SCORE, TimeReal			,10);
	//GDEFINE_ENUM_VALUE(UI_PLAYER_SCORE, TimeSim			,11);

	GDEFINE_ENUM_TYPE (UI_LOAD, uint8_t	);
	GDEFINE_ENUM_TYPE (UI_DESKTOP, uint8_t);

	GDEFINE_ENUM_TYPE (UI_SETTINGS, uint8_t);
	GDEFINE_ENUM_VALUE(UI_SETTINGS, Back		, 0);
	GDEFINE_ENUM_VALUE(UI_SETTINGS, Game		, 1);
	GDEFINE_ENUM_VALUE(UI_SETTINGS, Graphics	, 2);
	GDEFINE_ENUM_VALUE(UI_SETTINGS, Audio		, 3);

	GDEFINE_ENUM_TYPE (UI_SHOP, uint8_t);
	GDEFINE_ENUM_VALUE(UI_SHOP, Back			, 0);

	GDEFINE_ENUM_TYPE (UI_WELCOME, uint8_t);
	GDEFINE_ENUM_VALUE(UI_WELCOME, Confirm		, 0);

	GDEFINE_ENUM_TYPE (UI_CREDITS, uint8_t);
	GDEFINE_ENUM_VALUE(UI_CREDITS, Back			, 0);
	GDEFINE_ENUM_VALUE(UI_CREDITS, __________	, 1);
	GDEFINE_ENUM_VALUE(UI_CREDITS, Idea			, 2);
	GDEFINE_ENUM_VALUE(UI_CREDITS, Gontrill		, 3);
	GDEFINE_ENUM_VALUE(UI_CREDITS, _________	, 4);
	GDEFINE_ENUM_VALUE(UI_CREDITS, Programming	, 5);
	GDEFINE_ENUM_VALUE(UI_CREDITS, asm128		, 6);
	GDEFINE_ENUM_VALUE(UI_CREDITS, ________		, 7);
	GDEFINE_ENUM_VALUE(UI_CREDITS, Sounds		, 8);
	GDEFINE_ENUM_VALUE(UI_CREDITS, Microph		, 9);
	GDEFINE_ENUM_VALUE(UI_CREDITS, _______		, 10);
	GDEFINE_ENUM_VALUE(UI_CREDITS, Graphics		, 11);
	GDEFINE_ENUM_VALUE(UI_CREDITS, PabloAriel	, 12);
	GDEFINE_ENUM_VALUE(UI_CREDITS, ______		, 13);
	GDEFINE_ENUM_VALUE(UI_CREDITS, Home			, 14);

	GDEFINE_ENUM_TYPE(UI_BRIEF, uint8_t);
	GDEFINE_ENUM_TYPE(UI_STAGE, uint8_t);
	GDEFINE_ENUM_TYPE(UI_STATS, uint8_t);
	GDEFINE_ENUM_TYPE(UI_STORE, uint8_t);
	GDEFINE_ENUM_TYPE(UI_SCORE, uint8_t);

	struct SUIRadialGauge {
		::llc::apod<::llc::n3f2_t>		Vertices				= {};
		::llc::apod<::llc::tris1_t>		Indices					= {};
		int16_t							CurrentValue			= 0;
		int16_t							MaxValue				= 64;
		::llc::img<::llc::bgra>			Image					= {};

		stacxpr	::llc::rgbaf			COLOR_MIN_DEFAULT		= ::llc::GREEN;
		stacxpr	::llc::rgbaf			COLOR_MID_DEFAULT		= ::llc::ORANGE;
		stacxpr	::llc::rgbaf			COLOR_MAX_DEFAULT		= ::llc::RED;
		stacxpr	::llc::rgbaf			COLOR_BKG_DEFAULT		= ::llc::BROWN * .5 + ::llc::DARKBLUE * .5;
		
		inline	int16_t					SetValue				(float weight)							noexcept	{ return CurrentValue = int16_t(weight * MaxValue); }
				int16_t					SetValue				(float currentValue, uint32_t maxValue)	noexcept	{ return SetValue(currentValue / maxValue); }
	};

			::llc::error_t			gaugeBuildRadial		(::ghg::SUIRadialGauge & gauge, const ::llc::circlef2_t & gaugeMetrics, int16_t resolution, int16_t width);
			::llc::error_t			gaugeImageUpdate		(::ghg::SUIRadialGauge & gauge, ::llc::g8bgra target, ::llc::rgbaf colorMin = ::ghg::SUIRadialGauge::COLOR_MIN_DEFAULT, ::llc::rgbaf colorMid = ::ghg::SUIRadialGauge::COLOR_MID_DEFAULT, ::llc::rgbaf colorMax = ::ghg::SUIRadialGauge::COLOR_MAX_DEFAULT, ::llc::bgra colorEmpty = ::ghg::SUIRadialGauge::COLOR_BKG_DEFAULT, bool radialColor = false);
	inline	::llc::error_t			gaugeImageUpdate		(::ghg::SUIRadialGauge & gauge, ::llc::rgbaf colorMin = ::ghg::SUIRadialGauge::COLOR_MIN_DEFAULT, ::llc::rgbaf colorMid = ::ghg::SUIRadialGauge::COLOR_MID_DEFAULT, ::llc::rgbaf colorMax = ::ghg::SUIRadialGauge::COLOR_MAX_DEFAULT, ::llc::bgra colorEmpty = ::ghg::SUIRadialGauge::COLOR_BKG_DEFAULT, bool radialColor = false) {
		//gauge.Image.Texels.fill({0, 0, 0, 0});
		return ::ghg::gaugeImageUpdate(gauge, gauge.Image, colorMin, colorMid, colorMax, colorEmpty, radialColor);
	}

	struct SUIPlayShipPartViewport {
		::llc::cid_t					Viewport		;
		::llc::SCameraPoints			Camera			;
		SUIRadialGauge					GaugeLife		;
		SUIRadialGauge					GaugeCooldown	;
		SUIRadialGauge					GaugeDelay		;
		::llc::m4f2_t					MatrixProjection;
		::ghg::TRenderTarget			RenderTargetOrbiter	;
		::ghg::TRenderTarget			RenderTargetWeaponLoad	;
		::ghg::TRenderTarget			RenderTargetWeaponType	;
	};


	GDEFINE_ENUM_TYPE (SHIP_CONTROLLER, uint16_t);
	GDEFINE_ENUM_VALUE(SHIP_CONTROLLER, Forward	, 0x01);
	GDEFINE_ENUM_VALUE(SHIP_CONTROLLER, Back	, 0x02);
	GDEFINE_ENUM_VALUE(SHIP_CONTROLLER, Left	, 0x03);
	GDEFINE_ENUM_VALUE(SHIP_CONTROLLER, Right	, 0x04);
	GDEFINE_ENUM_VALUE(SHIP_CONTROLLER, Turbo	, 0x05);

	struct SUIPlayer {
		::llc::apobj<::ghg::SUIPlayShipPartViewport>	
								ModuleViewports			= {};
		::llc::SDialog			DialogPlay				= {};
		::llc::SDialog			DialogHome				= {};

		::llc::acid				ControlsDialogPlay		= {};
		::llc::acid				ControlsDialogHome		= {};

		::llc::SInputBox		InputBox				= {};
	
		::llc::astc<128>		TextScore				= {};
		::llc::astc<128>		TextHits				= {};
		::llc::astc<128>		TextShots				= {};
		::llc::astc<128>		TextBullets				= {};
		::llc::astc<128>		TextDamageDone			= {};
		::llc::astc<128>		TextDamageReceived		= {};
		::llc::astc<128>		TextHitsSurvived		= {};
		::llc::astc<128>		TextOrbitersLost		= {};
		::llc::astc<128>		TextKilledShips			= {};
		::llc::astc<128>		TextKilledOrbiters		= {};
	};

	struct SUIPlay {
		::llc::aobj<::ghg::SUIPlayer>	PlayerUI				= {};
		::ghg::SGalaxyHellDrawCache		DrawCache				= {};

		::llc::astatic<sc_t, 128>		TextLevel				= {};
		::llc::astatic<sc_t, 128>		TextTimeStage			= {};
		::llc::astatic<sc_t, 128>		TextTimeWorld			= {};
		::llc::astatic<sc_t, 128>		TextTimeReal			= {};
	};


	struct SPlayerShip { 
		::llc::SSpaceshipCore			Core;
		::llc::SSpaceshipScore			Score;
		::llc::au2_t						Parts;
		::llc::au2_t						Weapons;

		::llc::error_t					Save					(::llc::au0_t & output) const {
			llc_necs(::llc::savePOD (output, Core	));
			llc_necs(::llc::savePOD (output, Score	));
			llc_necs(::llc::saveView(output, Parts	));
			llc_necs(::llc::saveView(output, Weapons));
			return 0;
		}

		::llc::error_t					Load					(::llc::vcu0_t & input) {
			llc_necs(::llc::loadPOD	(input, Core	));
			llc_necs(::llc::loadPOD	(input, Score	));
			llc_necs(::llc::loadView(input, Parts	));
			llc_necs(::llc::loadView(input, Weapons	));
			return 0;
		}
	};


#pragma pack(push, 1)
	GDEFINE_ENUM_TYPE (PLAYER_TYPE, uint8_t);
	GDEFINE_ENUM_VALUE(PLAYER_TYPE, Human	, 0);
	GDEFINE_ENUM_VALUE(PLAYER_TYPE, AI		, 1);
	GDEFINE_ENUM_VALUE(PLAYER_TYPE, Network	, 2);

	struct SPlayerScore { 
		uint64_t			TimePlayed;
		uint64_t			TimeDead;
		uint64_t			KeyPresses;
	};

	struct SPlayerState {
		PLAYER_TYPE			Type;
		::llc::bgra			Color;
		int32_t				ShipSelected;
		::ghg::SPlayerScore	Score;
	};
#pragma pack(pop)

	struct SPlayer {
		::llc::vcst_t						Name			= {};
		::ghg::SPlayerState				State			= {};
		::llc::apod<::llc::SWeapon>		Weapons			= {};
		::llc::apod<::llc::SSpaceshipOrbiter>	Orbiters		= {};
		::llc::aobj<::ghg::SPlayerShip>	Ships			= {};

		::llc::error_t					Save			(const ::llc::vcst_t & filename) const {
			::llc::au0_t							serialized;
			llc_necs(Save(serialized));
			::llc::au0_t							deflated;
			llc_necs(::llc::arrayDeflate(serialized, deflated));
			info_printf("Player size in bytes: %u.", serialized.size());
			info_printf("Player file size: %u.", deflated.size());
			llc_necall(::llc::fileFromMemory({filename}, deflated), "fileName: %s", ::llc::label(filename).begin());
			return 0;
		}

		::llc::error_t					Load			(const ::llc::vcst_t & filename) {
			::llc::au0_t							serialized;
			llc_necall(::llc::fileToMemory({filename}, serialized), "fileName: %s", ::llc::label(filename).begin());
			::llc::au0_t							inflated;
			llc_necs(::llc::arrayInflate(serialized, inflated));
			info_printf("Player file size: %u.", inflated.size());
			info_printf("Player size in bytes: %u.", serialized.size());
			::llc::vcu0_t							input		= inflated;
			return Load(input);
		}

		::llc::error_t					Save			(::llc::au0_t & output) const {
			llc_necs(::llc::saveView(output, Name));
			llc_necs(::llc::savePOD(output, State));
			llc_necs(::llc::saveView(output, Weapons));
			llc_necs(::llc::saveView(output, Orbiters));
			llc_necs(output.append(::llc::vcu0_t{(const uint8_t*)&Ships.size(), 4}));
			for(uint32_t iShip = 0; iShip < Ships.size(); ++iShip) {
				llc_necall(Ships[iShip].Save(output), "iShip: %u", iShip);
			}
			return 0;
		}

		::llc::error_t					Load			(::llc::vcu0_t & input) {
			llc_necs(::llc::loadLabel	(input, Name	));
			llc_necs(::llc::loadPOD		(input, State	));
			llc_necs(::llc::loadView	(input, Weapons	));
			llc_necs(::llc::loadView	(input, Orbiters));
			llc_necs(Ships.resize(*(uint32_t*)input.begin()));
			input							= {input.begin() + sizeof(uint32_t), input.size() - 4};
			for(uint32_t iShip = 0; iShip < Ships.size(); ++iShip) {
				llc_necall(Ships[iShip].Load(input), "iShip: %u", iShip);
			}
			return 0;
		}
	};

	struct SPlayerManager {

	};

	struct SUserCredentials {
		::llc::vcst_t		Username;
		::llc::vcst_t		Password;
	};

	enum SAVE_MODE { SAVE_MODE_AUTO, SAVE_MODE_QUICK, SAVE_MODE_STAGE, SAVE_MODE_USER };

	GDEFINE_ENUM_TYPE(APP_STATE, uint8_t);
	GDEFINE_ENUM_VALUE(APP_STATE, Init		,  0);
	GDEFINE_ENUM_VALUE(APP_STATE, Welcome	,  1);
	GDEFINE_ENUM_VALUE(APP_STATE, Home		,  2);
	GDEFINE_ENUM_VALUE(APP_STATE, Profile	,  3);
	GDEFINE_ENUM_VALUE(APP_STATE, Shop		,  4);
	GDEFINE_ENUM_VALUE(APP_STATE, Play		,  5);
	GDEFINE_ENUM_VALUE(APP_STATE, Brief		,  6);
	GDEFINE_ENUM_VALUE(APP_STATE, Stage		,  7);
	GDEFINE_ENUM_VALUE(APP_STATE, Stats		,  8);
	GDEFINE_ENUM_VALUE(APP_STATE, Store		,  9);
	GDEFINE_ENUM_VALUE(APP_STATE, Score		, 10);
	GDEFINE_ENUM_VALUE(APP_STATE, About		, 11);
	GDEFINE_ENUM_VALUE(APP_STATE, Settings	, 12);
	GDEFINE_ENUM_VALUE(APP_STATE, Load		, 13);
	GDEFINE_ENUM_VALUE(APP_STATE, Quit		, 14);
	GDEFINE_ENUM_VALUE(APP_STATE, COUNT		, 15);

	struct SGalaxyHellApp {
		::llc::apobj<TRenderTarget>			RenderTargetPool			= {};
		::llc::apobj<TRenderTarget>			RenderTargetQueue			= {};
		::std::mutex						RenderTargetLockPool		= {};
		::std::mutex						RenderTargetLockQueue		= {};
		vltl	uint64_t					CurrentRenderTarget			= 0;
		int32_t								PlayerSelected				= 0;

		bool								Exit						= false;
		::llc::astatic<::llc::SDialog, ::ghg::APP_STATE_COUNT>	
											DialogPerState				= {};

		::llc::astatic<::llc::acid, APP_STATE_COUNT>	
											DialogControls				= {}; 	

		::llc::SVirtualKeyboard				VirtualKeyboard				= {};
		::ghg::SUIPlay						UIPlay;

		::llc::SDialog						DialogDesktop				= {};
		::llc::aobj<::ghg::SPlayer>			Players						= {};
		::llc::pnco<::llc::SDialogTuner<uint8_t>>	TunerPlayerCount;
		::llc::acid							DesktopControls			= {}; 	

		::ghg::SGalaxyHell						Game;
		::llc::aobj<::ghg::SUserCredentials>	UserCredentials				= {};

		::llc::pobj<::llc::SDialogViewport>	Inputbox;
		::llc::apod<sc_t>					InputboxText;

		::llc::aobj<::llc::apod<sc_t>>	FileNames					= {};
		::llc::vcs						SavegameFolder				= ".";
		::llc::vcs						ExtensionSaveAuto			= ".auto.ghz";
		::llc::vcs						ExtensionSaveStage			= ".stage.ghz";
		::llc::vcs						ExtensionSaveQuick			= ".quick.ghz";
		::llc::vcs						ExtensionSaveUser			= ".user.ghz";
		::llc::vcs						ExtensionSave				= ".ghz";
		::llc::vcs						ExtensionImages				= ".png";

		::llc::vcs						ExtensionProfile			= ".ghp";
		::llc::vcs						ExtensionCredentials		= ".ghc";

		::ghg::APP_STATE				ActiveState					= APP_STATE_Init;

		::llc::error_t					AddNewPlayer				(::llc::vcst_t playerName)			{
			::llc::trim(playerName);
			return Players.push_back({::llc::label(::llc::vcs{playerName})});
		}

		::llc::error_t					Save						(SAVE_MODE autosaveMode)				{
#if GH_SAVE_ENABLED
			sc_t								fileName[4096]				= {};
			::llc::au0_t							b64PlayerName				= {};
			for(uint32_t iPlayer = 0; iPlayer < Players.size(); ++iPlayer) {
				b64PlayerName.clear();
				const ::ghg::SPlayer				& player					= Players[iPlayer];
				::llc::base64Encode(player.Name, b64PlayerName);
				sprintf_s(fileName, "%s/%s.%llu%s", SavegameFolder.begin(), b64PlayerName.begin(), 0ULL, ExtensionProfile.begin());
				llc_necall(player.Save(fileName), "iPlayer: %i", iPlayer);
			}
			::llc::vcst_t							extension;
			switch(autosaveMode) {
			case SAVE_MODE_USER		: extension = ExtensionSaveUser		; break;
			case SAVE_MODE_STAGE	: extension = ExtensionSaveStage	; break;
			case SAVE_MODE_QUICK	: extension = ExtensionSaveQuick	; break;
			case SAVE_MODE_AUTO		: extension = ExtensionSaveAuto		; break;
			}

			const uint64_t						timeCurrent					= (SAVE_MODE_USER != autosaveMode) ? 0 : ::llc::timeCurrent();
			const uint64_t						timeStart					= (SAVE_MODE_AUTO == autosaveMode) ? 0 : Game.PlayState.GlobalState.UserTime.Started;
			sprintf_s(fileName, "%s/%llu.%llu%s", SavegameFolder.begin(), timeStart, timeCurrent, extension.begin());

			const int32_t						totalHealth					= Game.ShipState.SpaceshipManager.GetTeamHealth(0);
			if(totalHealth > 0) // Save only if a player is alive
				return ::ghg::solarSystemSave(Game, fileName);
#else
			(void)autosaveMode;
#endif
			return 0;
		}
	};

	::llc::error_t	guiSetup			(::ghg::SGalaxyHellApp & gameui, const ::llc::pobj<::llc::SInput> & inputState);
	::llc::error_t	guiUpdate			(::ghg::SGalaxyHellApp & gameui, ::llc::vpobj<::llc::SEventSystem> sysEvents);
	::llc::err_t		galaxyHellToggleRenderer(::ghg::SGalaxyHellApp & app);
	
	::llc::error_t	galaxyHellUpdate	(::ghg::SGalaxyHellApp & app, double lastTimeSeconds, const ::llc::pobj<::llc::SInput> & inputState, ::llc::vpobj<::llc::SEventSystem> systemEventsNew);
	::llc::error_t	galaxyHellDraw		(::ghg::SGalaxyHellApp & app, ::llc::n2u1_t renderTargetSize);

	::llc::error_t	listFilesSavegame	(::ghg::SGalaxyHellApp & app, const ::llc::vcst_t & saveGameFolder, ::llc::aobj<::llc::vcst_t> & savegameFilenames);
	::llc::error_t	listFilesProfile	(::ghg::SGalaxyHellApp & app, const ::llc::vcst_t & saveGameFolder, ::llc::aobj<::llc::vcst_t> & savegameFilenames);

}
