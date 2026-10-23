#include "llc_xaudio2.h"

#include "llc_galaxy_hell_app.h"

#include "llc_framework.h"
#include "llc_engine_gui_d3d.h"

//#include "llc_udp_client_app.h"

#ifndef CED_DEMO_08_H_298837492837
#define CED_DEMO_08_H_298837492837

struct SApplication {
	::llc::SFramework		Framework;

	::std::mutex			LockGUI;
	::std::mutex			LockRender;

	::llc::AUDIO_STATE		AudioState;

	::ghg::SGalaxyHellApp	GalaxyHellApp;

	::llc::pobj<::llc::SD3DApplication> D3D;
	::llc::SD3DRenderTarget	ShipTarget;
	::llc::SD3DGUIMeshResources GUIMesh;
	::llc::SD3DGUIResources<::llc::bgra> ShipLabel;
	::llc::an2u1_t			LabelPixels;
	bool					COMInitialized	= false;
 
	//::llc::SClient		Client;

							SApplication		(::llc::SRuntimeValues & runtimeValues)	: Framework(runtimeValues)		{ GalaxyHellApp.Game.ShipState.Engine.Scene->Graphics.create(); }
};

#endif // CED_DEMO_08_H_298837492837
