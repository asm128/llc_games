#include "llc_framework.h"
#include "llc_gui.h"
#include "llc_grid_color.h"
#include "llc_img_color.h"
#include "llc_runtime.h"
#include "llc_runtime_module.h"
#include "llc_size.h"
#include "llc_system_key.h"

#ifndef LLC_GPFTW_PORT_H_23627
#define LLC_GPFTW_PORT_H_23627

#include <cstdio>

namespace gpftw_port {
	struct SHost {
		::llc::SFramework	Framework;
		::llc::cid_t		Title	= ::llc::CID_INVALID;
		::llc::cid_t		Status	= ::llc::CID_INVALID;
		::llc::cid_t		Help	= ::llc::CID_INVALID;
		char				StatusText[128] = {};

		SHost(::llc::SRuntimeValues & runtime) : Framework(runtime, {}) {}
	};

	inline ::llc::err_t controlCreate(SHost & host, ::llc::cid_t & id, ::llc::vcst_t text, ::llc::s1_t y, ::llc::s1_t width) {
		if_fail_fe(id = ::llc::controlCreate(*host.Framework.GUI));
		::llc::SControlPlacement & placement = host.Framework.GUI->Controls.Placement[id];
		placement.Area = {{0, y}, {width, 28}};
		placement.Align = ::llc::ALIGN_CENTER;
		placement.Margin = {1, 1, 1, 1};
		placement.Border = {1, 1, 1, 1};
		host.Framework.GUI->Controls.Text[id].Align = ::llc::ALIGN_CENTER;
		host.Framework.GUI->Controls.Text[id].FontSelected = 1;
		rtrn ::llc::controlTextSet(*host.Framework.GUI, id, text);
	}

	inline ::llc::err_t setup(SHost & host, ::llc::vcst_t title, ::llc::vcst_t help) {
		::llc::SWindow & window = host.Framework.RootWindow;
		window.Size = {800, 600};
		if_null_ve(::llc::OS_NO_MEMORY, window.BackBuffer.create());
		if_fail_fe(window.BackBuffer->resize(window.Size.u2()));
		if_fail_fe(::llc::mainWindowCreate(window, host.Framework.RuntimeValues.PlatformDetail, window.Input));
		::llc::SRasterFontManager fonts = {};
		if_fail_fe(::llc::rasterFontDefaults(fonts));
		host.Framework.GUI->Fonts = fonts.Fonts;
		host.Framework.GUI->SelectedFont = 1;
		if_fail_fe(controlCreate(host, host.Title, title, -284, 760));
		if_fail_fe(controlCreate(host, host.Status, "", -250, 760));
		if_fail_fe(controlCreate(host, host.Help, help, 272, 780));
		rtrn ::llc::guiUpdateMetrics(*host.Framework.GUI, window.Size, true);
	}

	inline ::llc::err_t statusSet(SHost & host, ::llc::vcst_t status) {
		if_fail_fe(::llc::controlTextSet(*host.Framework.GUI, host.Status, status));
		host.Framework.GUI->Controls.SetUpdated(host.Status, false);
		rtrn 0;
	}

	inline ::llc::err_t update(SHost & host) {
		cnst ::llc::err_t result = ::llc::updateFramework(host.Framework);
		if(result)
			rtrn result;
		if(host.Framework.RootWindow.Closed || host.Framework.RootWindow.Input->KeyDown(::llc::SYSTEM_KEY_Escape))
			rtrn ::llc::APPLICATION_STATE_EXIT;
		rtrn 0;
	}

	inline void rect(::llc::g8bgra target, ::llc::s2_t x, ::llc::s2_t y, ::llc::s2_t width, ::llc::s2_t height, ::llc::SColorBGRA color) nxpt {
		cnst ::llc::s2_t left = ::llc::max((::llc::s2_t)0, x);
		cnst ::llc::s2_t top = ::llc::max((::llc::s2_t)0, y);
		cnst ::llc::s2_t right = ::llc::min((::llc::s2_t)target.metrics().x, x + width);
		cnst ::llc::s2_t bottom = ::llc::min((::llc::s2_t)target.metrics().y, y + height);
		for(::llc::s2_t row = top; row < bottom; ++row)
		for(::llc::s2_t col = left; col < right; ++col)
			target[row][col] = color;
	}

	inline ::llc::err_t draw(SHost & host, ::llc::g8bgra target) {
		rtrn ::llc::guiDraw(*host.Framework.GUI, target);
	}
}

#endif // LLC_GPFTW_PORT_H_23627
