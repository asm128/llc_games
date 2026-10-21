#include "llc_galaxy_hell_app.h"
#include "llc_file.h"
#include "llc_raster_lh.h"
#include "llc_grid_copy.h"
#include "llc_sphere.h"

using ::llc::get_value_namep, ::llc::get_enum_namep, ::llc::failed;
LLC_USING_TYPEINT();

stxp	::llc::GUI_COLOR_MODE	GHG_MENU_COLOR_MODE		= ::llc::GUI_COLOR_MODE_3D;
stxp	bool					BACKGROUND_3D_ONLY		= true;
stxp	::llc::n2u1_t			MODULE_CAMERA_SIZE		= {48, 48};
stxp	::llc::n2u1_t			MODULE_VIEWPORT_SIZE	= {128, 64};
stxp	::llc::n2u1_t			WEAPON_BAR_SIZE			= {96, 16};
stxp	::llc::n2u1_t			PLAYER_UI_SIZE			= {320, 40};
stxp	::llc::n2u1_t			HOME_BUTTON_SIZE		= {160, 24};

static	::llc::error_t			guiSetupCommon			(::llc::SGUI & gui) {
	gui.ColorModeDefault	= ::llc::GUI_COLOR_MODE_3D;
	gui.ThemeDefault		= ::llc::ASCII_COLOR_DARKRED * 16 + 10;
	gui.SelectedFont		= 7;

	::llc::pobj<::llc::SGUIColors>	colors;
	colors.create(*gui.Colors);
	gui.Colors				= colors;
	return 0;
}

static	::llc::error_t			dialogCreateCommon		(::llc::SDialog & dialogLoad, const ::llc::pobj<::llc::SInput> & inputState, const ::llc::n2f2_t & cursorPos) { 
	dialogLoad						= {};
	dialogLoad.Input				= inputState;
	dialogLoad.GUI->CursorPos		= cursorPos;
	dialogLoad.GUI->Controls.Draw[0].NoClient	= true;
	dialogLoad.GUI->Controls.Draw[0].NoBorder	= true;
	::guiSetupCommon(*dialogLoad.GUI);
	dialogLoad.Update();
	return 0;
}

// -------------------------------- Set up player GUI for home screen
static	::llc::error_t			uiPlayerSetupHome		(::ghg::SUIPlayer & uiPlayer, ::llc::pobj<::llc::SInput> input, ::llc::n2f2_t cursorPos, uint32_t iPlayer, const ::llc::vcst_t playerName) {
	::llc::ALIGN						playerAlign			= {};
	::llc::n2s1_t						playerPosition		= {};
	switch(iPlayer) {
	default:
	case 0: playerAlign = ::llc::ALIGN_BOTTOM_LEFT;		break;
	case 1: playerAlign = ::llc::ALIGN_TOP_RIGHT;		break;
	case 2: playerAlign = ::llc::ALIGN_BOTTOM_RIGHT;	break;
	case 3: playerAlign = ::llc::ALIGN_TOP_LEFT;		break;
	}
	switch(iPlayer) {
	default:
	case 0: playerPosition = {}; break; //{0, -10/2}; break;	// I don't know where this measure comes from
	case 1: playerPosition = {}; break; //{0, int16_t(-20 * (int16_t)::llc::get_value_count<::ghg::UI_PLAYER_SCORE>())}; break;
	case 2: playerPosition = {}; break; //{120}; break;
	case 3: playerPosition = {}; break; //{-120}; break;
	}
	::llc::SDialog				& playerDialog		= uiPlayer.DialogHome;
	llc_necs(::dialogCreateCommon(playerDialog, input, cursorPos));
	::llc::SGUI					& playerGUI			= *playerDialog.GUI;
	::llc::pobj<::llc::a8bgra>	palette;
	palette.create(*playerGUI.Colors->Palette);
	playerGUI.Colors->Palette = palette;
	playerGUI.ThemeDefault	= ::llc::ASCII_COLOR_DARKGREY * 16 + 12;
	::llc::guiCreateControlList<::ghg::UI_PROFILE>(playerGUI, playerDialog.Root, PLAYER_UI_SIZE, playerPosition, playerAlign, ::llc::ALIGN_CENTER, uiPlayer.ControlsDialogHome);
	for(uint32_t iButton = 0, iStop = ::llc::get_value_count<::ghg::UI_PROFILE>() - 1, iOffset = playerDialog.Root + 1 + ghg::UI_PROFILE_Name; iButton < iStop; ++iButton) {
		playerGUI.Controls.Placement	[iOffset + iButton].Margin				= {}; 
		//playerGUI.Controls.Controls		[iButton].Border				= {2, 2, 2, 2};
		//playerGUI.Controls.Draw			[iOffset + iButton].NoBorder		= true;
		playerGUI.Controls.Draw			[iOffset + iButton].NoClient		= true;
	}
	for(uint32_t iButton = 0, iStop = ::llc::get_value_count<::ghg::UI_PROFILE>() - 1, iOffset = playerDialog.Root + 1 + ghg::UI_PROFILE_Score; iButton < iStop; ++iButton) {
		playerGUI.Controls.Placement[iOffset + iButton].Area.Offset.y		-= int16_t(16 * iButton);
		playerGUI.Controls.Placement[iOffset + iButton].Area.Size.y			= 24; 
	}

	switch(iPlayer) {
	default:
	case 0: playerAlign = ::llc::ALIGN_BOTTOM_LEFT	; break;
	case 1: playerAlign = ::llc::ALIGN_TOP_RIGHT	; break;
	case 2: playerAlign = ::llc::ALIGN_BOTTOM_RIGHT	; break;
	case 3: playerAlign = ::llc::ALIGN_TOP_LEFT		; break;
	}
	llc_necs(::llc::inputBoxCreate(uiPlayer.InputBox, *playerDialog.GUI, playerDialog.Root));
	playerGUI.Controls.SetHidden(uiPlayer.InputBox.IdRoot, true);
	playerGUI.Controls.Placement[uiPlayer.InputBox.IdRoot].Align			= playerAlign;
	playerGUI.Controls.Placement[uiPlayer.InputBox.IdRoot].Area.Offset.x	= 240;

	llc_necs(::llc::controlFontSet(playerGUI, playerDialog.Root + 1 + ::ghg::UI_PROFILE_Name, 10));
	llc_necs(::llc::controlTextSet(playerGUI, playerDialog.Root + 1 + ::ghg::UI_PROFILE_Name, playerName));
	return 0;
}

// -------------------------------- Set up player GUI for in-stage display
static	::llc::error_t	uiPlayerSetupPlay	(::ghg::SUIPlayer & uiPlayer, ::llc::pobj<::llc::SInput> input, ::llc::n2f2_t cursorPos, uint32_t iPlayer, const ::llc::vcst_t pilotName, uint32_t nShipParts) {
	::llc::n2s1_t				playerPosition;
	::llc::ALIGN				playerAlign		;
	switch(iPlayer) {
	default:
	case 0: playerAlign = ::llc::ALIGN_BOTTOM_LEFT;		break;
	case 1: playerAlign = ::llc::ALIGN_TOP_RIGHT;		break;
	case 2: playerAlign = ::llc::ALIGN_BOTTOM_RIGHT;	break;
	case 3: playerAlign = ::llc::ALIGN_TOP_LEFT;		break;
	}
	switch(iPlayer) {
	default:
	case 0: playerPosition = {}; break;
	case 1: playerPosition = {}; break;
	case 2: playerPosition = {}; break;
	case 3: playerPosition = {}; break;
	}
	::llc::SDialog				& playerDialog		= uiPlayer.DialogPlay;
	llc_necs(::dialogCreateCommon(playerDialog, input, cursorPos));
	::llc::SGUI					& playerGUI			= *playerDialog.GUI;
	::llc::pobj<::llc::a8bgra>	palette;
	palette.create(*playerGUI.Colors->Palette);
	playerGUI.Colors->Palette = palette;
	playerGUI.ThemeDefault	= ::llc::ASCII_COLOR_DARKGREY * 16 + 12;
	::llc::guiCreateControlList<::ghg::UI_PROFILE>(playerGUI, playerDialog.Root, PLAYER_UI_SIZE, playerPosition, playerAlign, ::llc::ALIGN_CENTER, uiPlayer.ControlsDialogPlay);
	for(uint32_t iButton = 0, iStop = ::llc::get_value_count<::ghg::UI_PROFILE>() - 1, iOffset = playerDialog.Root + 1 + ghg::UI_PROFILE_Name; iButton < iStop; ++iButton) {
		playerGUI.Controls.Placement[iOffset + iButton].Margin		= {}; 
		//playerGUI.Controls.Controls	[iOffset + iButton].Border	= {};
		//playerGUI.Controls.Draw		[iOffset + iButton].NoBorder	= true;
		playerGUI.Controls.Draw		[iOffset + iButton].NoClient	= true;
		//playerGUI.Controls.Constraints[iOffset + iButton].AttachSizeToControl.x	= ::llc::cid_t(iOffset + iButton + 1);
	}
	for(uint32_t iButton = 0, iStop = ::llc::get_value_count<::ghg::UI_PROFILE>() - 1, iOffset = playerDialog.Root + 1 + ghg::UI_PROFILE_Score; iButton < iStop; ++iButton) {
		playerGUI.Controls.Placement[iOffset + iButton].Area.Offset.y	-= int16_t(16 * iButton);
		playerGUI.Controls.Placement[iOffset + iButton].Area.Size.y		= 24; 
		//playerGUI.Controls.Placement[iOffset + iButton].Area.Size.x		= 24; 
		//playerGUI.Controls.Constraints[iOffset + iButton].AttachSizeToText.x	= true;
	}

	llc_necs(::llc::controlFontSet(playerGUI, playerDialog.Root + 1 + ::ghg::UI_PROFILE_Name, 10));
	llc_necs(::llc::controlTextSet(playerGUI, playerDialog.Root + 1 + ::ghg::UI_PROFILE_Name, pilotName));
	llc_necs(::llc::controlTextSet(playerGUI, playerDialog.Root + 1 + ::ghg::UI_PROFILE_Score, ""));

	for(uint32_t iOrbiter = 0, countViewports = nShipParts; iOrbiter < countViewports; ++iOrbiter) {
		int32_t						indexVP				= uiPlayer.ModuleViewports.push_back({});
		uiPlayer.ModuleViewports[indexVP]->Viewport = ::llc::controlCreate(playerGUI); // Orbiter
		::llc::controlCreate(playerGUI); // Weapon Load
		::llc::controlCreate(playerGUI); // Weapon Type
		for(uint32_t iControl = 0; iControl < 3; ++iControl)
			::llc::controlSetParent(playerGUI, ::llc::cid_t(uiPlayer.ModuleViewports[indexVP]->Viewport + iControl), playerDialog.Root); // Weapon Load

		::ghg::SUIPlayShipPartViewport	& viewport		= *uiPlayer.ModuleViewports[indexVP];
		viewport.RenderTargetOrbiter.resize(::MODULE_CAMERA_SIZE);
		viewport.RenderTargetWeaponLoad.resize(::WEAPON_BAR_SIZE);
		viewport.RenderTargetWeaponType.resize(::WEAPON_BAR_SIZE);
		::llc::SControlPlacement	& controlOrbiter	= playerGUI.Controls.Placement[viewport.Viewport + 0];
		::llc::SControlPlacement	& controlWeaponType	= playerGUI.Controls.Placement[viewport.Viewport + 2];
		::llc::SControlPlacement	& controlWeaponLoad	= playerGUI.Controls.Placement[viewport.Viewport + 1];
		{
			::llc::SControlImage	& imageOrbiter		= playerGUI.Controls.Images[viewport.Viewport + 0];
			::llc::SControlImage	& imageWeaponType	= playerGUI.Controls.Images[viewport.Viewport + 2];
			::llc::SControlImage	& imageWeaponLoad	= playerGUI.Controls.Images[viewport.Viewport + 1];
			imageOrbiter	.Image	= viewport.RenderTargetOrbiter.Color.View;
			imageWeaponType	.Image	= viewport.RenderTargetWeaponLoad.Color.View;
			imageWeaponLoad	.Image	= viewport.RenderTargetWeaponType.Color.View;
			imageOrbiter.ImageAlign	= imageWeaponType.ImageAlign = imageWeaponLoad.ImageAlign	= ::llc::ALIGN_CENTER;
		}

			 if(0 == iPlayer) controlOrbiter.Align = controlWeaponLoad.Align = controlWeaponType.Align = ::llc::ALIGN_CENTER_LEFT	;
		else if(1 == iPlayer) controlOrbiter.Align = controlWeaponLoad.Align = controlWeaponType.Align = ::llc::ALIGN_CENTER_RIGHT	;
		else if(2 == iPlayer) controlOrbiter.Align = controlWeaponLoad.Align = controlWeaponType.Align = ::llc::ALIGN_CENTER_BOTTOM	;
		else if(3 == iPlayer) controlOrbiter.Align = controlWeaponLoad.Align = controlWeaponType.Align = ::llc::ALIGN_CENTER_TOP	;
		if(1 >= iPlayer)	controlOrbiter.Area.Offset	= controlWeaponLoad.Area.Offset	= controlWeaponType.Area.Offset	= {0, int16_t(-(int16_t(countViewports * (MODULE_CAMERA_SIZE.y + 4)) >> 1) + (MODULE_CAMERA_SIZE.y + 4) * iOrbiter)};
		else	 			controlOrbiter.Area.Offset	= controlWeaponLoad.Area.Offset	= controlWeaponType.Area.Offset	= {int16_t(-(int16_t(countViewports * (WEAPON_BAR_SIZE.x + 4)) >> 1) + (WEAPON_BAR_SIZE.x + 4) * iOrbiter)};
		controlOrbiter.Area.Size	= MODULE_CAMERA_SIZE.s1();
		controlOrbiter.Border		= {};
		controlOrbiter.Margin		= {};

		controlWeaponLoad.Area.Size	= controlWeaponType.Area.Size	= WEAPON_BAR_SIZE.s1();	//MODULE_VIEWPORT_SIZE.s1();
		controlWeaponLoad.Border	= controlWeaponType.Border		= {1, 1, 1, 1};
		controlWeaponLoad.Margin	= controlWeaponType.Margin		= {};

		if(1 >= iPlayer) {
			controlWeaponLoad.Area.Offset.y	-= WEAPON_BAR_SIZE.y / 2 + 1;
			controlWeaponType.Area.Offset.y	+= WEAPON_BAR_SIZE.y / 2 + 1;
			controlWeaponLoad.Area.Offset.x	+= 
			controlWeaponType.Area.Offset.x	+= MODULE_CAMERA_SIZE.x + 1;
		}
		else {
			controlWeaponLoad.Area.Offset.y	+= 1 * MODULE_CAMERA_SIZE.y + 2;
			controlWeaponType.Area.Offset.y	+= 1 * MODULE_CAMERA_SIZE.y + WEAPON_BAR_SIZE.y + 2;
		}

		playerGUI.Controls.Text		[viewport.Viewport + 1].Align	= ::llc::ALIGN_CENTER; //iPlayer ? ::llc::ALIGN_CENTER_RIGHT : ::llc::ALIGN_CENTER_LEFT;
		playerGUI.Controls.Text		[viewport.Viewport + 2].Align	= ::llc::ALIGN_CENTER; //iPlayer ? ::llc::ALIGN_CENTER_RIGHT : ::llc::ALIGN_CENTER_LEFT;

		playerGUI.Controls.Draw	[viewport.Viewport + 0].NoClient	= 
		playerGUI.Controls.Draw	[viewport.Viewport + 1].NoClient	= 
		playerGUI.Controls.Draw	[viewport.Viewport + 2].NoClient	= true;
		playerGUI.Controls.Draw	[viewport.Viewport + 0].NoBorder	= true;
		playerGUI.Controls.Draw	[viewport.Viewport + 1].NoBorder	= true;
		//playerGUI.Controls.Draw	[viewport.Viewport + 2].NoBorder	= true;

		::llc::m4f2_t				& matrixProjection	= viewport.MatrixProjection;
		matrixProjection.FieldOfView(::llc::math_pi * .25, MODULE_CAMERA_SIZE.x / (double)MODULE_CAMERA_SIZE.y, 0.01, 500.0);
		::llc::m4f2_t				matrixViewport		= {};
		matrixViewport.ViewportLH(MODULE_CAMERA_SIZE.u1());
		matrixProjection		*= matrixViewport;


		const ::llc::circlef2_t		circleLife			= {::llc::min(MODULE_CAMERA_SIZE.x, MODULE_CAMERA_SIZE.y) * .5f - 6 * 0, viewport.RenderTargetOrbiter.Color.View.metrics().f2() * .5};
		const ::llc::circlef2_t		circleDelay			= {::llc::min(MODULE_CAMERA_SIZE.x, MODULE_CAMERA_SIZE.y) * .5f - 6 * 1, viewport.RenderTargetOrbiter.Color.View.metrics().f2() * .5};
		const ::llc::circlef2_t		circleCooldown		= {::llc::min(MODULE_CAMERA_SIZE.x, MODULE_CAMERA_SIZE.y) * .5f - 6 * 2, viewport.RenderTargetOrbiter.Color.View.metrics().f2() * .5};
		
		::ghg::gaugeBuildRadial(viewport.GaugeLife, circleLife, 32, 12);
	}
	playerDialog.Update();
	return 0;
}

//static ::llc::error_t	guiSetupInit		(::llc::SDialog & dialog) { return guiSetupButtonList<::ghg::UI_LOAD>(*dialog.GUI,  60, 0, ::llc::ALIGN_BOTTOM_RIGHT); }
static	::llc::error_t	guiSetupLoad		(::llc::SDialog & /*dialog*/) { 
	//::llc::ai32					temp;
	//temp.resize();
	//llc_necs(::llc::guiSetupButtonList<::ghg::UI_LOAD>(*dialog.GUI, dialog.Root, 60, 0, ::llc::ALIGN_CENTER, ::llc::ALIGN_CENTER, {})); 
	return 0;
}

static	::llc::error_t	guiSetupHome		(::ghg::SGalaxyHellApp & app, ::llc::SDialog & dialog) { 
	llc_necs(::llc::guiCreateControlList<::ghg::UI_HOME>(*dialog.GUI, dialog.Root, HOME_BUTTON_SIZE, {0, int16_t(-HOME_BUTTON_SIZE.y * ::llc::get_value_count<::ghg::UI_HOME>() / 2)}, ::llc::ALIGN_CENTER, ::llc::ALIGN_CENTER, app.DialogControls[::ghg::APP_STATE_Home])); 
	for(uint32_t iButton = 0; iButton < ::llc::get_value_count<::ghg::UI_HOME>(); ++iButton)
		dialog.GUI->Controls.Placement[iButton + 1].Area.Offset.y += 0;

	for(uint32_t iButton = ::ghg::UI_HOME_Start; iButton < ::llc::get_value_count<::ghg::UI_HOME>(); ++iButton)
		dialog.GUI->Controls.Placement[iButton + 1].Area.Offset.y += 36;

	dialog.GUI->Controls.Text[::ghg::UI_HOME_Start + 1].Text = "Start new game";

	llc_necs(::llc::tunerCreate(dialog, app.TunerPlayerCount));
	app.TunerPlayerCount->ValueLimits.Min	= 1;
	app.TunerPlayerCount->ValueLimits.Max	= ::ghg::MAX_PLAYERS;
	app.TunerPlayerCount->FuncValueFormat	= [](::llc::vcst_t & string, uint8_t value, const ::llc::minmax<uint8_t> & /*limits*/) { 
		string = (value == 1) ? ::llc::vcs("%lli Player") : ::llc::vcs("%lli Players"); 
		return 0; 
	};
	app.TunerPlayerCount->SetValue(1);
	//tuner->IdGUIControl
	::llc::SControlPlacement	& control					= dialog.GUI->Controls.Placement[app.TunerPlayerCount->IdGUIControl];
	control.Area.Size.x		= 160;
	control.Area.Size.y		= 24;
	control.Area.Offset.y	= int16_t(dialog.GUI->Controls.Placement[::ghg::UI_HOME_Start + 1].Area.Offset.y - (dialog.GUI->Controls.Placement[::ghg::UI_HOME_Start + 1].Area.Size.y >> 1) - (control.Area.Size.y >> 1));
	control.Align			= ::llc::ALIGN_CENTER;

	app.UIPlay.PlayerUI.resize(::ghg::MAX_PLAYERS);	
	for(uint32_t iPlayer = 0; iPlayer < ::ghg::MAX_PLAYERS; ++iPlayer) {
		::ghg::SUIPlayer			& uiPlayer					= app.UIPlay.PlayerUI[iPlayer];
		llc_necall(::uiPlayerSetupHome(uiPlayer, dialog.Input, dialog.GUI->CursorPos, iPlayer, ""/*app.Players[iPlayer].Name*/), "iPlayer: %i", iPlayer);
	}
	return 0;
}
static	::llc::error_t	guiSetupProfile		(::ghg::SGalaxyHellApp & app, ::llc::SDialog & dialog) { return ::llc::guiCreateControlList<::ghg::UI_PLAY>(*dialog.GUI, dialog.Root,  60, 0, ::llc::ALIGN_BOTTOM_RIGHT, ::llc::ALIGN_CENTER, app.DialogControls[::ghg::APP_STATE_Profile]); }
static	::llc::error_t	guiSetupShop		(::ghg::SGalaxyHellApp & app, ::llc::SDialog & dialog) { 
	::llc::SGUI					& gui				= *dialog.GUI;
	llc_necs(::llc::guiCreateControlList<::ghg::UI_SHOP>(gui, dialog.Root, 60, 0, ::llc::ALIGN_BOTTOM_RIGHT, ::llc::ALIGN_CENTER, app.DialogControls[::ghg::APP_STATE_Shop]));

	::llc::pobj<::llc::SDialogViewport>	viewport	= {};
	llc_necs(::llc::viewportCreate(dialog, viewport));
	//viewport->Settings.DisplacementLockX	= false;;
	//viewport->Settings.DisplacementLockY	= false;;
	gui.Controls.Placement[viewport->IdGUIControl	].Align			= ::llc::ALIGN_CENTER;
	gui.Controls.Placement[viewport->IdGUIControl	].Area.Offset	= {};
	gui.Controls.Placement[viewport->IdGUIControl	].Area.Size		= {640, 480};
	gui.Controls.Images		[viewport->IdClient		].ImageInvertY	= true;
	return 0;
}

static	::llc::error_t	guiSetupWelcome		(::ghg::SGalaxyHellApp & app, ::llc::SDialog & dialog) { 
	::llc::SGUI					& gui				= *dialog.GUI;
	llc_necs(::llc::guiCreateControlList<::ghg::UI_WELCOME>(gui, dialog.Root, ::llc::n2u1_t{128, 32}, ::llc::n2s1_t{0, 64}, ::llc::ALIGN_CENTER, ::llc::ALIGN_CENTER, app.DialogControls[::ghg::APP_STATE_Welcome]));
	::llc::viewportCreate(dialog, app.Inputbox);
	::llc::pobj<::llc::SDialogViewport>	viewport	= app.Inputbox;
	gui.Controls.Placement[viewport->IdGUIControl	].Align			= ::llc::ALIGN_CENTER;
	gui.Controls.Placement[viewport->IdGUIControl	].Area.Offset	= {};
	gui.Controls.Placement[viewport->IdClient		].Area.Size		= {256, 20};
	gui.Controls.Text		[viewport->IdTitle		].Text			= "Enter your name:";
	::llc::viewportAdjustSize(gui.Controls.Placement[viewport->IdGUIControl].Area.Size, gui.Controls.Placement[viewport->IdClient].Area.Size);
	gui.Controls.Images		[viewport->IdClient		].ImageInvertY	= true;
	return 0;
}

static	::llc::error_t	guiSetupPlay		(::ghg::SGalaxyHellApp & app, ::llc::SDialog & dialog) { 
	dialog.GUI->SelectedFont	= 6;

	llc_necs(::llc::guiCreateControlList<::ghg::UI_PLAY>(*dialog.GUI, dialog.Root, {220, 18}, {}, ::llc::ALIGN_TOP_RIGHT, ::llc::ALIGN_CENTER, app.DialogControls[::ghg::APP_STATE_Play]));
	for(uint32_t iButton = 0; iButton < ::ghg::UI_PLAY_Level; ++iButton) {
		//dialog.GUI->Controls.Text	[iButton + 1].FontSelected	= ::llc::ALIGN_TOP_LEFT;
	}
	for(uint32_t iButton = ::ghg::UI_PLAY_Level; iButton < ::llc::get_value_count<::ghg::UI_PLAY>(); ++iButton) {
		dialog.GUI->Controls.Text		[iButton + 1].FontSelected	= 7;
		dialog.GUI->Controls.Draw		[iButton + 1].ColorTheme	= ::llc::ASCII_COLOR_GREEN * 16 + 6;
		dialog.GUI->Controls.Placement[iButton + 1].Align			= ::llc::ALIGN_TOP_LEFT;
		dialog.GUI->Controls.Placement[iButton + 1].Area.Offset.y	-= int16_t(18 - 2 * (iButton - 1));
		dialog.GUI->Controls.Placement[iButton + 1].Area.Size.y	+= 2;
	}

	// Setup new viewports if necessary. This may happen if the ship acquires new modules while playing the stage
	app.UIPlay.PlayerUI.resize(::ghg::MAX_PLAYERS);	
	for(uint32_t iPlayer = 0; iPlayer < ::ghg::MAX_PLAYERS; ++iPlayer) {
		::ghg::SUIPlayer			& uiPlayer			= app.UIPlay.PlayerUI[iPlayer];
		llc_necall(::uiPlayerSetupPlay(uiPlayer, app.DialogPerState[0].Input, app.DialogPerState[0].GUI->CursorPos, iPlayer, ""/*app.Game.Pilots[iPlayer].Name*/, ::ghg::MAX_ORBITER_COUNT), "iPlayer: %i", iPlayer);
	}
	return 0;
}

static	::llc::error_t	guiSetupAbout		(::ghg::SGalaxyHellApp & app, ::llc::SDialog & dialog) { return ::llc::guiCreateControlList<::ghg::UI_CREDITS>(*dialog.GUI, -1, 160, int16_t(-20 * ::llc::get_value_count<::ghg::UI_CREDITS>() / 2), ::llc::ALIGN_CENTER, ::llc::ALIGN_CENTER, app.DialogControls[::ghg::APP_STATE_About]); }
static	::llc::error_t	guiSetupSettings	(::ghg::SGalaxyHellApp & app, ::llc::SDialog & dialog) { return ::llc::guiCreateControlList<::ghg::UI_SETTINGS>(*dialog.GUI, -1, {160,20}, {}, ::llc::ALIGN_CENTER, ::llc::ALIGN_CENTER, app.DialogControls[::ghg::APP_STATE_Settings]); }

::llc::error_t			ghg::guiSetup		(::ghg::SGalaxyHellApp & app, const ::llc::pobj<::llc::SInput> & input) {
	if_true_e(failed(::dialogCreateCommon(app.DialogDesktop, input, {})));

	for(uint32_t iGUI = 0; iGUI < app.DialogPerState.size(); ++iGUI)
		if_true_ef(failed(::dialogCreateCommon(app.DialogPerState[iGUI], input, {})), "iGUI: %i", iGUI);

	if_true_e(failed(::guiSetupWelcome	(app, app.DialogPerState[::ghg::APP_STATE_Welcome	])));
	if_true_e(failed(::guiSetupHome	(app, app.DialogPerState[::ghg::APP_STATE_Home		])));
	if_true_e(failed(::guiSetupPlay	(app, app.DialogPerState[::ghg::APP_STATE_Play		])));
	if_true_e(failed(::guiSetupShop	(app, app.DialogPerState[::ghg::APP_STATE_Shop		])));
	if_true_e(failed(::guiSetupProfile	(app, app.DialogPerState[::ghg::APP_STATE_Profile	])));
	if_true_e(failed(::guiSetupSettings(app, app.DialogPerState[::ghg::APP_STATE_Settings	])));
	if_true_e(failed(::guiSetupAbout	(app, app.DialogPerState[::ghg::APP_STATE_About		])));
	if_true_e(failed(::guiSetupLoad	(app.DialogPerState[::ghg::APP_STATE_Load			])));

	if_true_e(failed(::llc::virtualKeyboardSetup437(*app.DialogDesktop.GUI, app.VirtualKeyboard)));;
	return 0;
}

static	::llc::error_t	guiHandleLoad		(::ghg::SGalaxyHellApp & app, ::llc::SGUI & gui, uint32_t idControl, ::ghg::SGalaxyHell & /*game*/) { 
	if(idControl < (gui.Controls.Text.size() - 2)) {
		try {
			if_true_ef(0 > ::ghg::solarSystemLoad(app.Game, gui.Controls.Text[idControl + 1].Text), "Failed to load file %i: '%s'", idControl + 1, gui.Controls.Text[idControl + 1].Text.begin());
		}
		catch (const sc_t * ) {
			::ghg::solarSystemReset(app.Game);
			return -1;
		}
		app.TunerPlayerCount->SetValue((uint8_t)app.Game.PlayState.Constants.Players);
		for(uint32_t iPilot = 0; iPilot < app.Game.PlayState.Constants.Players; ++iPilot) {
			const ::llc::vcst_t			namePilot	= app.Game.Pilots[iPilot].Name;
			for(uint32_t iPlayer = 0; iPlayer < app.Game.PlayState.Constants.Players; ++iPlayer) {
				if(iPlayer >= app.Players.size())
					app.AddNewPlayer(namePilot);

				const ::llc::vcst_t			namePlayer	= app.Players[iPlayer].Name;
				if(namePilot == namePlayer) {
					::std::swap(app.Players[iPlayer], app.Players[iPilot]);
					break;
				}
			}
			for(uint32_t iPlayer = app.Game.PlayState.Constants.Players; iPlayer < app.Players.size(); ++iPlayer) {
				const ::llc::vcst_t			namePlayer	= app.Players[iPlayer].Name;
				if(namePilot == namePlayer) {
					::std::swap(app.Players[iPlayer], app.Players[iPilot]);
					break;
				}
			}
		}
	}
	return ::ghg::APP_STATE_Home; 
}

static	::llc::error_t	dialogCreateLoad	(::ghg::SGalaxyHellApp & app, ::llc::SDialog & dialogLoad, ::llc::view<::llc::vcst_t> pathFileNames, const ::llc::pobj<::llc::SInput> & inputState, const ::llc::n2f2_t & cursorPos) { 
	::dialogCreateCommon(dialogLoad, inputState, cursorPos);
	return ::llc::guiCreateControlList(*dialogLoad.GUI, pathFileNames, dialogLoad.Root, {256, 20}, {0, int16_t(-20 * pathFileNames.size() / 2)}, ::llc::ALIGN_CENTER, ::llc::ALIGN_LEFT, app.DialogControls[::ghg::APP_STATE_Load]);
}

static	::llc::error_t	guiHandleHome		(::ghg::SGalaxyHellApp & app, ::llc::SGUI & /*gui*/, uint32_t idControl, ::ghg::SGalaxyHell & game) { 
	switch((::ghg::UI_HOME)idControl) {
	case ::ghg::UI_HOME_Start: 
		::ghg::solarSystemReset(game);
		game.PlayState.Constants.Players = (uint32_t)app.TunerPlayerCount->ValueCurrent;
		for(uint32_t iPlayer = app.Players.size(); iPlayer < app.Game.PlayState.Constants.Players; ++iPlayer) {
			sc_t				text [64]			= {};
			sprintf_s(text, "Player %i", app.Players.size() + 1);
			app.AddNewPlayer(text);
		}
		for(uint32_t iPlayer = 0; iPlayer < app.Players.size(); ++iPlayer) {
			if(iPlayer >= app.Game.Pilots.size())
				app.Game.PilotCreate({app.Players[iPlayer].Name, ::ghg::PLAYER_COLORS[iPlayer]});
			else
				app.Game.Pilots[iPlayer].Name = app.Players[iPlayer].Name;
		}
		game.PlayState.GlobalState.Paused	= false;
		return ::ghg::APP_STATE_Play;
	case ::ghg::UI_HOME_Continue: 
		game.PlayState.GlobalState.Paused	= false;
		return ::ghg::APP_STATE_Play;
	case ::ghg::UI_HOME_Save:
		if_true_e(0 > app.Save(::ghg::SAVE_MODE_USER));
		break;
	case ::ghg::UI_HOME_Load: {
		::llc::aobj<::llc::vcst_t>	pathFileNames;
		::ghg::listFilesSavegame(app, app.SavegameFolder, pathFileNames);

		pathFileNames.push_back(::llc::vcs{"Back"});
		::dialogCreateLoad(app, app.DialogPerState[::ghg::APP_STATE_Load], pathFileNames, app.DialogPerState[::ghg::APP_STATE_Home].Input, app.DialogPerState[::ghg::APP_STATE_Home].GUI->CursorPos);
		return ::ghg::APP_STATE_Load;
	}
	case ::ghg::UI_HOME_Settings: return ::ghg::APP_STATE_Settings; 
	case ::ghg::UI_HOME_Credits: return ::ghg::APP_STATE_About; 
	case ::ghg::UI_HOME_Shop: return ::ghg::APP_STATE_Shop; 
	case ::ghg::UI_HOME_Exit: 
		game.PlayState.GlobalState.Paused	= true;
		return ::ghg::APP_STATE_Quit;
	}
	return ::ghg::APP_STATE_Home; 
}

static	::llc::error_t	guiHandleUser		(::llc::SGUI & /*gui*/, uint32_t /*idControl*/, ::ghg::SGalaxyHell & /*game*/) { return ::ghg::APP_STATE_Profile; }
static	::llc::error_t	guiHandleShop		(::llc::SGUI & /*gui*/, uint32_t idControl, ::ghg::SGalaxyHell & /*game*/) { 
	if(idControl == (uint32_t)::ghg::UI_SHOP_Back) {
		return ::ghg::APP_STATE_Home;
	}
	return ::ghg::APP_STATE_Shop; 
}
static	::llc::error_t	guiHandlePlay		(::ghg::SGalaxyHellApp & app, ::llc::SGUI & /*gui*/, uint32_t idControl, ::ghg::SGalaxyHell & /*game*/) { 
	if(idControl == (uint32_t)::ghg::UI_PLAY_Menu) {
		app.Save(::ghg::SAVE_MODE_AUTO);
		return ::ghg::APP_STATE_Home;
	}
	return ::ghg::APP_STATE_Play; 
}

static	::llc::error_t	guiHandleWelcome	(::ghg::SGalaxyHellApp & app, ::llc::SGUI & /*gui*/, uint32_t idControl, ::ghg::SGalaxyHell & /*game*/) { 
	if(idControl == (uint32_t)::ghg::UI_WELCOME_Confirm && app.InputboxText.size()) {
		app.AddNewPlayer(::llc::vcst_t{app.InputboxText.begin(), app.InputboxText.size()});
		if(app.Game.Pilots.size())
			app.Game.Pilots[0] = {app.Players[0].Name, ::ghg::PLAYER_COLORS[0]};
		else
			app.Game.PilotCreate({app.Players[0].Name, ::ghg::PLAYER_COLORS[0]});

		app.Save(::ghg::SAVE_MODE_AUTO);
		return ::ghg::APP_STATE_Home;
	}
	return ::ghg::APP_STATE_Welcome; 
}
static	::llc::error_t	guiHandleSetup		(::llc::SGUI & /*gui*/, uint32_t idControl, ::ghg::SGalaxyHell & /*game*/) { 
	if(idControl == (uint32_t)::ghg::UI_SETTINGS_Audio) {
		return ::ghg::APP_STATE_Home;
	}
	else if(idControl == (uint32_t)::ghg::UI_SETTINGS_Graphics) {
		return ::ghg::APP_STATE_Home;
	}
	else if(idControl == (uint32_t)::ghg::UI_SETTINGS_Back) {
		return ::ghg::APP_STATE_Home;
	}
	return ::ghg::APP_STATE_Settings; 
}
static	::llc::error_t	guiHandleAbout		(::llc::SGUI & /*gui*/, uint32_t idControl, ::ghg::SGalaxyHell & /*game*/) { 
	if(idControl == (uint32_t)::ghg::UI_CREDITS_Back || idControl == (uint32_t)::ghg::UI_CREDITS_Home) {
		return ::ghg::APP_STATE_Home;
	}
	return ::ghg::APP_STATE_About; 
}

tplt<size_t _nStorageSize>
static	::llc::error_t	sprintfTime			(const sc_t *prefix, sc_t (&dest)[_nStorageSize], double seconds) {
	uint32_t					timeHours			= (int)seconds / 3600;
	uint32_t					timeMinutes			= (int)seconds / 60 % 60;
	uint32_t					timeSeconds			= (int)seconds % 60;
	uint32_t					timeCents			= int (seconds * 10) % 10;
	sprintf_s(dest, "%s%i:%.2i:%.2i.%.2i", prefix, timeHours, timeMinutes, timeSeconds, timeCents);
	return 0;
}

tplt<size_t _bufSize>
static	::llc::error_t	sprintfScore		(sc_t (&buffer)[_bufSize], const ::llc::SSpaceshipScore & shipScore, const double nitro) { 
	return sprintf_s(buffer, "%c %llu  %c %llu  %c %llu  %c %llu  %c %.2f", 4, shipScore.Score, 94, shipScore.Shots, 1, (uint64_t)shipScore.KilledShips, 2, (uint64_t)shipScore.KilledOrbiters, 24, nitro);
}

static	::llc::error_t	uiPlayerUpdateHome	(::ghg::SUIPlayer & uiPlayer, uint16_t iPlayer, const ::llc::vcst_t playerName, const ::llc::bgra & shipColor, const ::llc::SSpaceshipScore & shipScore, const double nitro) { 
	sprintfScore(uiPlayer.TextScore.Storage, shipScore, nitro);
	::llc::SDialog				& playerDialog		= uiPlayer.DialogHome;
	::llc::SGUI					& playerGUI			= *playerDialog.GUI;
	llc_necs(::llc::controlTextSet(playerGUI, playerDialog.Root + 1 + ::ghg::UI_PROFILE_Score, uiPlayer.TextScore.Storage));
	llc_necs(::llc::controlTextSet(playerGUI, playerDialog.Root + 1 + ::ghg::UI_PROFILE_Name, playerName));
	{
		::llc::SControlDraw			& controlDraw		= playerGUI.Controls.Draw[playerDialog.Root + 1 + ::ghg::UI_PROFILE_Name];
		controlDraw.ColorTheme		= int16_t(3 + iPlayer);
		const ::llc::SControlTheme	& theme				= (*playerGUI.Colors->ControlThemes)[(0 == controlDraw.ColorTheme) ? playerGUI.ThemeDefault : controlDraw.ColorTheme - 1];
		const ::llc::astu32<::llc::UI_CONTROL_AREA_COUNT>	& colorCombo	= theme.ColorCombos[::llc::GUI_CONTROL_PALETTE_NORMAL];
		(*playerGUI.Colors->Palette)[colorCombo[::llc::UI_CONTROL_AREA_TEXT_FACE	]] = shipColor;
		(*playerGUI.Colors->Palette)[colorCombo[::llc::UI_CONTROL_AREA_BACKGROUND	]] = shipColor;
		playerGUI.Controls.Draw[uiPlayer.InputBox.IdText].ColorTheme = controlDraw.ColorTheme;
		playerGUI.Controls.Draw[uiPlayer.InputBox.IdText].NoClient = true;
		//playerGUI.Controls.Draw[uiPlayer.InputBox.IdText].NoBorder = true;

	}
	return 0;
}

static	::llc::error_t	uiPlayerUpdatePlay	(::ghg::SUIPlayer & uiPlayer, uint32_t iPlayer, const ::ghg::SGalaxyHell & game, const ::llc::SSpaceshipScore & shipScore, ::std::mutex & lockGame, ::ghg::SGalaxyHellDrawCache & drawCache) {
	::llc::SDialog				& playerDialog		= uiPlayer.DialogPlay;
	::llc::SGUI					& playerGUI			= *playerDialog.GUI;

	const ::llc::SSpaceshipCore		& shipCore			= game.ShipState.SpaceshipManager.ShipCores[iPlayer];
	sprintfScore(uiPlayer.TextScore.Storage, shipScore, shipCore.Nitro.Value);
	const ::llc::rgbaf			shipColor			= (shipCore.Team ? ::llc::RED : ::llc::rgbaf(game.Pilots[iPlayer].Color));
	llc_necs(::llc::controlTextSet(playerGUI, 1 + ::ghg::UI_PILOT_Name	, game.Pilots[iPlayer].Name));
	llc_necs(::llc::controlTextSet(playerGUI, 1 + ::ghg::UI_PILOT_Score, uiPlayer.TextScore.Storage));
	{
		::llc::SControlDraw			& controlDraw		= playerGUI.Controls.Draw[1 + ::ghg::UI_PILOT_Name];
		controlDraw.ColorTheme	= int16_t(3 + iPlayer);
		const ::llc::SControlTheme	& theme				= (*playerGUI.Colors->ControlThemes)[(0 == controlDraw.ColorTheme) ? playerGUI.ThemeDefault : controlDraw.ColorTheme - 1];
		const ::llc::astu32<::llc::UI_CONTROL_AREA_COUNT>	& colorCombo			= theme.ColorCombos[::llc::GUI_CONTROL_PALETTE_NORMAL];
		(*playerGUI.Colors->Palette)[colorCombo[::llc::UI_CONTROL_AREA_TEXT_FACE	]] = shipColor;
		(*playerGUI.Colors->Palette)[colorCombo[::llc::UI_CONTROL_AREA_BACKGROUND	]] = shipColor;
	}
	for(uint32_t iOrbiter = 0, countViewports = game.ShipState.SpaceshipManager.ShipParts[iPlayer].size(); iOrbiter < countViewports; ++iOrbiter) {
		::ghg::SUIPlayShipPartViewport	& viewport		= *uiPlayer.ModuleViewports[iOrbiter];

		const ::llc::SSpaceshipOrbiter	& orbiter		= game.ShipState.SpaceshipManager.Orbiters[game.ShipState.SpaceshipManager.ShipParts[iPlayer][iOrbiter]];
		const ::llc::SWeapon			& weapon		= game.ShipState.WeaponManager.Weapons[orbiter.Weapon];

		llc_necall(::llc::controlTextSet(playerGUI, viewport.Viewport + 1, ::llc::get_value_label(weapon.Shot.Type)), "%s", "");
		llc_necall(::llc::controlTextSet(playerGUI, viewport.Viewport + 2, ::llc::get_value_label(weapon.Type)), "%s", "");

		const float					healthRatio		= (float)::llc::clamped(orbiter.Health.Weight(), 0.0, 1.0);
		const float					ratioOverheat	= (orbiter.Health.Value > 0) ? (float)weapon.Trigger.Overheat.WeightClamp() : 0;
		const float					ratioDelay		= (orbiter.Health.Value > 0) ? (float)weapon.Trigger.Delay  .WeightClamp() : 0;

		const ::llc::rgbaf			colorLife		= ::llc::interpolate_linear(::llc::RED, ::llc::GREEN, healthRatio);
		const ::llc::rgbaf			colorDelay		= ::llc::interpolate_linear(::llc::GRAY * .5, ::llc::YELLOW, ratioDelay);;
		const ::llc::rgbaf			colorCooldown	= ::llc::interpolate_linear(::llc::LIGHTBLUE * ::llc::CYAN, ::llc::ORANGE * .5 + ::llc::RED * .5, ratioOverheat);

		viewport.GaugeLife.SetValue(healthRatio	);

		{ // Update ship part render and health bar
			::llc::m4f2_t				matrixView		= {};
			::llc::SCameraPoints		& camera		= viewport.Camera;
			camera.Target			= game.ShipState.Scene.Transforms[game.ShipState.EntitySystem.Entities[game.ShipState.ShipPartEntity[iOrbiter] + 1].Transform].GetTranslation();
			camera.Position			= camera.Target;
			camera.Position.y		+= 7; //-= 20;
			stacxpr	::llc::n3f2_t		cameraUp		= {1, 0, 0};
			matrixView.LookAt(camera.Position, camera.Target, cameraUp);
			matrixView				*= uiPlayer.ModuleViewports[iOrbiter]->MatrixProjection;

			::ghg::TRenderTarget		& renderTarget	= viewport.RenderTargetOrbiter;
			::llc::g8bgra	targetPixels	= renderTarget.Color		; 
			::llc::grid<uint32_t>		depthBuffer		= renderTarget.DepthStencil	;
			//targetPixels.fill(tone);
			::std::lock_guard			lock			(lockGame);
			memset(targetPixels.begin(), 0, targetPixels.byte_count());
			memset(depthBuffer.begin(), -1, depthBuffer.byte_count());

			drawCache.PixelCoords.clear();
			uint32_t					pixelsDrawn		= 0;
			pixelsDrawn				+= ::ghg::drawOrbiter(game.ShipState, game.ShipState.ShipPartEntity[iOrbiter], {}, (float)game.DecoState.AnimationTime, matrixView, targetPixels, depthBuffer, drawCache);

			if(healthRatio) {
				llc_necs(::ghg::gaugeImageUpdate(viewport.GaugeLife, targetPixels, ::llc::RED, ::llc::YELLOW, ::llc::LIGHTGREEN));
			}
		}

		{ // Update weapon load bar
			::ghg::TRenderTarget		& renderTarget	= viewport.RenderTargetWeaponLoad;
			::llc::g8bgra	targetPixels	= renderTarget.Color		; 
			::llc::grid<uint32_t>		depthBuffer		= renderTarget.DepthStencil	;
			::std::lock_guard			lock			(lockGame);
			memset(targetPixels.begin(), 0, targetPixels.byte_count());
			memset(depthBuffer.begin(), -1, depthBuffer.byte_count());

			if(ratioDelay) {
				drawCache.PixelCoords.clear();
				for(uint32_t iLine = 0; iLine < targetPixels.metrics().y; ++iLine)
					::llc::drawLine(targetPixels.metrics().u1(), ::llc::line2<int16_t>{{0, (int16_t)iLine}, {int16_t((targetPixels.metrics().x - 1) * ratioDelay), (int16_t)iLine}}, drawCache.PixelCoords);
				
				for(uint32_t iPixel = 0; iPixel < drawCache.PixelCoords.size(); ++iPixel) {
					const ::llc::n2s1_t		& pixelCoord = drawCache.PixelCoords[iPixel];
					const ::llc::n2f2_t		floatCoord			= pixelCoord.f2();
					const double			distanceFromCenter	= fabs((floatCoord.y / (targetPixels.metrics().y)) - .5) * 2.0;
					::llc::setPixel(targetPixels, pixelCoord, ::llc::interpolate_linear(colorDelay, ::llc::GRAY * .25, distanceFromCenter));
				}
			}
		}

		{ // Update weapon type bar
			::std::lock_guard			lock			(lockGame);
			::ghg::TRenderTarget		& renderTarget	= viewport.RenderTargetWeaponType;
			::llc::g8bgra	targetPixels	= renderTarget.Color		; 
			::llc::grid<uint32_t>		depthBuffer		= renderTarget.DepthStencil	;
			memset(targetPixels.begin(), 0, targetPixels.byte_count());
			memset(depthBuffer.begin(), -1, depthBuffer.byte_count());
			if(ratioOverheat) {
				drawCache.PixelCoords.clear();
				for(uint32_t iLine = 0; iLine < targetPixels.metrics().y; ++iLine)
					::llc::drawLine(targetPixels.metrics().u1(), ::llc::line2<int16_t>{{0, (int16_t)iLine}, {int16_t((targetPixels.metrics().x - 1) * ratioOverheat), (int16_t)iLine}}, drawCache.PixelCoords);
				
				for(uint32_t iPixel = 0; iPixel < drawCache.PixelCoords.size(); ++iPixel) {
					const ::llc::n2s1_t			& pixelCoord		= drawCache.PixelCoords[iPixel];
					const ::llc::n2f2_t			floatCoord			= pixelCoord.f2();
					const double				distanceFromCenter	= fabs((floatCoord.y / (targetPixels.metrics().y)) - .5) * 2.0;
					::llc::setPixel(targetPixels, drawCache.PixelCoords[iPixel], ::llc::interpolate_linear(colorCooldown, ::llc::GRAY * .25, distanceFromCenter));
				}
			}
		}
	}
	return 0;
}

static	::llc::error_t	guiUpdatePlay		(::ghg::SGalaxyHellApp & app) { 
	::ghg::SGalaxyHell			& game				= app.Game;

	::llc::SDialog				& dialog			= app.DialogPerState[::ghg::APP_STATE_Play];
	::llc::SGUI					& gui				= *dialog.GUI;

	sprintf_s(app.UIPlay.TextLevel.Storage, "Level: %i", game.PlayState.GlobalState.Stage);
	sprintfTime("Total Time: "	, app.UIPlay.TextTimeReal	.Storage, game.PlayState.GlobalState.UserTime.Played);
	sprintfTime("Stage Time: "	, app.UIPlay.TextTimeStage	.Storage, game.PlayState.TimeRealStage);
	::llc::controlTextSet(gui, 1 + ::ghg::UI_PLAY_Level		, ::llc::vcs{app.UIPlay.TextLevel		.Storage});
	::llc::controlTextSet(gui, 1 + ::ghg::UI_PLAY_TimeStage , ::llc::vcs{app.UIPlay.TextTimeStage	.Storage});
	::llc::controlTextSet(gui, 1 + ::ghg::UI_PLAY_TimeReal	, ::llc::vcs{app.UIPlay.TextTimeReal	.Storage});

	if(0 == game.ShipState.SpaceshipManager.ShipCores.size())
		return 0;

	for(uint32_t iPlayer = 0; iPlayer < app.TunerPlayerCount->ValueCurrent &&	iPlayer < app.UIPlay.PlayerUI.size(); ++iPlayer) { app.UIPlay.PlayerUI[iPlayer].DialogHome.GUI->Controls.SetHidden(0, false); }
	for(uint32_t iPlayer = (uint32_t)app.TunerPlayerCount->ValueCurrent;		iPlayer < app.UIPlay.PlayerUI.size(); ++iPlayer) { app.UIPlay.PlayerUI[iPlayer].DialogHome.GUI->Controls.SetHidden(0, true); }

	::ghg::SGalaxyHellDrawCache	& drawCache			= app.UIPlay.DrawCache;
	drawCache.LightPointsWorld.clear();
	drawCache.LightColorsWorld.clear();
	::ghg::getLightArraysFromShips(game.ShipState, drawCache.LightPointsWorld, drawCache.LightColorsWorld);
	drawCache.LightPointsModel.reserve(drawCache.LightPointsWorld.size());
	drawCache.LightColorsModel.reserve(drawCache.LightColorsWorld.size());
	if(app.ActiveState == ::ghg::APP_STATE_Play) {
		for(uint32_t iPlayer = 0; iPlayer < game.PlayState.Constants.Players; ++iPlayer) {
			::ghg::SUIPlayer			& uiPlayer			= app.UIPlay.PlayerUI[iPlayer];
			llc_necall(::uiPlayerUpdatePlay(uiPlayer, iPlayer, app.Game, app.Game.ShipState.SpaceshipManager.ShipScores[iPlayer], app.Game.LockUpdate, drawCache), "iPlayer: %i", iPlayer);
			uiPlayer.DialogPlay.GUI->Controls.SetHidden(0, false);
		}
		for(uint32_t iPlayer = app.Game.PlayState.Constants.Players; iPlayer < ghg::MAX_PLAYERS; ++iPlayer) { 
			app.UIPlay.PlayerUI[iPlayer].DialogPlay.GUI->Controls.SetHidden(0, true); 
		}
	}
	return 0;
}

static	::llc::error_t	guiUpdateHome				(::ghg::SGalaxyHellApp & app, ::llc::vpobj<::llc::SEventSystem> frameEvents) {
	for(uint32_t iPlayer = 0; iPlayer < app.TunerPlayerCount->ValueCurrent; ++iPlayer) {
		::ghg::SUIPlayer			& uiPlayer					= app.UIPlay.PlayerUI[iPlayer];
		::llc::SDialog				& dialog					= uiPlayer.DialogHome;
		::llc::SGUI					& gui						= *dialog.GUI;
		gui.Controls.SetHidden(dialog.Root, false); 

		if(app.Players.size() <= iPlayer) {
			::std::lock_guard			lock						(app.Game.LockUpdate);
			sc_t text [64] = {};
			sprintf_s(text, "Player %i", app.Players.size() + 1);
			app.AddNewPlayer(text);
		}

		::llc::rgbaf				shipColor					= ::ghg::PLAYER_COLORS[iPlayer];
		double						nitro						= 0;
		if((iPlayer < app.Game.ShipState.SpaceshipManager.ShipCores.size())) {
			const ::llc::SSpaceshipCore		& shipCore				= app.Game.ShipState.SpaceshipManager.ShipCores[iPlayer];
			shipColor				= (shipCore.Team ? ::ghg::PLAYER_COLORS[iPlayer] : ::llc::bgra(app.Game.Pilots[iPlayer].Color));
			nitro					= shipCore.Nitro.Value;
		}

		llc_necall(::uiPlayerUpdateHome(uiPlayer, int16_t(iPlayer), app.Players[iPlayer].Name, shipColor
			, (app.Game.ShipState.SpaceshipManager.ShipScores.size() <= iPlayer) ? ::llc::SSpaceshipScore{} : app.Game.ShipState.SpaceshipManager.ShipScores[iPlayer]
			, nitro), "iPlayer: %i", iPlayer
		);

		::llc::acid					controlsToProcess			= {};
		::llc::guiGetProcessableControls(gui, controlsToProcess);
		if(int32_t result = uiPlayer.InputBox.Update(gui, uiPlayer.InputBox.VirtualKeyboard, frameEvents, controlsToProcess)) {
			if(result == INT_MAX) {
				::llc::vcst_t					trimmed						= {uiPlayer.InputBox.Text.begin(), ::llc::min(uiPlayer.InputBox.Text.size(), 16U)};
				::llc::trim(trimmed);
				if(trimmed.size()) {
					app.Players[iPlayer].Name	= ::llc::label(trimmed);
					if(iPlayer < app.Game.Pilots.size())
						app.Game.Pilots[iPlayer].Name	= app.Players[iPlayer].Name;
				}
				uiPlayer.InputBox.Edit(gui, false);
			}
		}
		else {
			::llc::guiProcessControls(gui, controlsToProcess, [&](uint32_t iControl) {
				uint32_t					offsetControl				= dialog.Root + 1;
				switch(iControl - (offsetControl)) {
				case ::ghg::UI_PROFILE_Name:
					uiPlayer.InputBox.SetText(gui, gui.Controls.Text[offsetControl + ::ghg::UI_PROFILE_Name].Text);
					uiPlayer.InputBox.Edit(gui, true);
				}
				return 0;
			});
		}
	}

	for(uint32_t iPlayer = (uint32_t)app.TunerPlayerCount->ValueCurrent; iPlayer < ghg::MAX_PLAYERS; ++iPlayer) {
		::ghg::SUIPlayer			& uiPlayer		= app.UIPlay.PlayerUI[iPlayer];
		::llc::SDialog				& dialog		= uiPlayer.DialogHome;
		::llc::SGUI					& gui			= *dialog.GUI;
		gui.Controls.SetHidden(dialog.Root, true);
	}

	return 0;
}

::llc::error_t			ghg::guiUpdate				(::ghg::SGalaxyHellApp & app, ::llc::vpobj<::llc::SEventSystem> sysEvents) {
	{
		::llc::SDialog				& dialog					= app.DialogDesktop;
		::llc::SGUI					& gui						= *dialog.GUI;
		::llc::SInput				& input						= *dialog.Input;
		::llc::guiProcessInput(gui, input, sysEvents); 
	}

	::ghg::SGalaxyHell			& game						= app.Game;

	::ghg::APP_STATE			appState					= app.ActiveState; 
	for(uint32_t iAppState = 0; iAppState < ::ghg::APP_STATE_COUNT; ++iAppState) {
		::llc::SDialog				& dialog					= app.DialogPerState[iAppState];
		::llc::SGUI					& gui						= *dialog.GUI;
		::llc::SInput				& input						= *dialog.Input;
		::llc::guiProcessInput(gui, input, sysEvents); 
	}

	for(uint32_t iPlayer = 0; iPlayer < app.UIPlay.PlayerUI.size(); ++iPlayer) {
		::ghg::SUIPlayer			& uiPlayer					= app.UIPlay.PlayerUI[iPlayer];
		::llc::guiProcessInput(*uiPlayer.DialogPlay.GUI, *uiPlayer.DialogPlay.Input, sysEvents);
		::llc::guiProcessInput(*uiPlayer.DialogHome.GUI, *uiPlayer.DialogHome.Input, sysEvents);
	}
	
	if(app.ActiveState == ::ghg::APP_STATE_Play)
		::guiUpdatePlay(app);
	else 
		::guiUpdateHome(app, sysEvents);

	::llc::SDialog				& dialog					= app.DialogPerState[appState];
	::llc::SGUI					& gui						= *dialog.GUI;
	dialog.Update();
	::llc::acid					controlsToProcess			= {};
	::llc::guiProcessControls(gui, [&app, &gui, &game, &appState](::llc::cid_t idControl) {
			idControl = idControl - 1;
			info_printf("Executed %u.", idControl);
			switch(appState) {
			case ::ghg::APP_STATE_Play		: return appState = (::ghg::APP_STATE)::guiHandlePlay		(app, gui, idControl, game);
			case ::ghg::APP_STATE_Home		: return appState = (::ghg::APP_STATE)::guiHandleHome		(app, gui, idControl, game);
			case ::ghg::APP_STATE_Welcome	: return appState = (::ghg::APP_STATE)::guiHandleWelcome	(app, gui, idControl, game);
			case ::ghg::APP_STATE_Load		: return appState = (::ghg::APP_STATE)::guiHandleLoad		(app, gui, idControl, game);
			case ::ghg::APP_STATE_Profile	: return appState = (::ghg::APP_STATE)::guiHandleUser		(gui, idControl, game);
			case ::ghg::APP_STATE_Shop		: return appState = (::ghg::APP_STATE)::guiHandleShop		(gui, idControl, game);
			case ::ghg::APP_STATE_Settings	: return appState = (::ghg::APP_STATE)::guiHandleSetup		(gui, idControl, game);
			case ::ghg::APP_STATE_About		: return appState = (::ghg::APP_STATE)::guiHandleAbout		(gui, idControl, game);
			}
			return appState;
		});

	return appState;
}

::llc::error_t			ghg::gaugeBuildRadial			(::ghg::SUIRadialGauge & gauge, const ::llc::circlef2_t & gaugeMetrics, int16_t resolution, int16_t width) {
	const double				stepUnit						= (1.0 / resolution) * ::llc::math_2pi;
	const ::llc::spheref2_t		sphereMetrics					= {gaugeMetrics.Radius, {gaugeMetrics.Center.x, gaugeMetrics.Center.y, .5f}};
	const double				finalRadius						= gaugeMetrics.Radius;	//::llc::interpolate_linear(gaugeMetrics.Radius, gaugeMetrics.Radius * .5, ::llc::clamped(abs(sinCos.Cos), 0.0, 1.0)); //
	const double				outerRadius						= finalRadius;			//::llc::interpolate_linear(gaugeMetrics.Radius, gaugeMetrics.Radius * .5, ::llc::clamped(abs(sinCos.Cos), 0.0, 1.0)); //
	const double				innerRadius						= outerRadius - width;	//::llc::interpolate_linear(gaugeMetrics.Radius, gaugeMetrics.Radius * .5, ::llc::clamped(abs(sinCos.Cos), 0.0, 1.0)); //
	for(int16_t iStep = 0, stepCount = resolution; iStep < stepCount; ++iStep) {
		::llc::SSinCos				sinCos							= {sin(iStep * stepUnit), -cos(iStep * stepUnit)};
		const ::llc::n3f3_t			relativePosSmall				=
			{ sinCos.Sin * innerRadius
			, sinCos.Cos * innerRadius
			};
		const ::llc::n3f3_t			relativePos						=
			{ sinCos.Sin * outerRadius
	  		, sinCos.Cos * outerRadius
			};
		gauge.Vertices.push_back(sphereMetrics.Center + relativePosSmall.f2());
		gauge.Vertices.push_back(sphereMetrics.Center + relativePos.f2());
	}
	for(int16_t iStep = 0, stepCount = (int16_t)(resolution * 2) - 3; iStep < stepCount; ++iStep) {
		gauge.Indices.push_back({int16_t(0 + iStep), int16_t(1 + iStep), int16_t(2 + iStep)});
		gauge.Indices.push_back({int16_t(1 + iStep), int16_t(3 + iStep), int16_t(2 + iStep)});
	}
	const int16_t				iStep							= int16_t(gauge.Vertices.size() - 2);
	gauge.Indices.push_back({int16_t(0 + iStep), int16_t(1 + iStep), 0});
	gauge.Indices.push_back({int16_t(1 + iStep), 1, 0});
	gauge.MaxValue			= resolution;
	return 0;
}


::llc::error_t			ghg::gaugeImageUpdate			(::ghg::SUIRadialGauge & gauge, ::llc::g8bgra target, ::llc::rgbaf colorMin, ::llc::rgbaf colorMid, ::llc::rgbaf colorMax, ::llc::bgra colorEmpty, bool radialColor)  {
	static ::llc::imgu2_t		dummyDepth;
	const ::llc::n3f2_t			center3							= (gauge.Vertices[1] - gauge.Vertices[gauge.Vertices.size() / 2 + 1]) / 2 + gauge.Vertices[gauge.Vertices.size() / 2 + 1];
	const ::llc::n2f2_t			center2							= {center3.x, center3.y};
	const double				radiusLarge						= (center3 - gauge.Vertices[1]).Length();
	const double				radiusSmall						= (center3 - gauge.Vertices[0]).Length();
	const double				radiusHalfWidth					= (radiusLarge - radiusSmall) / 2;
	const double				radiusCenter					= radiusHalfWidth + radiusSmall;
	dummyDepth.resize(target.metrics(), 0xFFFFFFFFU);
	::llc::apod<::llc::n2s1_t>		pixelCoords;
	::llc::apod<::llc::trif2_t>		triangleWeights;
	for(uint32_t iTriangle = 0, triangleCount = gauge.Indices.size(); iTriangle < triangleCount; ++iTriangle) {
		const ::llc::tri3f2_t			triangleCoords				=
			{ gauge.Vertices[gauge.Indices[iTriangle].A]
			, gauge.Vertices[gauge.Indices[iTriangle].B]
			, gauge.Vertices[gauge.Indices[iTriangle].C]
			};
		const double					colorFactor					= radialColor ? ::llc::min(1.0, iTriangle / (double)triangleCount) : gauge.CurrentValue / (float)gauge.MaxValue;
		::llc::rgbaf					finalColor;
		const bool						isEmptyGauge				= ((iTriangle + 2) >> 1) >= (uint32_t)gauge.CurrentValue * 2;
		finalColor					= isEmptyGauge
			? ::llc::rgbaf(colorEmpty)
			: (colorFactor < .5)
				? ::llc::interpolate_linear(colorMin, colorMid, (::llc::min(1.0, iTriangle * 2.25 / (double)triangleCount)))
				: ::llc::interpolate_linear(colorMid, colorMax, (colorFactor - .5) * 2)
				;
		finalColor.Clamp();
		pixelCoords.clear();
		::llc::drawTriangle(target.metrics(), triangleCoords, pixelCoords, triangleWeights, dummyDepth);
		for(uint32_t iPixelCoords = 0; iPixelCoords < pixelCoords.size(); ++iPixelCoords) {
			const ::llc::n2s1_t		pixelCoord					= pixelCoords[iPixelCoords];
//#define GAUGE_NO_SHADING
#ifndef GAUGE_NO_SHADING
			const ::llc::n2f2_t				floatCoord					= pixelCoord.f2();
			const double					distanceFromCenter			= (floatCoord - center2).Length();
			const double					distanceFromRadiusCenter	= fabs(distanceFromCenter - radiusCenter) / radiusHalfWidth;
			finalColor.a				= (float)(1.f - distanceFromRadiusCenter);
#else
			(void)radiusCenter;
#endif		
			::llc::bgra						& targetPixel				= target[pixelCoord.y][pixelCoord.x];
			targetPixel					= ::llc::interpolate_linear(::llc::rgbaf{targetPixel}, finalColor, finalColor.a);
			targetPixel.a				= uint8_t(finalColor.a * 255);
		}
	}
	return 0;
}


