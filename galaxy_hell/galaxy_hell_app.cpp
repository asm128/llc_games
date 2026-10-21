#include "llc_galaxy_hell_app.h"
#include "llc_grid_copy.h"
#include "llc_raster_lh.h"
#include "llc_file.h"
#include "llc_deflate.h"
#include "llc_path.h"
#include "llc_event_screen.h"

using ::llc::get_value_namep, ::llc::get_enum_namep, ::llc::failed;
LLC_USING_TYPEINT();

static ::llc::error_t updateAllGuiMetrics(::ghg::SGalaxyHellApp & app, ::llc::n2u1_t targetSize) {
	::std::lock_guard lockUpdate(app.Game.LockUpdate);
	if(app.DialogDesktop.GUI)
		llc_necs(::llc::guiUpdateMetrics(*app.DialogDesktop.GUI, targetSize, true));
	for(uint32_t iState = 0; iState < app.DialogPerState.size(); ++iState)
		if(app.DialogPerState[iState].GUI)
			llc_necs(::llc::guiUpdateMetrics(*app.DialogPerState[iState].GUI, targetSize, true));
	for(uint32_t iPlayer = 0; iPlayer < app.UIPlay.PlayerUI.size(); ++iPlayer) {
		if(app.UIPlay.PlayerUI[iPlayer].DialogHome.GUI)
			llc_necs(::llc::guiUpdateMetrics(*app.UIPlay.PlayerUI[iPlayer].DialogHome.GUI, targetSize, true));
		if(app.UIPlay.PlayerUI[iPlayer].DialogPlay.GUI)
			llc_necs(::llc::guiUpdateMetrics(*app.UIPlay.PlayerUI[iPlayer].DialogPlay.GUI, targetSize, true));
	}
	return 0;
}

static	::llc::error_t	processScreenEvent		(::ghg::SGalaxyHellApp & app, const ::llc::SEventView<::llc::EVENT_SCREEN> & screenEvent) { 
	switch(screenEvent.Type) {
	default: break;
	case ::llc::EVENT_SCREEN_Close:
	case ::llc::EVENT_SCREEN_Deactivate: 
		if(app.ActiveState != ::ghg::APP_STATE_Welcome) {
			app.Save(::ghg::SAVE_MODE_AUTO);
			app.ActiveState						= ::ghg::APP_STATE_Home;
		}
		break;
	case ::llc::EVENT_SCREEN_Create:
	case ::llc::EVENT_SCREEN_Resize: {
		::llc::n2<uint16_t>			newMetrics				= *(const ::llc::n2<uint16_t>*)screenEvent.Data.begin();
		llc_necs(::updateAllGuiMetrics(app, newMetrics));

		app.RenderTargetPool.resize(16);
		for(uint32_t iRT = 0; iRT < 16; ++iRT) {
			app.RenderTargetPool[iRT]->resize(newMetrics);
		}

		app.Game.DrawCache.RenderTargetMetrics = newMetrics;
		double						currentRatio			= app.Game.DrawCache.RenderTargetMetrics.y / (double)app.Game.DrawCache.RenderTargetMetrics.x;
		double						targetRatioY			=  9 / 16.0;
		if(currentRatio >= targetRatioY)
			app.Game.DrawCache.RenderTargetMetrics.y = (uint16_t)(app.Game.DrawCache.RenderTargetMetrics.x * targetRatioY + .1f);
		else 
			app.Game.DrawCache.RenderTargetMetrics.x = (uint16_t)(app.Game.DrawCache.RenderTargetMetrics.y / targetRatioY + .1f);
		}
		break;
	}
	return 0;
}

static	::llc::error_t	processTextEvent		(::ghg::SGalaxyHellApp & app, const ::llc::SEventView<::llc::EVENT_TEXT> & screenEvent) { 
	switch(screenEvent.Type) {
	default: break;
	case ::llc::EVENT_TEXT_Char:
		if(screenEvent.Data[0] >= 0x20 && screenEvent.Data[0] <= 0x7F) {
			if(app.ActiveState == ::ghg::APP_STATE_Welcome)
				app.InputboxText.push_back(screenEvent.Data[0]);
		}
		break;
	}
	return 0;
}

static	::llc::error_t	processKeyboardEvent	(::ghg::SGalaxyHellApp & app, const ::llc::SEventView<::llc::EVENT_KEYBOARD> & screenEvent) { 
	switch(screenEvent.Type) {
	default: break;
	case ::llc::EVENT_KEYBOARD_Down:
		if(screenEvent.Data[0] == VK_ESCAPE) {
			if(app.ActiveState != ::ghg::APP_STATE_Home) {
				app.Save(::ghg::SAVE_MODE_AUTO);
				app.ActiveState				= ::ghg::APP_STATE_Home;
			}
			else { 
				bool editing = false;
				for(uint32_t iPlayer = 0; iPlayer < app.TunerPlayerCount->ValueCurrent; ++iPlayer) {
					if(app.UIPlay.PlayerUI[iPlayer].InputBox.Editing) {
						editing = true;
						break;
					}
				}
				if(!editing) {
					app.Game.PlayState.GlobalState.Paused	= false;
					app.ActiveState				= ::ghg::APP_STATE_Play;
				}
			}
		}
		break;
	}
	return 0;
}

static	::llc::error_t	processSystemEvent		(::ghg::SGalaxyHellApp & app, const ::llc::SEventSystem & sysEvent) { 
	switch(sysEvent.Type) {
	default: break;
	case ::llc::SYSTEM_EVENT_Screen		: if_true_e(failed(::llc::eventExtractAndHandle<::llc::EVENT_SCREEN	>(sysEvent, [&app](auto ev) { return processScreenEvent		(app, ev); }))); break;
	case ::llc::SYSTEM_EVENT_Text		: if_true_e(failed(::llc::eventExtractAndHandle<::llc::EVENT_TEXT		>(sysEvent, [&app](auto ev) { return processTextEvent		(app, ev); }))); break;
	case ::llc::SYSTEM_EVENT_Keyboard	: if_true_e(failed(::llc::eventExtractAndHandle<::llc::EVENT_KEYBOARD	>(sysEvent, [&app](auto ev) { return processKeyboardEvent	(app, ev); }))); break;
	}
	return 0;
}

::llc::error_t			ghg::galaxyHellUpdate	(::ghg::SGalaxyHellApp & app, double lastTimeSeconds, const ::llc::pobj<::llc::SInput> & inputState, ::llc::vpobj<::llc::SEventSystem> systemEventsNew) {
	if_true_vi(1, app.ActiveState == ::ghg::APP_STATE_Quit);

	llc_necs(systemEventsNew.for_each([&app](const ::llc::pobj<::llc::SEventSystem> & sysEvent) { return ::processSystemEvent(app, *sysEvent); }));

	switch(app.ActiveState) {
	case APP_STATE_Init		: {
		::ghg::solarSystemSetup(app.Game, app.Game.DrawCache.RenderTargetMetrics);
		::ghg::guiSetup(app, inputState);
		if(app.RenderTargetPool.size())
			llc_necs(::updateAllGuiMetrics(app, app.RenderTargetPool[0]->Color.View.metrics().u1()));

		::llc::aobj<::llc::string>					fileNames					= {};
		::llc::pathList(app.SavegameFolder, fileNames, app.ExtensionSaveAuto);
		if(fileNames.size()) {
			if(failed(::ghg::solarSystemLoad(app.Game, fileNames[0]))) {
				app.Game.PilotsReset();
			}
		}
		fileNames					= {};
		::llc::pathList(app.SavegameFolder, fileNames, app.ExtensionProfile);
		app.Players.resize(fileNames.size());

		for(uint32_t iPlayer = 0; iPlayer < fileNames.size(); ++iPlayer) {
			try {
				llc_necall(app.Players[iPlayer].Load(fileNames[iPlayer]), "fileNames[iPlayer]: %s", fileNames[iPlayer].begin());
			}
			catch (const sc_t * ) {
				app.Players[iPlayer] = {};
			}

		}
		app.TunerPlayerCount->SetValue((uint8_t)app.Game.PlayState.Constants.Players);
		for(uint32_t iPilot = 0; iPilot < app.Game.PlayState.Constants.Players; ++iPilot) {
			const ::llc::vcst_t namePilot	= app.Game.Pilots[iPilot].Name;
			for(uint32_t iPlayer = 0; iPlayer < app.Game.PlayState.Constants.Players; ++iPlayer) {
				if(iPlayer >= app.Players.size())
					app.AddNewPlayer(namePilot);

				const ::llc::vcst_t namePlayer	= app.Players[iPlayer].Name;
				if(namePilot == namePlayer) {
					::std::swap(app.Players[iPlayer], app.Players[iPilot]);
					break;
				}
			}
			for(uint32_t iPlayer = app.Game.PlayState.Constants.Players; iPlayer < app.Players.size(); ++iPlayer) {
				const ::llc::vcst_t namePlayer	= app.Players[iPlayer].Name;
				if(namePilot == namePlayer) {
					::std::swap(app.Players[iPlayer], app.Players[iPilot]);
					break;
				}
			}
		}
		app.ActiveState					= APP_STATE_Welcome;
		break;
	}
	case  APP_STATE_Welcome:
		if(app.Players.size())
			app.ActiveState					= APP_STATE_Home;
		else {
			//app.DialogDesktop.GUI->Controls.States[app.VirtualKeyboard.IdRoot].Hidden = false;
		}
		break;
	}	 

	const bool inGame = app.ActiveState == ::ghg::APP_STATE_Play;

	::llc::view<::ghg::SShipController>		controllerPlayer	= app.Game.ShipControllers;


	const ::llc::SInput								& input				= *app.DialogDesktop.Input;
	if(controllerPlayer.size() && app.Game.PlayState.Constants.Players == 1) {
		controllerPlayer[0].Forward					= input.KeyboardCurrent.KeyState[VK_UP		] || input.KeyboardCurrent.KeyState['W'];
		controllerPlayer[0].Back					= input.KeyboardCurrent.KeyState[VK_DOWN	] || input.KeyboardCurrent.KeyState['S'];
		controllerPlayer[0].Left					= input.KeyboardCurrent.KeyState[VK_LEFT	] || input.KeyboardCurrent.KeyState['A'];
		controllerPlayer[0].Right					= input.KeyboardCurrent.KeyState[VK_RIGHT	] || input.KeyboardCurrent.KeyState['D'];
		controllerPlayer[0].Turbo					= input.KeyboardCurrent.KeyState[VK_RCONTROL] || input.KeyboardCurrent.KeyState[VK_CONTROL] || input.KeyboardCurrent.KeyState[VK_LSHIFT] || input.KeyboardCurrent.KeyState[VK_SHIFT];
	}
	else if(controllerPlayer.size() > 1) {
		controllerPlayer[0].Forward					= input.KeyboardCurrent.KeyState['W'];
		controllerPlayer[0].Back					= input.KeyboardCurrent.KeyState['S'];
		controllerPlayer[0].Left					= input.KeyboardCurrent.KeyState['A'];
		controllerPlayer[0].Right					= input.KeyboardCurrent.KeyState['D'];
		controllerPlayer[0].Turbo					= input.KeyboardCurrent.KeyState[VK_LSHIFT] || input.KeyboardCurrent.KeyState[VK_SHIFT];

		controllerPlayer[1].Forward					= input.KeyboardCurrent.KeyState[VK_UP		];
		controllerPlayer[1].Back					= input.KeyboardCurrent.KeyState[VK_DOWN	];
		controllerPlayer[1].Left					= input.KeyboardCurrent.KeyState[VK_LEFT	];
		controllerPlayer[1].Right					= input.KeyboardCurrent.KeyState[VK_RIGHT	];
		controllerPlayer[1].Turbo					= input.KeyboardCurrent.KeyState[VK_RCONTROL] || input.KeyboardCurrent.KeyState[VK_CONTROL];
	}

	::ghg::solarSystemUpdate(app.Game, (false == inGame) ? 0 : lastTimeSeconds, *inputState, systemEventsNew);
	for(uint32_t iShip = 0; iShip < app.Game.ShipState.SpaceshipManager.ShipOrbiterActionQueue.size(); ++iShip)
		if(iShip < app.Game.PlayState.Constants.Players) {
			if(app.Game.ShipState.SpaceshipManager.ShipOrbiterActionQueue[iShip]) {
				::llc::apod<::llc::SHIP_ACTION> & actionQueue = *app.Game.ShipState.SpaceshipManager.ShipOrbiterActionQueue[iShip];
				for(uint32_t iEvent = 0; iEvent < actionQueue.size(); ++iEvent)
					if(actionQueue[iEvent] == ::llc::SHIP_ACTION_Spawn) {
						app.Save(::ghg::SAVE_MODE_STAGE);
						actionQueue[iEvent] = (::llc::SHIP_ACTION)-1;
					}
			}
		}
	//::ghg::overlayUpdate(app.Overlay, app.World.PlayState.Stage, app.World.ShipState.ShipCores.size() ? app.World.ShipState.ShipCores[0].Score : 0, app.World.PlayState.TimeWorld);
	// Galaxy3l
	app.ActiveState					= (::ghg::APP_STATE)::ghg::guiUpdate(app, systemEventsNew);
	return 0;
}

::llc::error_t					ghg::galaxyHellDraw				(::ghg::SGalaxyHellApp & app, ::llc::n2u1_t renderTargetSize) {
	if(app.ActiveState < 2)
		return 0;

	::llc::pobj<::ghg::TRenderTarget>		target	= {};
	{
		::std::lock_guard			rtLock(app.RenderTargetLockPool);
		if(app.RenderTargetPool.size()) {
			target					= app.RenderTargetPool[0];
			app.RenderTargetPool.remove_unordered(0);
		}
	}
	if(!target) {
		::std::lock_guard			rtLock(app.RenderTargetLockQueue);
		if(app.RenderTargetQueue.size() > 1) {
			target					= app.RenderTargetQueue[0];
			app.RenderTargetQueue.remove_unordered(0);
		}
	}

	target->resize(renderTargetSize.u2(), {0, 0, 0, 1}, 0xFFFFFFFFU);

	::llc::g8bgra							targetPixels			= target->Color.View;
	::llc::grid<uint32_t>									depthBuffer				= target->DepthStencil.View;
	switch(app.ActiveState) {
	default					: 
	case APP_STATE_Play		: 
		::ghg::solarSystemDraw(app.Game, app.Game.DrawCache, app.Game.LockUpdate);
		::llc::pobj<::ghg::TRenderTarget>		sourceRT	= {};
		sourceRT								= app.Game.DrawCache.RenderTarget;
		if(!sourceRT)
			break;

		::llc::g8bgra			cameraView			= sourceRT->Color.View;
		::llc::n2s1_t						cameraViewMetrics	= cameraView.metrics().s1();
		::llc::grid_copy(targetPixels, cameraView, ::llc::n2<uint32_t>
				{ (targetPixels.metrics().x >> 1) - (cameraView.metrics().x >> 1)
				, (targetPixels.metrics().y >> 1) - (cameraView.metrics().y >> 1)
				}
			);

		const ::llc::n2s1_t		targetCenter		= targetPixels.metrics().s1() / 2;
		const ::llc::n2s1_t		cameraCenter		= cameraView.metrics().s1() / 2;
		::llc::n2s1_t				cornerTopLeft		= targetCenter + ::llc::n2s1_t{int16_t(-cameraCenter.x), int16_t(-cameraCenter.y)};
		::llc::n2s1_t				cornerTopRight		= targetCenter + ::llc::n2s1_t{int16_t( cameraCenter.x), int16_t(-cameraCenter.y)};
		::llc::n2s1_t				cornerBottomLeft	= targetCenter + ::llc::n2s1_t{int16_t(-cameraCenter.x), int16_t( cameraCenter.y)};
		::llc::n2s1_t				cornerBottomRight	= targetCenter + ::llc::n2s1_t{int16_t( cameraCenter.x), int16_t( cameraCenter.y)};

		cornerTopLeft		.InPlaceClamp({}, {(int16_t)(targetPixels.metrics().x - 1), (int16_t)(targetPixels.metrics().y - 1)});
		cornerTopRight		.InPlaceClamp({}, {(int16_t)(targetPixels.metrics().x - 1), (int16_t)(targetPixels.metrics().y - 1)});
		cornerBottomLeft	.InPlaceClamp({}, {(int16_t)(targetPixels.metrics().x - 1), (int16_t)(targetPixels.metrics().y - 1)});
		cornerBottomRight	.InPlaceClamp({}, {(int16_t)(targetPixels.metrics().x - 1), (int16_t)(targetPixels.metrics().y - 1)});

		::llc::drawLine(targetPixels, ::llc::line2s1_t{cornerTopLeft		, cornerTopRight	}, ::llc::DARKGRAY);
		::llc::drawLine(targetPixels, ::llc::line2s1_t{cornerTopLeft		, cornerBottomLeft	}, ::llc::DARKGRAY);
		::llc::drawLine(targetPixels, ::llc::line2s1_t{cornerTopRight	, cornerBottomRight	}, ::llc::GRAY * 1.1);
		::llc::drawLine(targetPixels, ::llc::line2s1_t{cornerBottomLeft	, cornerBottomRight	}, ::llc::GRAY * 1.1);

		//::ghg::overlayDraw(app.Overlay, app.World.DrawCache, app.World.PlayState.TimeWorld, depthBuffer, targetPixels);
		break;
	}

	{
		::std::lock_guard							lockUpdate			(app.Game.LockUpdate);
		::llc::guiDraw(*app.DialogPerState[app.ActiveState].GUI, targetPixels);
	}
	if(app.ActiveState != ::ghg::APP_STATE_Play) {
		::std::lock_guard							lockUpdate			(app.Game.LockUpdate);
		::llc::guiDraw(*app.DialogDesktop.GUI, targetPixels);
	}
	for(uint32_t iPlayer = 0; iPlayer < app.UIPlay.PlayerUI.size(); ++iPlayer) {
		::std::lock_guard							lockUpdate			(app.Game.LockUpdate);
		::llc::guiDraw((app.ActiveState == ::ghg::APP_STATE_Play)
			? *app.UIPlay.PlayerUI[iPlayer].DialogPlay.GUI
			: *app.UIPlay.PlayerUI[iPlayer].DialogHome.GUI
			, targetPixels
		);
	}
	{
		::std::lock_guard			rtLock(app.RenderTargetLockQueue);
		app.RenderTargetQueue.push_back(target);
	}
	return 0;
}

::llc::error_t					ghg::listFilesSavegame		(::ghg::SGalaxyHellApp & app, const ::llc::vcst_t & saveGameFolder, ::llc::aobj<::llc::vcst_t> & savegameFilenames) {
	::llc::aobj<::llc::string>	fileNames;
	if_true_ef(failed(::llc::pathList(saveGameFolder, fileNames, app.ExtensionSave)), "%s", saveGameFolder.begin());

	::llc::aobj<::llc::vcst_t>				pathFileNames;
	for(uint32_t iFile = 0; iFile < fileNames.size(); ++iFile)
		pathFileNames.push_back(fileNames[iFile]);

	::llc::aobj<::llc::vcst_t>				pathFileNamesSaveAuto;
	::llc::aobj<::llc::vcst_t>				pathFileNamesSaveUser;
	::llc::aobj<::llc::vcst_t>				pathFileNamesSaveStage;
	::llc::filterPostfix(pathFileNames, app.ExtensionSaveAuto	, pathFileNamesSaveAuto);
	::llc::filterPostfix(pathFileNames, app.ExtensionSaveUser	, pathFileNamesSaveUser);
	::llc::filterPostfix(pathFileNames, app.ExtensionSaveStage	, pathFileNamesSaveStage);
		
	for(uint32_t iPath = 0; iPath < pathFileNamesSaveAuto .size(); ++iPath) savegameFilenames.push_back(::llc::label(pathFileNamesSaveAuto [iPath]));
	for(uint32_t iPath = 0; iPath < pathFileNamesSaveUser .size(); ++iPath) savegameFilenames.push_back(::llc::label(pathFileNamesSaveUser [iPath]));
	for(uint32_t iPath = 0; iPath < pathFileNamesSaveStage.size(); ++iPath) savegameFilenames.push_back(::llc::label(pathFileNamesSaveStage[iPath]));
	return 0;
}

::llc::error_t					ghg::listFilesProfile		(::ghg::SGalaxyHellApp & app, const ::llc::vcst_t & saveGameFolder, ::llc::aobj<::llc::vcst_t> & savegameFilenames) {
	::llc::aobj<::llc::string>	fileNames;
	if_true_ef(failed(::llc::pathList(saveGameFolder, fileNames, app.ExtensionProfile)), "%s", saveGameFolder.begin());

	::llc::aobj<::llc::vcst_t>				pathFileNames;
	for(uint32_t iFile = 0; iFile < fileNames.size(); ++iFile)
		savegameFilenames.push_back(::llc::label(::llc::vcst_t{fileNames[iFile].begin(), fileNames[iFile].size()}));
	return 0;
}
