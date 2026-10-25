#include "galaxy_hell_win32.h"

#include "llc_png.h"
#include "llc_grid_copy.h"
#include "llc_raster_lh.h"
#include "llc_system_key.h"
#include "llc_mutex.h"

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

static ::llc::err_t drawShipLabelGPU(SApplication & app, const ::ghg::SGalaxyHell & game, ::llc::u2_t iShip, const ::llc::m4f2_t & matrixVP, const ::llc::n2u2_t & targetMetrics) {
	llc_necs(::ghg::getShipLabelCoords(game, iShip, matrixVP, targetMetrics, *game.DecoState.FontManager.Fonts[10], app.LabelPixels));
	if(0 == app.LabelPixels.size())
		return 0;
	::llc::n2u1_t first = app.LabelPixels[0], last = first;
	for(const ::llc::n2u1_t & pixel : app.LabelPixels) {
		first.x = ::llc::min(first.x, pixel.x); first.y = ::llc::min(first.y, pixel.y);
		last .x = ::llc::max(last .x, pixel.x); last .y = ::llc::max(last .y, pixel.y);
	}
	auto & resources = *app.D3D->DeviceResources;
	auto & label = app.ShipLabel;
	::llc::pcom<ID3D11Device> owner;
	if(label.VertexBuffer) label.VertexBuffer->GetDevice(&owner);
	if(owner.get() != resources.GetD3DDevice()) {
		label.ReleaseDeviceResources();
		llc_necs(label.CreateDeviceResources(resources.GetD3DDevice()));
		D3D11_BLEND_DESC blend = {};
		label.BlendState->GetDesc(&blend);
		blend.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
		::llc::pcom<ID3D11BlendState> blendState;
		llc_hrcall(resources.GetD3DDevice()->CreateBlendState(&blend, &blendState));
		label.BlendState = blendState;
	}
	const ::llc::n2u2_t size = {::llc::max((::llc::u2_t)(last.x - first.x + 1), label.RenderTarget.metrics().x), ::llc::max((::llc::u2_t)(last.y - first.y + 1), label.RenderTarget.metrics().y)};
	if(!label.Texture2D || label.RenderTarget.metrics() != size)
		llc_necs(label.CreateSizeDependentResources(resources.GetD3DDevice(), size.u1()));
	memset(label.RenderTarget.begin(), 0, label.RenderTarget.byte_count());
	::llc::bgra color = game.Pilots[iShip].Color;
	color.a = game.ShipState.SpaceshipManager.ShipCores[iShip].Health > 0 ? 255 : 51;
	for(const ::llc::n2u1_t & pixel : app.LabelPixels)
		label.RenderTarget[pixel.y - first.y][pixel.x - first.x] = color;
	auto context = resources.GetD3DDeviceContext();
	const D3D11_VIEWPORT viewport = {(float)first.x, (float)first.y, (float)size.x, (float)size.y, 0, 1};
	context->RSSetViewports(1, &viewport);
	const ::llc::err_t result = ::llc::d3dGUIDraw(resources, label);
	const D3D11_VIEWPORT fullViewport = {0, 0, (float)targetMetrics.x, (float)targetMetrics.y, 0, 1};
	context->RSSetViewports(1, &fullViewport);
	return result;
}

static ::llc::err_t drawExplosionGPU(::llc::Sample3DSceneRenderer & renderer, const ::ghg::SGalaxyHell & game, const ::ghg::SExplosion & explosion) {
	if(0 == explosion.Slices.size()) return 0;
	const ::llc::SEngineGraphics & graphics = *game.ShipState.Engine.Scene->Graphics;
	if_true_fe((::llc::u2_t)explosion.IndexMesh >= graphics.Meshes.size() || (::llc::u2_t)explosion.IndexImage >= graphics.Skins.size());
	if_true_fe(explosion.Slices.size() != explosion.Particles.Position.size());
	const ::llc::SGeometryMesh & mesh = *graphics.Meshes[explosion.IndexMesh];
	const ::llc::SSkin & skin = *graphics.Skins[explosion.IndexImage];
	if_true_fe(0 == mesh.GeometryBuffers.size());
	const auto & indices = *graphics.Buffers[mesh.GeometryBuffers[0]];
	const ::llc::u2_t indexCount = indices.Data.size() / indices.Desc.Format.TotalBytes();
	::llc::s2_t shader;
	llc_necs(shader = graphics.Shaders.Names.find("psGalaxyHellFragment"));
	for(::llc::u2_t iPart = 0; iPart < explosion.Particles.Position.size(); ++iPart) {
		const ::llc::rangeu2_t slice = {explosion.Slices[iPart].Offset * 3U, explosion.Slices[iPart].Count * 3U};
		if_true_fe(slice.Offset > indexCount || slice.Count > indexCount - slice.Offset);
		if(0 == slice.Count) continue;
		::llc::SRenderNodeConstants constants;
		constants.Model.Identity();
		if(iPart % 5) constants.Model.RotationY(game.DecoState.AnimationTime * 2);
		if(iPart % 3) constants.Model.RotationX(game.DecoState.AnimationTime * 2);
		if(iPart % 2) constants.Model.RotationZ(game.DecoState.AnimationTime * 2);
		constants.Model.SetTranslation(explosion.Particles.Position[iPart], false);
		llc_necs(::llc::lightFilter(constants.Model.GetTranslation(), renderer.RenderCache.LightsWorld, renderer.RenderCache.LightsNode));
		constants.MVP = (constants.Model * renderer.ConstantBufferScene.View * renderer.ConstantBufferScene.Perspective).GetTranspose();
		constants.Model = constants.Model.GetTranspose();
		llc_necs(renderer.Render(explosion.IndexMesh, slice, skin.Textures, shader, false, constants, renderer.RenderCache.LightsNode, 1, ::llc::CULL_MODE_NONE));
	}
	return 0;
}

static ::llc::err_t drawExplosionsGPU(SApplication & app, const ::ghg::SGalaxyHell & game) {
	if(0 == game.DecoState.Explosions.size()) return 0;
	auto device = app.D3D->DeviceResources->GetD3DDevice();
	::llc::pcom<ID3D11Device> owner;
	if(app.FragmentClip) app.FragmentClip->GetDevice(&owner);
	if(owner.get() != device) {
		::llc::au0_t code;
		llc_necs(::llc::fileToMemory("gsGalaxyHellFragment.cso", code));
		::llc::pcom<ID3D11GeometryShader> shader;
		llc_hrcall(device->CreateGeometryShader(code.begin(), code.size(), nullptr, &shader));
		app.FragmentClip = shader;
	}
	auto context = app.D3D->DeviceResources->GetD3DDeviceContext();
	context->GSSetShader(app.FragmentClip, nullptr, 0);
	::llc::err_t result = 0;
	for(const ::ghg::SExplosion & explosion : game.DecoState.Explosions)
		if(::llc::failed(result = ::drawExplosionGPU(app.D3D->Scene, game, explosion))) break;
	context->GSSetShader(nullptr, nullptr, 0);
	return result;
}

static ::llc::err_t drawShipsGPU(SApplication & app, const ::ghg::SGalaxyHell & game, ::ghg::SGalaxyHellDrawCache & cache, const ::llc::m4f2_t & matrixVP) {
	::std::lock_guard lockRender(app.LockRender);
	::llc::SD3DApplication & d3d = *app.D3D;
	::llc::Sample3DSceneRenderer & renderer = d3d.Scene;
	const ::llc::SEngineScene & scene = *game.ShipState.Engine.Scene;
	const ::llc::SEngineGraphics & graphics = *scene.Graphics;
	if(renderer.VertexBuffer.size() != graphics.Meshes.size() || renderer.ShaderResourceView.size() != graphics.Surfaces.size() || renderer.PixelShader.size() != graphics.Shaders.size()) {
		renderer.IndexBuffer.clear(); renderer.VertexBuffer.clear();
		renderer.ShaderResourceView.clear(); renderer.PixelShader.clear(); renderer.LightSlots.clear();
		llc_necs(d3d.CreateDeviceDependentEngineResources(d3d.DeviceResources->GetD3DDevice(), graphics));
	}
	if(renderer.VertexShader.size() == 1)
		llc_necs(renderer.CreateVertexShader("vsGalaxyHell"));
	::llc::g8bgra colors = cache.RenderTarget->Color;
	::llc::gu2_t depth = cache.RenderTarget->DepthStencil;
	llc_necs(app.ShipTarget.Begin(*d3d.DeviceResources, colors));
	::llc::SEngineSceneConstants & constants = renderer.ConstantBufferScene;
	constants = {};
	constants.View.Identity();
	constants.Screen.ViewportLH(colors.metrics().u1());
	constants.Perspective = matrixVP * constants.Screen.GetInverse();
	constants.LightDirection = game.ShipState.Scene.Global.LightVector;
	constants.Time = (::llc::f2_t)game.DecoState.AnimationTime;
	llc_necs(scene.BuildLightList(renderer.RenderCache.LightsWorld));
	for(::llc::u2_t iLight = 0; iLight < cache.LightPointsWorld.size(); ++iLight) {
		::llc::SRenderLight light;
		light.Position = cache.LightPointsWorld[iLight];
		light.Color.Diffuse = cache.LightColorsWorld[iLight];
		light.Range = 10;
		llc_necs(renderer.RenderCache.LightsWorld.push_back(light));
	}
	const double pulse = fabsf(sinf((float)game.DecoState.AnimationTime * 3));
	for(::llc::u2_t iShip = 0; iShip < game.ShipState.SpaceshipManager.ShipCores.size(); ++iShip) {
		const ::llc::SSpaceshipCore & ship = game.ShipState.SpaceshipManager.ShipCores[iShip];
		if(ship.Health <= 0 && ship.Team)
			continue;
		const ::llc::rgbaf color = iShip < game.PlayState.Constants.Players ? ::llc::bgra(game.Pilots[iShip].Color) : ::llc::bgra(::llc::RED);
		const ::llc::rgbaf emission = pulse < .5 ? ::llc::rgbaf{} : color * (pulse * .5);
		llc_necs(::ghg::getShipRenderNodes(game, iShip, cache.ShipNodes));
		for(::llc::u2_t node : cache.ShipNodes) {
			::llc::SRenderMaterial material = graphics.Skins[scene.RenderNodes[node].Skin]->Material;
			material.Emission = {emission.r, emission.g, emission.b};
			llc_necs(::llc::d3dDrawEngineNode(renderer, scene, node, renderer.RenderCache.LightsWorld, &material, 1));
		}
		if(iShip < game.PlayState.Constants.Players && !ship.Team) {
			llc_necs(::drawShipLabelGPU(app, game, iShip, matrixVP, colors.metrics()));
		}
	}
	llc_necs(::drawExplosionsGPU(app, game));
	return app.ShipTarget.Read(*d3d.DeviceResources, colors, depth);
}

GDEFINE_ENUM_TYPE(DEBUG_CONTROL, ::llc::u0_t);
GDEFINE_ENUM_VALUED(DEBUG_CONTROL, Help,     0, "F1   Help");
GDEFINE_ENUM_VALUED(DEBUG_CONTROL, Panel,    1, "F2   Close controls");
GDEFINE_ENUM_VALUED(DEBUG_CONTROL, HUD,      2, "F3   Show / hide HUD");
GDEFINE_ENUM_VALUED(DEBUG_CONTROL, Renderer, 3, "F4   Switch CPU / D3D11");
GDEFINE_ENUM_VALUED(DEBUG_CONTROL, Pause,    4, "P    Pause / resume");
GDEFINE_ENUM_VALUED(DEBUG_CONTROL, Slower,   5, "F6   Slower");
GDEFINE_ENUM_VALUED(DEBUG_CONTROL, Faster,   6, "F7   Faster");
GDEFINE_ENUM_VALUED(DEBUG_CONTROL, Normal,   7, "F8   Normal speed (1x)");

static ::llc::err_t debugControl(SApplication & app, DEBUG_CONTROL action) {
	auto & gui = *app.Framework.GUI;
	auto & game = app.GalaxyHellApp.Game;
	switch(action) {
	case DEBUG_CONTROL_Help:
		gui.Controls.SetHidden(app.DebugHelp, !gui.Controls.IsHidden(app.DebugHelp));
		if(!gui.Controls.IsHidden(app.DebugHelp)) gui.Controls.SetHidden(app.DebugPanel, true);
		break;
	case DEBUG_CONTROL_Panel:
		gui.Controls.SetHidden(app.DebugPanel, !gui.Controls.IsHidden(app.DebugPanel));
		if(!gui.Controls.IsHidden(app.DebugPanel)) gui.Controls.SetHidden(app.DebugHelp, true);
		break;
	case DEBUG_CONTROL_HUD:
		gui.Controls.SetHidden(app.DebugHUD, !gui.Controls.IsHidden(app.DebugHUD));
		break;
	case DEBUG_CONTROL_Renderer: return ::ghg::galaxyHellToggleRenderer(app.GalaxyHellApp);
	case DEBUG_CONTROL_Pause: {
		if(app.GalaxyHellApp.ActiveState != ::ghg::APP_STATE_Play) break;
		::llc::lock_guard lock(game.LockUpdate);
		game.PlayState.GlobalState.Paused = !game.PlayState.GlobalState.Paused;
		break;
	}
	case DEBUG_CONTROL_Slower:
	case DEBUG_CONTROL_Faster:
	case DEBUG_CONTROL_Normal: {
		::llc::lock_guard lock(game.LockUpdate);
		game.CustomTimeScale = action == DEBUG_CONTROL_Normal ? 1
			: ::llc::clamped(game.CustomTimeScale * (action == DEBUG_CONTROL_Slower ? .5 : 2), .25, 4.0);
		break;
	}
	default: return -1;
	}
	return 0;
}

static ::llc::err_t updateDebugControls(SApplication & app) {
	const ::llc::u0_t keys[] = {::llc::SYSTEM_KEY_F1, ::llc::SYSTEM_KEY_F2, ::llc::SYSTEM_KEY_F3, ::llc::SYSTEM_KEY_F4, 'P', ::llc::SYSTEM_KEY_F6, ::llc::SYSTEM_KEY_F7, ::llc::SYSTEM_KEY_F8};
	auto & gui = *app.Framework.GUI;
	const auto & input = *app.Framework.RootWindow.Input;
	const bool visible = !gui.Controls.IsHidden(app.DebugPanel);
	for(::llc::u2_t i = 0; i < app.DebugControls.size(); ++i)
		if((input.KeyDown(keys[i]) && !input.KeyboardCurrent.KeyState[::llc::SYSTEM_KEY_Menu] && !input.KeyboardCurrent.KeyState[::llc::SYSTEM_KEY_Control])
			|| (visible && gui.Controls.IsAction(app.DebugControls[i])))
			llc_necs(::debugControl(app, (DEBUG_CONTROL)i));
	return 0;
}

static ::llc::err_t setupDebugControls(SApplication & app) {
	auto & gui = *app.Framework.GUI;
	llc_necs(app.DebugPanel = ::llc::controlCreate(gui));
	gui.Controls.Placement[app.DebugPanel].Area = {{12, 44}, {376, 250}};
	gui.Controls.Placement[app.DebugPanel].Align = ::llc::ALIGN_TOP_LEFT;
	llc_necs(app.DebugStatus = ::llc::controlCreateChild(gui, app.DebugPanel));
	gui.Controls.Placement[app.DebugStatus].Area = {{8, 8}, {352, 24}};
	gui.Controls.Text[app.DebugStatus].Align = ::llc::ALIGN_CENTER_LEFT;
	gui.Controls.Draw[app.DebugStatus].NoBorder = gui.Controls.Draw[app.DebugStatus].NoClient = true;
	gui.Controls.States[app.DebugStatus].Mask = gui.Controls.States[app.DebugHUD].Mask;
	llc_necs(::llc::guiCreateControlList<DEBUG_CONTROL>(gui, app.DebugPanel, {352, 24}, {8, 36}, ::llc::ALIGN_TOP_LEFT, ::llc::ALIGN_CENTER_LEFT, app.DebugControls));
	llc_necs(app.DebugHelp = ::llc::controlCreate(gui));
	gui.Controls.Placement[app.DebugHelp].Area = {{12, 44}, {376, 310}};
	gui.Controls.Placement[app.DebugHelp].Align = ::llc::ALIGN_TOP_LEFT;
	const ::llc::vcst_t help[] = {
		"GALAXY HELL / HELP", "F1   Close help", "F2   Controls", "F3   Show / hide HUD", "F4   Switch CPU / D3D11",
		"P    Pause / resume during play", "F6 / F7   Slower / faster", "F8   Restore 1x speed", "F5 / F9   Save / load reserved",
		"WASD / arrows   Move", "Shift / Ctrl   Turbo", "Escape   Menu / continue"
	};
	::llc::acid lines;
	llc_necs(::llc::guiCreateControlList(gui, help, app.DebugHelp, {352, 24}, {8, 8}, ::llc::ALIGN_TOP_LEFT, ::llc::ALIGN_CENTER_LEFT, lines));
	for(const auto id : lines) {
		gui.Controls.Draw[id].NoBorder = gui.Controls.Draw[id].NoClient = true;
		gui.Controls.States[id].Mask = gui.Controls.States[app.DebugHUD].Mask;
	}
	gui.Controls.SetHidden(app.DebugPanel, true);
	gui.Controls.SetHidden(app.DebugHelp, true);
	return 0;
}

static ::llc::err_t updateDebugHUD(SApplication & app) {
	auto & framework = app.Framework;
	if(framework.Timer.LastTimeMicroseconds)
		app.DebugFrames.Frame(framework.Timer.LastTimeMicroseconds);
	const auto & cache = app.GalaxyHellApp.Game.DrawCache;
	const double milliseconds = 1000 * (app.DebugFrames.FramesPerSecond ? app.DebugFrames.AverageFrameTime : framework.Timer.LastTimeSeconds);
	const int count = snprintf(app.DebugHUDText, sizeof(app.DebugHUDText), "%s | %3u FPS | %6.2f ms avg | %ux%u | F3", cache.UseGPU && cache.DrawShips ? "D3D11" : "CPU", app.DebugFrames.FramesPerSecond, milliseconds, (unsigned)framework.RootWindow.Size.x, (unsigned)framework.RootWindow.Size.y);
	if_true_fe(count < 0 || (::llc::u2_t)count >= sizeof(app.DebugHUDText));
	llc_necs(::llc::controlTextSet(*framework.GUI, app.DebugHUD, {app.DebugHUDText, (::llc::u2_t)count}));
	// The view keeps the same storage when its text changes length or digits.
	framework.GUI->Controls.SetUpdated(app.DebugHUD, false);
	auto & gui = *framework.GUI;
	if(!gui.Controls.IsHidden(app.DebugPanel)) {
		const auto & game = app.GalaxyHellApp.Game;
		const bool paused = game.PlayState.GlobalState.Paused;
		const bool playing = app.GalaxyHellApp.ActiveState == ::ghg::APP_STATE_Play;
		const auto length = ::llc::sprintf_s(app.DebugStatusText, "%s | %.2fx | %s", cache.UseGPU && cache.DrawShips ? "D3D11" : "CPU", game.CustomTimeScale, playing ? (paused ? "PAUSED" : "RUNNING") : "MENU");
		llc_necs(length);
		llc_necs(::llc::controlTextSet(gui, app.DebugStatus, {app.DebugStatusText, (::llc::u2_t)length}));
		gui.Controls.SetUpdated(app.DebugStatus, false);
		gui.Controls.SetSelected(app.DebugControls[DEBUG_CONTROL_HUD], !gui.Controls.IsHidden(app.DebugHUD));
		gui.Controls.SetSelected(app.DebugControls[DEBUG_CONTROL_Renderer], cache.UseGPU && cache.DrawShips);
		gui.Controls.SetSelected(app.DebugControls[DEBUG_CONTROL_Pause], paused);
		gui.Controls.SetSelected(app.DebugControls[DEBUG_CONTROL_Normal], game.CustomTimeScale == 1);
		gui.Controls.SetDisabled(app.DebugControls[DEBUG_CONTROL_Pause], !playing);
		gui.Controls.SetDisabled(app.DebugControls[DEBUG_CONTROL_Slower], game.CustomTimeScale <= .25);
		gui.Controls.SetDisabled(app.DebugControls[DEBUG_CONTROL_Faster], game.CustomTimeScale >= 4);
	}
	return 0;
}

static ::llc::error_t presentFrame(SApplication & app, const ::ghg::TRenderTarget & frame, bool drawGUI) {
	auto & game = app.GalaxyHellApp;
	::std::lock_guard lockUpdate(game.Game.LockUpdate);
	::std::lock_guard lockRender(app.LockRender);
	const ::llc::SWindow & window = app.Framework.RootWindow;
	if(window.NoDraw || 0 == window.Size.x || 0 == window.Size.y || frame.Color.metrics() != window.Size.u2())
		return 0;
	::llc::SD3DApplication & d3d = *app.D3D;
	if(d3d.GUIStuff.RenderTarget.metrics() != window.Size.u2())
		llc_necs(d3d.SetWindowSize(window.Size));
	memcpy(d3d.GUIStuff.RenderTarget.begin(), frame.Color.begin(), frame.Color.byte_count());
	if(!drawGUI)
		llc_necs(::llc::guiDraw(*app.Framework.GUI, d3d.GUIStuff.RenderTarget.View));
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
	if(drawGUI) {
		llc_necs(::llc::d3dGUIDraw(resources, d3d.GUIStuff, *game.DialogPerState[game.ActiveState].GUI, window.Size));
		if(game.ActiveState != ::ghg::APP_STATE_Play)
			llc_necs(::llc::d3dGUIDraw(resources, d3d.GUIStuff, *game.DialogDesktop.GUI, window.Size));
		for(auto & player : game.UIPlay.PlayerUI)
			llc_necs(::llc::d3dGUIDraw(resources, d3d.GUIStuff, game.ActiveState == ::ghg::APP_STATE_Play ? *player.DialogPlay.GUI : *player.DialogHome.GUI, window.Size));
		llc_necs(::llc::d3dGUIDraw(resources, d3d.GUIStuff, *app.Framework.GUI, window.Size));
	}
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
		app.GalaxyHellApp.Game.DrawCache.DrawShips = {};
		app.ShipLabel = {};
		app.ShipTarget = {};
		app.FragmentClip = {};
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
	drawCache.DrawShips = [&app](const ::ghg::SGalaxyHell & game, ::ghg::SGalaxyHellDrawCache & cache, const ::llc::m4f2_t & matrixVP) { return ::drawShipsGPU(app, game, cache, matrixVP); };
	::llc::SGUI & debugGUI = *framework.GUI;
	llc_necs(app.DebugHUD = ::llc::controlCreate(debugGUI));
	debugGUI.Controls.Placement[app.DebugHUD].Area = {{0, 8}, {520, 24}};
	debugGUI.Controls.Placement[app.DebugHUD].Align = ::llc::ALIGN_TOP | ::llc::ALIGN_HCENTER;
	debugGUI.Controls.Text[app.DebugHUD].Align = ::llc::ALIGN_CENTER;
	debugGUI.Controls.Draw[app.DebugHUD].NoBorder = true;
	debugGUI.Controls.Draw[app.DebugHUD].ColorMode = ::llc::GUI_COLOR_MODE_Flat;
	debugGUI.ThemeDefault = ::llc::ASCII_COLOR_DARKGREY * 16 + ::llc::ASCII_COLOR_WHITE;
	debugGUI.Controls.States[app.DebugHUD].Mask &= ~(::llc::GUI_CONTROL_FLAG_Hovered | ::llc::GUI_CONTROL_FLAG_Pressed | ::llc::GUI_CONTROL_FLAG_Selected | ::llc::GUI_CONTROL_FLAG_Focused | ::llc::GUI_CONTROL_FLAG_Action);
	llc_necs(::setupDebugControls(app));
	llc_necs(::updateDebugHUD(app));
	llc_necs(::llc::guiUpdateMetrics(debugGUI, mainWindow.Size, false));

	return 0;
}

::llc::error_t			update				(SApplication & app, bool exitSignal)	{
	if_true_vi(::llc::APPLICATION_STATE_EXIT, exitSignal);

	::llc::SFramework			& framework			= app.Framework;
	::llc::SWindow				& mainWindow		= framework.RootWindow;
	//::llc::STimer				timer;

	llc_necs(mainWindow.EventQueue.for_each([&app](const ::llc::pobj<::llc::SEventSystem> & sysEvent) { return ::processSystemEvent(app, *sysEvent); }));

	// The queued frame was drawn before this update can change the renderer setting.
	const bool frameUseGPU = app.GalaxyHellApp.Game.DrawCache.UseGPU && app.GalaxyHellApp.Game.DrawCache.DrawShips;
	llc_necs(::updateDebugControls(app));
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
		const ::llc::error_t result = ::presentFrame(app, *frame, frameUseGPU);
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

	llc_necs(::updateDebugHUD(app));
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
