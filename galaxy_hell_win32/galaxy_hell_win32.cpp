#include "galaxy_hell_win32.h"

#include "llc_png.h"
#include "llc_grid_copy.h"
#include "llc_raster_lh.h"

#include <time.h>
#include <direct.h>

//#include <joystickapi.h>

//#define LLC_AVOID_LOCAL_APPLICATION_MODULE_MODEL_EXECUTABLE_RUNTIME
#include "llc_app_impl.h"

//#define USE_STEAM_CLIENT
//#define STEAM_CEG
#if defined(USE_STEAM_CLIENT)
#include "steam/steam_api.h"
#include "steam/isteamuserstats.h"
#include "steam/isteamremotestorage.h"
#include "steam/isteammatchmaking.h"
#include "steam/steam_gameserver.h"

#if defined(STEAM_CEG)
// Steam DRM header file
#include "cegclient.h"
#else
#define Steamworks_InitCEGLibrary() (true)
#define Steamworks_TermCEGLibrary()
#define Steamworks_TestSecret()
#define Steamworks_SelfCheck()
#endif
#endif

LLC_DEFINE_APPLICATION_ENTRY_POINT(::SApplication, "Galaxy Hell v0.4");

static ::llc::error_t presentFrame(SApplication & app, const ::ghg::TRenderTarget & frame) {
	const ::llc::SWindow & window = app.Framework.RootWindow;
	if(window.NoDraw || 0 == window.Size.x || 0 == window.Size.y || frame.Color.metrics() != window.Size.u2())
		return 0;
	::llc::SD3DApplication & d3d = *app.D3D;
	if(d3d.GUIStuff.RenderTarget.metrics() != window.Size.u2())
		llc_necs(d3d.SetWindowSize(window.Size));
	memcpy(d3d.GUIStuff.RenderTarget.begin(), frame.Color.begin(), frame.Color.byte_count());
	for(auto & pixel : d3d.GUIStuff.RenderTarget.Texels)
		pixel.a = 255; // Only the upload copy is opaque; the completed CPU frame is unchanged.
	auto & resources = *d3d.DeviceResources;
	auto context = resources.GetD3DDeviceContext();
	ID3D11RenderTargetView * target = resources.GetBackBufferRenderTargetView();
	context->OMSetRenderTargets(1, &target, resources.GetDepthStencilView());
	const D3D11_VIEWPORT viewport = resources.GetScreenViewport();
	context->RSSetViewports(1, &viewport);
	const ::llc::rgbaf clear = {0, 0, 0, 1};
	context->ClearRenderTargetView(target, &clear.r);
	llc_necs(::llc::d3dGUIDraw(resources, d3d.GUIStuff));
	return resources.Present();
}

static	::llc::error_t	processScreenEvent		(::SApplication & /*app*/, const ::llc::SEventView<::llc::EVENT_SCREEN> & screenEvent) { 
	switch(screenEvent.Type) {
	default: break;
	case ::llc::EVENT_SCREEN_Create:
//#if !defined(DISABLE_D3D11)
//		llc_necs(app.D3DApp.Initialize(app.Framework.RootWindow.PlatformDetail.WindowHandle, app.Engine.Scene->Graphics));
//#endif
	case ::llc::EVENT_SCREEN_Resize: 
		//llc_necs(::updateSizeDependentResources(app));
		break;
	}
	return 0;
}

static	::llc::error_t	processKeyboardEvent	(::SApplication & app, const ::llc::SEventView<::llc::EVENT_KEYBOARD> & screenEvent) { 
	switch(screenEvent.Type) {
	default: break;
	case ::llc::EVENT_KEYBOARD_Down:
	case ::llc::EVENT_KEYBOARD_SysDown:
		switch(screenEvent.Data[0]) {
		case VK_RETURN:
			if(GetAsyncKeyState(VK_MENU) & 0xFFF0)
				llc_necs(::llc::fullScreenToggle(app.Framework.RootWindow));
			break;
		}
		break;
	}
	return 0;
}

static	::llc::error_t	processSystemEvent	(::SApplication & app, const ::llc::SEventSystem & sysEvent) { 
	switch(sysEvent.Type) {
	default: break;
	case ::llc::SYSTEM_EVENT_Screen		: llc_necs(::llc::eventExtractAndHandle<::llc::EVENT_SCREEN  >(sysEvent, [&app](const ::llc::SScreenEventView & screenEvent) { return ::processScreenEvent  (app, screenEvent); })); break;
	case ::llc::SYSTEM_EVENT_Keyboard	: llc_necs(::llc::eventExtractAndHandle<::llc::EVENT_KEYBOARD>(sysEvent, [&app](const ::llc::SEventView<::llc::EVENT_KEYBOARD> & screenEvent) { return ::processKeyboardEvent(app, screenEvent); })); break;
	}
	return 0;
}

::llc::error_t			cleanup				(::SApplication & app)						{ 
	::llc::SFramework			& framework			= app.Framework;
	::llc::SWindow				& mainWindow		= framework.RootWindow;
	::ghg::galaxyHellUpdate(app.GalaxyHellApp, framework.FrameInfo.Seconds.LastFrame, framework.RootWindow.Input, mainWindow.EventQueue);
	app.AudioState.CleanupAudio(); 

	if(app.D3D) {
		if(app.D3D->DeviceResources && app.D3D->DeviceResources->m_swapChain)
			if_fail_e(app.D3D->Shutdown());
		app.D3D = {};
	}
	if(app.COMInitialized) {
		::CoUninitialize();
		app.COMInitialized = false;
	}
#ifdef USE_STEAM_CLIENT
	SteamAPI_Shutdown();
	Steamworks_TermCEGLibrary();
#endif
	//::llc::clientDisconnect(app.Client);
	//::llc::tcpipShutdown();
	return ::llc::mainWindowDestroy(app.Framework.RootWindow); 
}

#ifdef USE_STEAM_CLIENT
//-----------------------------------------------------------------------------
// Purpose: callback hook for debug text emitted from the Steam API
//-----------------------------------------------------------------------------
extern "C" void __cdecl SteamAPIDebugTextHook( int nSeverity, const sc_t *pchDebugText ) {
	// if you're running in the debugger, only warnings (nSeverity >= 1) will be sent
	// if you add -debug_steamapi to the command-line, a lot of extra informational messages will also be sent
	error_printf("%s", pchDebugText );
	if ( nSeverity >= 1 ) {
		int x = 3;	// place to set a breakpoint for catching API errors
		(void)x;
	}
}

// Extracts some feature from the command line
bool ParseCommandLine( const sc_t *pchCmdLine, const sc_t **ppchServerAddress, const sc_t **ppchLobbyID ) {
	// Look for the +connect ipaddress:port parameter in the command line, Steam will pass this when a user has used the Steam Server browser to find a server for our game and is trying to join it. 
	const sc_t *pchConnectParam = "+connect ";
	const sc_t *pchConnect = strstr( pchCmdLine, pchConnectParam );
	*ppchServerAddress = NULL;
	if ( pchConnect && strlen( pchCmdLine ) > (pchConnect - pchCmdLine) + strlen( pchConnectParam ) )
		*ppchServerAddress = pchCmdLine + ( pchConnect - pchCmdLine ) + strlen( pchConnectParam );	// Address should be right after the +connect

	// look for +connect_lobby lobbyid paramter on the command line. Steam will pass this in if a user taken up an invite to a lobby
	const sc_t *pchConnectLobbyParam = "+connect_lobby ";
	const sc_t *pchConnectLobby = strstr( pchCmdLine, pchConnectLobbyParam );
	*ppchLobbyID = NULL;
	if ( pchConnectLobby && strlen( pchCmdLine ) > (pchConnectLobby - pchCmdLine) + strlen( pchConnectLobbyParam ) )
		*ppchLobbyID = pchCmdLine + ( pchConnectLobby - pchCmdLine ) + strlen( pchConnectLobbyParam );	// lobby ID should be right after the +connect_lobby

	return *ppchServerAddress || *ppchLobbyID;
}
#endif

::llc::error_t			setup				(::SApplication & app)						{
#ifdef USE_STEAM_CLIENT
	// if Steam is not running or the game wasn't started through Steam, SteamAPI_RestartAppIfNecessary starts the local Steam client and also launches this game again.
	// Once you get a public Steam AppID assigned for this game, you need to replace k_uAppIdInvalid with it and removed steam_appid.txt from the game depot.
	if_true_fef(SteamAPI_RestartAppIfNecessary( k_uAppIdInvalid ), "%s", "");
	if_true_fef(!Steamworks_InitCEGLibrary(), "%s", "Steamworks_InitCEGLibrary() failed\n" );	// Init Steam CEG

	// Initialize SteamAPI, if this fails we bail out since we depend on Steam for lots of stuff.
	// You don't necessarily have to if you write your code to check whether all the Steam interfaces are NULL before using them and provide alternate paths when they are unavailable.
	// This will also load the in-game steam overlay dll into your process. That dll is normally injected by steam when it launches games, but by calling this you cause it to always load, even when not launched via steam.
	if_true_fef(!SteamAPI_Init(), "%s", "SteamAPI_Init() failed\n");
	SteamClient()->SetWarningMessageHook( &SteamAPIDebugTextHook );	// set our debug handler
	if_true_fef(!SteamUser()->BLoggedOn(), "%s", "Steam user is not logged in\n");	// Ensure that the user has logged into Steam. This will always return true if the game is launched from Steam, but if Steam is at the login prompt when you run your game from the debugger, it will return false.

	const sc_t *pchServerAddress = 0, *pchLobbyID = 0;
	if ( !ParseCommandLine( app.Framework.RuntimeValues.PlatformDetail.EntryPointArgsWin.lpCmdLine, &pchServerAddress, &pchLobbyID ) ) {
		// no connect string on process command line. If app was launched via a Steam URL, the extra command line parameters in that URL get be retrieved with GetLaunchCommandLine. 
		// This way an attacker can't put malicious parameters in the process command line which might allow much more functionality then indented.
		sc_t szCommandLine[1024] = {};
		if ( SteamApps()->GetLaunchCommandLine( szCommandLine, sizeof( szCommandLine ) ) > 0 )
			ParseCommandLine( szCommandLine, &pchServerAddress, &pchLobbyID );
	}

	Steamworks_SelfCheck();	// do a DRM self check
	if_true_fef(!SteamInput()->Init( false ), "%s", "SteamInput()->Init failed.\n" );

	sc_t rgchCWD[1024] = {};
	if (!_getcwd(rgchCWD, sizeof(rgchCWD)))
		strcpy_s( rgchCWD, "." );

	sc_t rgchFullPath[1024] = {};
#if defined(LLC_WINDOWS)
	sprintf_s(rgchFullPath, "%s/%s", rgchCWD, "steam_input_manifest.vdf" );
#elif defined(LLC_OSX)
	_snprintf( rgchFullPath, sizeof( rgchFullPath ), "%s/steamworksexample.app/Contents/Resources/%s", rgchCWD, "steam_input_manifest.vdf" );	// hack for now, because we do not have utility functions available for finding the resource path alternatively we could disable the SteamController init on OS X
#else
	_snprintf( rgchFullPath, sizeof( rgchFullPath ), "%s/%s", rgchCWD, "steam_input_manifest.vdf" );
#endif
	SteamInput()->SetInputActionManifestFilePath( rgchFullPath );

#endif // USE_STEAM_CLIENT

	//llc_necall(::llc::tcpipInitialize(), "Failed to initialize network subsystem: '%s'.", "Unknown error");

	::llc::SFramework			& framework			= app.Framework;
	::llc::SWindow				& mainWindow		= framework.RootWindow;

	mainWindow.Size			= {1280, 720};

	if_true_e(::llc::failed(::llc::mainWindowCreate(mainWindow, framework.RuntimeValues.PlatformDetail, mainWindow.Input)));
	llc_necs(mainWindow.EventQueue.for_each([&app](const ::llc::pobj<::llc::SEventSystem> & sysEvent) { return ::processSystemEvent(app, *sysEvent); }));

	srand((uint32_t)time(0));

	::ghg::SGalaxyHellDrawCache	& drawCache			= app.GalaxyHellApp.Game.DrawCache;
	::llc::resize(4096 * 1024, drawCache.PixelCoords, drawCache.PixelVertexWeights, drawCache.LightColorsModel, drawCache.LightColorsWorld, drawCache.LightPointsModel, drawCache.LightPointsWorld);

	app.AudioState.InitAudio();
	app.AudioState.PrepareAudio("./../llc_games_data/sounds/thrust.wav");

	//app.Client.AddressConnect	= {};
	//::llc::tcpipAddress(9898, 0, ::llc::TRANSPORT_PROTOCOL_UDP, app.Client.AddressConnect);	// If loading the remote IP from the json fails, we fall back to the local address.
	//app.Client.AddressConnect.IP[0] = 192;
	//app.Client.AddressConnect.IP[1] = 168;
	//app.Client.AddressConnect.IP[2] = 0;
	//app.Client.AddressConnect.IP[3] = 188;
	//::llc::clientConnect(app.Client);

	//joySetThreshold(JOYSTICKID1, 1);
	//joySetThreshold(JOYSTICKID2, 1);
	//joySetCapture(framework.RootWindow.PlatformDetail.WindowHandle, JOYSTICKID1, 10, 0);
	//joySetCapture(framework.RootWindow.PlatformDetail.WindowHandle, JOYSTICKID2, 10, 0);

	framework.RootWindow.Input->JoystickCurrent.resize(4, {});
	//for(uint32_t i = 0; i < 4; ++i) 
	//	app.GamePad.Resume();


	const HRESULT result = ::CoInitializeEx(nullptr, COINIT_MULTITHREADED);
	if(result != RPC_E_CHANGED_MODE) {
		if_true_fef(FAILED(result), "CoInitializeEx failed: 0x%08X.", (::llc::u2_t)result);
		app.COMInitialized = true;
	}
	llc_necs(app.D3D->Initialize(mainWindow.PlatformDetail.WindowHandle, app.GalaxyHellApp.Game.ShipState.Engine.Scene->Graphics));
	llc_necs(app.D3D->CreateDeviceResources(*app.GalaxyHellApp.Game.ShipState.Engine.Scene->Graphics));
	llc_necs(app.D3D->SetWindowSize(mainWindow.Size));

	return 0;
}

::llc::error_t			update				(SApplication & app, bool exitSignal)	{
	if_true_vi(::llc::APPLICATION_STATE_EXIT, exitSignal);

	::llc::SFramework			& framework			= app.Framework;
	::llc::SWindow				& mainWindow		= framework.RootWindow;
	//::llc::STimer				timer;

	llc_necs(mainWindow.EventQueue.for_each([&app](const ::llc::pobj<::llc::SEventSystem> & sysEvent) { return ::processSystemEvent(app, *sysEvent); }));

	if_true_vi(::llc::APPLICATION_STATE_EXIT, 1 == ::ghg::galaxyHellUpdate(app.GalaxyHellApp, framework.FrameInfo.Seconds.LastFrame, mainWindow.Input, mainWindow.EventQueue));

	::llc::pobj<::ghg::TRenderTarget> frame;
	{
		::std::lock_guard			lockRTQueue	(app.GalaxyHellApp.RenderTargetLockQueue);
		if(app.GalaxyHellApp.RenderTargetQueue.size()) {
			::std::lock_guard			lockRTPool	(app.GalaxyHellApp.RenderTargetLockPool);

			frame				= app.GalaxyHellApp.RenderTargetQueue[app.GalaxyHellApp.RenderTargetQueue.size() - 1];
			app.GalaxyHellApp.RenderTargetQueue.pop_back();
			for(uint32_t iRT = 0; iRT < app.GalaxyHellApp.RenderTargetQueue.size(); ++iRT) {
				app.GalaxyHellApp.RenderTargetPool.push_back(app.GalaxyHellApp.RenderTargetQueue[iRT]);
			}
			app.GalaxyHellApp.RenderTargetQueue.clear();
		}
	}
	if(frame) {
		const ::llc::error_t result = ::presentFrame(app, *frame);
		{
			::std::lock_guard lockRTPool(app.GalaxyHellApp.RenderTargetLockPool);
			llc_necs(app.GalaxyHellApp.RenderTargetPool.push_back(frame));
		}
		llc_necs(result);
	}
	app.AudioState.vListenerPos			= app.GalaxyHellApp.Game.ShipState.Scene.Global.Camera[app.GalaxyHellApp.Game.ShipState.Scene.Global.CameraMode].Position; 
	app.AudioState.listener.OrientTop	= {0, 1, 0}; 
	app.AudioState.listener.OrientFront	= {1, 0, 0};  
	
	if(app.GalaxyHellApp.Game.ShipState.SpaceshipManager.ShipCores.size())
		app.AudioState.vEmitterPos			= app.GalaxyHellApp.Game.ShipState.GetShipPosition(0);
	
	app.AudioState.vEmitterPos 			*= .03f;
	app.AudioState.vListenerPos			*= .03f;
	if(::ghg::CAMERA_MODE_SKY == app.GalaxyHellApp.Game.ShipState.Scene.Global.CameraMode) {
		app.AudioState.vEmitterPos .x		=  0;
		app.AudioState.vListenerPos.x		=  0;
	}

	app.AudioState.emitter.DopplerScaler	= 6.0f; //float(app.GalaxyHellApp.World.PlayState.TimeScale);
	app.AudioState.FrequencyRatio			= (float)app.GalaxyHellApp.Game.PlayState.SimulatedTime.Scale;

	if(app.GalaxyHellApp.ActiveState == ::ghg::APP_STATE_Play && false == app.GalaxyHellApp.Game.PlayState.GlobalState.Paused) {
		app.AudioState.PauseAudio(true);
		//app.GamePad.Resume();
	}
	else {
		app.AudioState.PauseAudio(false);
		//app.GamePad.Suspend();
	}

	app.AudioState.UpdateAudio(framework.FrameInfo.Seconds.LastFrame);// / (app.GalaxyHellApp.World.ShipState.Ships.size() - 1));
#if defined(USE_STEAM_CLIENT)
	SteamAPI_RunCallbacks();
#endif

	if_true_vi(::llc::APPLICATION_STATE_EXIT, ::llc::APPLICATION_STATE_EXIT == ::llc::updateFramework(app.Framework));

	//for(uint32_t i = 0; i < 4; ++i) {
	//	DirectX::GamePad::State padState = app.GamePad.GetState(i);
	//	if(false == padState.IsConnected())
	//		warning_printf("Pad %i not connected", i);
	//	if(padState.IsLeftThumbStickDown	()) { 
	//		framework.RootWindow.Input->JoystickCurrent[i].Deltas.y = -1; }
	//	if(padState.IsLeftThumbStickLeft	()) { 
	//		framework.RootWindow.Input->JoystickCurrent[i].Deltas.x = -1; }
	//	if(padState.IsLeftThumbStickUp		()) { 
	//		framework.RootWindow.Input->JoystickCurrent[i].Deltas.y = +1; }
	//	if(padState.IsLeftThumbStickRight	()) { 
	//		framework.RootWindow.Input->JoystickCurrent[i].Deltas.x = +1; }
	//}
	
	//const GLubyte *	glGetString(GL_EXTENSIONS);
	return 0;
}

::llc::error_t			draw					(SApplication & app) {
    /* rotate a triangle around */
    //glClear(GL_COLOR_BUFFER_BIT);
    //glBegin(GL_TRIANGLES);
    //glColor3f(1.0f, 0.0f, 0.0f);
    //glVertex2i(0,  1);
    //glColor3f(0.0f, 1.0f, 0.0f);
    //glVertex2i(-1, -1);
    //glColor3f(0.0f, 0.0f, 1.0f);
    //glVertex2i(1, -1);
    //glEnd();
    //glFlush();
	return ::ghg::galaxyHellDraw(app.GalaxyHellApp, app.Framework.RootWindow.Size.u1());
}


//
