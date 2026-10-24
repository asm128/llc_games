#include "llc_app_impl.h"
#include "llc_font.h"
#include "llc_framework.h"
#include "llc_grid_color.h"
#include "llc_system_key.h"

stct SApplication {
	::llc::SFramework	Framework;
	::llc::cid_t		Workspace = ::llc::CID_INVALID;

	SApplication(::llc::SRuntimeValues & runtimeValues) : Framework(runtimeValues, {}) {}
};

::llc::err_t setup(SApplication & app) {
	::llc::SWindow & window = app.Framework.RootWindow;
	window.Size = {1280, 800};
	if_null_ve(::llc::OS_NO_MEMORY, window.BackBuffer.create());
	if_fail_fe(window.BackBuffer->resize(window.Size.u2()));
	if_fail_fe(::llc::mainWindowCreate(window, app.Framework.RuntimeValues.PlatformDetail, window.Input));

	::llc::SGUI & gui = *app.Framework.GUI;
	::llc::SRasterFontManager fonts = {};
	if_fail_fe(::llc::rasterFontDefaults(fonts));
	gui.Fonts = fonts.Fonts;
	gui.SelectedFont = 1;
	if_fail_fe(app.Workspace = ::llc::controlCreate(gui));
	::llc::SControlPlacement & placement = gui.Controls.Placement[app.Workspace];
	placement.Area = {{0, 0}, {420, 64}};
	placement.Align = ::llc::ALIGN_CENTER;
	placement.Margin = {2, 2, 2, 2};
	placement.Border = {2, 2, 2, 2};
	gui.Controls.Text[app.Workspace].Align = ::llc::ALIGN_CENTER;
	gui.Controls.Text[app.Workspace].FontSelected = 1;
	if_fail_fe(::llc::controlTextSet(gui, app.Workspace, "No document loaded"));
	rtrn ::llc::guiUpdateMetrics(gui, window.Size, true);
}

::llc::err_t cleanup(SApplication & app) {
	rtrn ::llc::mainWindowDestroy(app.Framework.RootWindow);
}

::llc::err_t update(SApplication & app, bool systemRequestedExit) {
	if(systemRequestedExit)
		rtrn ::llc::APPLICATION_STATE_EXIT;
	cnst ::llc::err_t result = ::llc::updateFramework(app.Framework);
	if(result)
		rtrn result;
	::llc::SWindow & window = app.Framework.RootWindow;
	if(window.Closed || window.Input->KeyDown(::llc::SYSTEM_KEY_Escape))
		rtrn ::llc::APPLICATION_STATE_EXIT;
	::llc::sleep(1);
	rtrn ::llc::guiUpdateMetrics(*app.Framework.GUI, window.Size, false);
}

::llc::err_t draw(SApplication & app) {
	::llc::SWindow & window = app.Framework.RootWindow;
	if(window.NoDraw)
		rtrn 0;
	if_fail_fe(window.BackBuffer->resize(window.Size.u2()));
	::llc::g8bgra target = window.BackBuffer->Color.View;
	if_fail_fe(target.fill(::llc::SColorBGRA{32, 30, 28}));
	rtrn ::llc::guiDraw(*app.Framework.GUI, target);
}

LLC_DEFINE_APPLICATION_ENTRY_POINT(SApplication, "LLC Games Editor")
