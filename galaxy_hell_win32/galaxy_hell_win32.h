#include "llc_xaudio2.h"

#include "llc_galaxy_hell_app.h"

#include "llc_framework.h"
#include "llc_engine_d3d_app.h"

//#include "llc_udp_client_app.h"

#ifndef CED_DEMO_08_H_298837492837
#define CED_DEMO_08_H_298837492837

struct SApplication {
	::llc::SFramework		Framework;
	::llc::SFrameMeter		DebugFrames;
	::llc::cid_t			DebugHUD		= -1;
	::llc::sc_t			DebugHUDText[128] = {};
	::llc::cid_t			DebugPanel		= -1, DebugHelp = -1, DebugStatus = -1;
	::llc::acid			DebugControls;
	::llc::sc_t			DebugStatusText[96] = {};

	::std::mutex			LockGUI;
	::std::mutex			LockRender;

	::llc::AUDIO_STATE		AudioState;

	::ghg::SGalaxyHellApp	GalaxyHellApp;

	::llc::pobj<::llc::SD3DApplication> D3D;
	::llc::SD3DRenderTarget	ShipTarget;
	::llc::pcom<ID3D11GeometryShader> FragmentClip;
	::llc::SD3DGUIResources<::llc::bgra> ShipLabel;
	::llc::an2u1_t			LabelPixels;
	bool					COMInitialized	= false;
 
	//::llc::SClient		Client;

							SApplication		(::llc::SRuntimeValues & runtimeValues)	: Framework(runtimeValues)		{ GalaxyHellApp.Game.ShipState.Engine.Scene->Graphics.create(); }
};

#endif // CED_DEMO_08_H_298837492837
