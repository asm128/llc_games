#include "llc_dialog.h"
#include "llc_dialog_controls.h"
#include "llc_png.h"

								::llc::IDialogControl::~IDialogControl		() { 
	if(-1 != IdGUIControl && DeleteControl && Dialog->GUI) 
		::llc::controlDelete(*Dialog->GUI, IdGUIControl, true); 
}

static	::llc::error_t	dialogInitializeColors			(::llc::SDialogColors & dialogColors, ::llc::apod<::llc::astatic<::llc::bgra, ::llc::UI_CONTROL_AREA_COUNT>> & palettes) {
	dialogColors.Control		[::llc::GUI_CONTROL_PALETTE_NORMAL	]			= (uint16_t)palettes.push_back({{::llc::DARKGRAY			, ::llc::DARKGRAY	, ::llc::DARKGRAY	, ::llc::DARKGRAY	, ::llc::DARKGRAY	, {}, ::llc::WHITE		* .85	,}});
	dialogColors.Control		[::llc::GUI_CONTROL_PALETTE_DISABLED]			= (uint16_t)palettes.push_back({{::llc::LIGHTGRAY * 1.3	, ::llc::ORANGE		, ::llc::YELLOW		, ::llc::MAGENTA	, ::llc::CYAN		, {}, ::llc::LIGHTGRAY	* 1.2	,}});
	dialogColors.Control		[::llc::GUI_CONTROL_PALETTE_HOVER	]			= (uint16_t)palettes.push_back({{::llc::GRAY	 			, ::llc::GRAY		, ::llc::GRAY		, ::llc::GRAY		, ::llc::GRAY		, {}, ::llc::WHITE				,}});
	dialogColors.Control		[::llc::GUI_CONTROL_PALETTE_PRESSED	]			= (uint16_t)palettes.push_back({{::llc::LIGHTGRAY			, ::llc::LIGHTBLUE	, ::llc::LIGHTBLUE	, ::llc::LIGHTBLUE	, ::llc::LIGHTBLUE	, {}, ::llc::WHITE				,}});
	dialogColors.Control		[::llc::GUI_CONTROL_PALETTE_ACTION	]			= (uint16_t)palettes.push_back({{::llc::BLUE				, ::llc::ORANGE		, ::llc::YELLOW		, ::llc::MAGENTA	, ::llc::CYAN		, {}, ::llc::WHITE				,}});
	dialogColors.Control		[::llc::GUI_CONTROL_PALETTE_OUTDATED]			= (uint16_t)palettes.push_back({{::llc::BLUE				, ::llc::ORANGE		, ::llc::YELLOW		, ::llc::MAGENTA	, ::llc::CYAN		, {}, ::llc::WHITE				,}});

	dialogColors.Button			[::llc::GUI_CONTROL_PALETTE_NORMAL	]			= (uint16_t)palettes.push_back({{::llc::GRAY				, ::llc::DARKGRAY	, ::llc::DARKGRAY	, ::llc::DARKGRAY	, ::llc::DARKGRAY	, {}, ::llc::WHITE				,}});
	dialogColors.Button			[::llc::GUI_CONTROL_PALETTE_DISABLED]			= (uint16_t)palettes.push_back({{::llc::LIGHTGRAY			, ::llc::ORANGE		, ::llc::YELLOW		, ::llc::MAGENTA	, ::llc::CYAN		, {}, ::llc::LIGHTGRAY * 1.1	,}});
	dialogColors.Button			[::llc::GUI_CONTROL_PALETTE_HOVER	]			= (uint16_t)palettes.push_back({{::llc::GRAY	 	* 1.1	, ::llc::GRAY		, ::llc::GRAY		, ::llc::GRAY		, ::llc::GRAY		, {}, ::llc::WHITE				,}});
	dialogColors.Button			[::llc::GUI_CONTROL_PALETTE_PRESSED	]			= (uint16_t)palettes.push_back({{::llc::LIGHTGRAY			, ::llc::ORANGE		, ::llc::ORANGE		, ::llc::ORANGE		, ::llc::ORANGE		, {}, ::llc::BLUE				,}});
	dialogColors.Button			[::llc::GUI_CONTROL_PALETTE_ACTION	]			= (uint16_t)palettes.push_back({{::llc::BLUE				, ::llc::ORANGE		, ::llc::YELLOW		, ::llc::MAGENTA	, ::llc::CYAN		, {}, ::llc::WHITE				,}});
	dialogColors.Button			[::llc::GUI_CONTROL_PALETTE_OUTDATED]			= (uint16_t)palettes.push_back({{::llc::BLUE				, ::llc::ORANGE		, ::llc::YELLOW		, ::llc::MAGENTA	, ::llc::CYAN		, {}, ::llc::WHITE				,}});

	dialogColors.CheckBox		[::llc::GUI_CONTROL_PALETTE_NORMAL	]			= (uint16_t)palettes.push_back({{::llc::WHITE		* 0.7	, ::llc::DARKGRAY	, ::llc::DARKGRAY	, ::llc::DARKGRAY	, ::llc::DARKGRAY	, {}, ::llc::WHITE				,}});
	dialogColors.CheckBox		[::llc::GUI_CONTROL_PALETTE_DISABLED]			= (uint16_t)palettes.push_back({{::llc::LIGHTGRAY			, ::llc::ORANGE		, ::llc::YELLOW		, ::llc::MAGENTA	, ::llc::CYAN		, {}, ::llc::LIGHTGRAY * 1.1	,}});
	dialogColors.CheckBox		[::llc::GUI_CONTROL_PALETTE_HOVER	]			= (uint16_t)palettes.push_back({{::llc::WHITE		* 0.85	, ::llc::GRAY		, ::llc::GRAY		, ::llc::GRAY		, ::llc::GRAY		, {}, ::llc::WHITE				,}});
	dialogColors.CheckBox		[::llc::GUI_CONTROL_PALETTE_PRESSED	]			= (uint16_t)palettes.push_back({{::llc::WHITE		 		, ::llc::BLUE		, ::llc::BLUE		, ::llc::BLUE		, ::llc::BLUE		, {}, ::llc::WHITE				,}});
	dialogColors.CheckBox		[::llc::GUI_CONTROL_PALETTE_ACTION	]			= (uint16_t)palettes.push_back({{::llc::BLUE				, ::llc::ORANGE		, ::llc::YELLOW		, ::llc::MAGENTA	, ::llc::CYAN		, {}, ::llc::WHITE				,}});
	dialogColors.CheckBox		[::llc::GUI_CONTROL_PALETTE_OUTDATED]			= (uint16_t)palettes.push_back({{::llc::BLUE				, ::llc::ORANGE		, ::llc::YELLOW		, ::llc::MAGENTA	, ::llc::CYAN		, {}, ::llc::WHITE				,}});

	dialogColors.Viewport		[::llc::GUI_CONTROL_PALETTE_NORMAL	]			= (uint16_t)palettes.push_back({{::llc::WHITE		* 0.7	, ::llc::DARKGRAY	, ::llc::DARKGRAY	, ::llc::DARKGRAY	, ::llc::DARKGRAY	, {}, ::llc::WHITE				,}});
	dialogColors.Viewport		[::llc::GUI_CONTROL_PALETTE_DISABLED]			= (uint16_t)palettes.push_back({{::llc::LIGHTGRAY			, ::llc::ORANGE		, ::llc::YELLOW		, ::llc::MAGENTA	, ::llc::CYAN		, {}, ::llc::LIGHTGRAY * 1.1	,}});
	dialogColors.Viewport		[::llc::GUI_CONTROL_PALETTE_HOVER	]			= (uint16_t)palettes.push_back({{::llc::WHITE		* 0.85	, ::llc::GRAY		, ::llc::GRAY		, ::llc::GRAY		, ::llc::GRAY		, {}, ::llc::WHITE				,}});
	dialogColors.Viewport		[::llc::GUI_CONTROL_PALETTE_PRESSED	]			= (uint16_t)palettes.push_back({{::llc::WHITE		 		, ::llc::BLUE		, ::llc::BLUE		, ::llc::BLUE		, ::llc::BLUE		, {}, ::llc::WHITE				,}});
	dialogColors.Viewport		[::llc::GUI_CONTROL_PALETTE_ACTION	]			= (uint16_t)palettes.push_back({{::llc::BLUE				, ::llc::ORANGE		, ::llc::YELLOW		, ::llc::MAGENTA	, ::llc::CYAN		, {}, ::llc::WHITE				,}});
	dialogColors.Viewport		[::llc::GUI_CONTROL_PALETTE_OUTDATED]			= (uint16_t)palettes.push_back({{::llc::BLUE				, ::llc::ORANGE		, ::llc::YELLOW		, ::llc::MAGENTA	, ::llc::CYAN		, {}, ::llc::WHITE				,}});

	dialogColors.ViewportTitle	[::llc::GUI_CONTROL_PALETTE_NORMAL	]			= dialogColors.Button[::llc::GUI_CONTROL_PALETTE_NORMAL		]; palettes.push_back({{::llc::WHITE		* 0.7	, ::llc::DARKGRAY	, ::llc::DARKGRAY	, ::llc::DARKGRAY	, ::llc::DARKGRAY	, {}, ::llc::WHITE				,}});
	dialogColors.ViewportTitle	[::llc::GUI_CONTROL_PALETTE_DISABLED]			= dialogColors.Button[::llc::GUI_CONTROL_PALETTE_DISABLED	]; palettes.push_back({{::llc::LIGHTGRAY			, ::llc::ORANGE		, ::llc::YELLOW		, ::llc::MAGENTA	, ::llc::CYAN		, {}, ::llc::LIGHTGRAY * 1.1	,}});
	dialogColors.ViewportTitle	[::llc::GUI_CONTROL_PALETTE_HOVER	]			= dialogColors.Button[::llc::GUI_CONTROL_PALETTE_HOVER		]; palettes.push_back({{::llc::WHITE		* 0.85	, ::llc::GRAY		, ::llc::GRAY		, ::llc::GRAY		, ::llc::GRAY		, {}, ::llc::WHITE				,}});
	dialogColors.ViewportTitle	[::llc::GUI_CONTROL_PALETTE_PRESSED	]			= dialogColors.Button[::llc::GUI_CONTROL_PALETTE_PRESSED	]; palettes.push_back({{::llc::WHITE		 		, ::llc::BLUE		, ::llc::BLUE		, ::llc::BLUE		, ::llc::BLUE		, {}, ::llc::WHITE				,}});
	dialogColors.ViewportTitle	[::llc::GUI_CONTROL_PALETTE_ACTION	]			= dialogColors.Button[::llc::GUI_CONTROL_PALETTE_ACTION	]; palettes.push_back({{::llc::BLUE					, ::llc::ORANGE		, ::llc::YELLOW		, ::llc::MAGENTA	, ::llc::CYAN		, {}, ::llc::WHITE				,}});
	dialogColors.ViewportTitle	[::llc::GUI_CONTROL_PALETTE_OUTDATED]			= dialogColors.Button[::llc::GUI_CONTROL_PALETTE_OUTDATED	]; palettes.push_back({{::llc::BLUE					, ::llc::ORANGE		, ::llc::YELLOW		, ::llc::MAGENTA	, ::llc::CYAN		, {}, ::llc::WHITE				,}});
	for(uint32_t i=0; i < 4; ++i) {
		dialogColors.Control		[::llc::GUI_CONTROL_PALETTE_SELECTED + i]		= dialogColors.Control			[::llc::GUI_CONTROL_PALETTE_NORMAL	+ i];
		dialogColors.Button			[::llc::GUI_CONTROL_PALETTE_SELECTED + i]		= dialogColors.Button			[::llc::GUI_CONTROL_PALETTE_NORMAL	+ i];
		dialogColors.CheckBox		[::llc::GUI_CONTROL_PALETTE_SELECTED + i]		= dialogColors.CheckBox			[::llc::GUI_CONTROL_PALETTE_NORMAL	+ i];
		dialogColors.Viewport		[::llc::GUI_CONTROL_PALETTE_SELECTED + i]		= dialogColors.Viewport			[::llc::GUI_CONTROL_PALETTE_NORMAL	+ i];
		dialogColors.ViewportTitle	[::llc::GUI_CONTROL_PALETTE_SELECTED + i]		= dialogColors.ViewportTitle	[::llc::GUI_CONTROL_PALETTE_NORMAL	+ i];
	}
	return 0;
}

static	::llc::error_t	dialogInitialize				(::llc::SDialog & dialog) {
	::llc::SGUI						& gui							= *dialog.GUI; 
	llc_necs(dialog.Root = ::llc::controlCreate(gui));
	gui.Controls.Placement		[dialog.Root].Margin				= {};
	gui.Controls.Placement		[dialog.Root].Border				= {};
	gui.Controls.Constraints	[dialog.Root].AttachSizeToControl	= {dialog.Root, dialog.Root};
	gui.Controls.States			[dialog.Root].Mask					&= ~::llc::GUI_CONTROL_FLAG_Hovered;
	gui.Controls.States			[dialog.Root].Mask					&= ~::llc::GUI_CONTROL_FLAG_Action;
	gui.Controls.States			[dialog.Root].Mask					&= ~::llc::GUI_CONTROL_FLAG_Pressed;
	dialog.DefaultControlDraw.FrameOut			= 1;
	dialog.DefaultControlDraw.UseNewPalettes	= 1;
	if(!dialog.Colors) {
		static ::llc::pobj<::llc::SDialogColors>	globalDefaultColors			= {};
		if(!globalDefaultColors) {
			llc_necs(::dialogInitializeColors(*globalDefaultColors.create(), gui.Colors->Palettes));
		}
		dialog.Colors = globalDefaultColors;
	}

	return 0;
}

::llc::SDialog::SDialog			()										{ GUI.create(); ::dialogInitialize(*this); }
::llc::SDialog::SDialog			(::llc::pobj<::llc::SGUI> & pgui)	{
	if(!pgui)
		pgui.create();
	GUI								= pgui;
	::dialogInitialize(*this);
}
::llc::SDialog::SDialog			(const ::llc::pobj<::llc::SGUI> & pgui)	{
	GUI								= pgui;
	if(!GUI)
		GUI.create();
	::dialogInitialize(*this);
}

stacxpr	const uint32_t	heightOfField			= 18;
::llc::cid_t			llc::checkBoxCreate		(::llc::SDialog			& dialog)								{
	cid_t						index					= -1;
	::llc::pobj<::llc::SDialogCheckBox>	checkBox;
	::llc::SControlTable		& controlTable			= dialog.GUI->Controls;
	llc_necs(index = dialog.Create(checkBox));
	controlTable.States[checkBox->IdGUIControl].Mask	= dialog.DefaultControlMask;
	::llc::memcpy_s(controlTable.Draw[checkBox->IdGUIControl].Palettes.Storage, dialog.Colors->CheckBox.Storage);
	return index;
}
::llc::error_t			llc::checkBoxUpdate			(::llc::SDialogCheckBox	& checkbox)								{
	::llc::SDialog				& dialog					= *checkbox.Dialog;
	::llc::SControlTable		& controlTable				= dialog.GUI->Controls;
	::llc::img<::llc::bgra>		& imageCross				= dialog.ImageCrossBGRA;
	if(controlTable.IsAction(checkbox.IdGUIControl)) {
		if(dialog.ImageCross.Texels.size() < 4) {
#if !(defined(LLC_ESP32) || defined(LLC_ARDUINO))
			if_fail_e(::llc::pngFileLoad("../llc_games_data/images/cross.png", imageCross));
#endif
			dialog.ImageCross.resize(imageCross.metrics());
			for(uint32_t iTexel = 0; iTexel < imageCross.Texels.size(); ++iTexel) {
				if(imageCross.Texels[iTexel] == (::llc::bgra)::llc::BLACK)
					dialog.ImageCross.View[iTexel]	=  true;
				else {
					imageCross.Texels[iTexel].a		= 0;
					dialog.ImageCross.View[iTexel]	= false;
				}
			}
		}
		checkbox.Checked									= !checkbox.Checked;
		controlTable.Images[checkbox.IdGUIControl].Image	= (false == checkbox.Checked) ? ::llc::g8bgra{} : imageCross.View;
	}
	return 0;
}

::llc::cid_t			llc::sliderCreate							(::llc::SDialog & dialog)								{
	::llc::cid_t				index										= -1;
	::llc::pobj<::llc::SDialogSlider>	slider;
	llc_necs(index = dialog.Create(slider));
	llc_necs(slider->IdButton = ::llc::controlCreateChild(*dialog.GUI, slider->IdGUIControl));
	::llc::SControlTable		& controlTable								= dialog.GUI->Controls;
	controlTable.Placement	[slider->IdGUIControl].Margin				= {};
	controlTable.Placement	[slider->IdButton].Area.Size				= {16, 16};
	controlTable.Text		[slider->IdButton].Text						= "-";
	controlTable.Constraints[slider->IdButton].AttachSizeToControl.y	= slider->IdButton;
	controlTable.Draw		[slider->IdButton].UseNewPalettes			= true;
	controlTable.Draw		[slider->IdButton].ColorMode				= ::llc::GUI_COLOR_MODE_3D;
	::llc::memcpy_s(controlTable.Draw[slider->IdButton		].Palettes.Storage, dialog.Colors->Button		.Storage);
	::llc::memcpy_s(controlTable.Draw[slider->IdGUIControl	].Palettes.Storage, dialog.Colors->CheckBox	.Storage);
	::llc::SControlText			& tunerText									= controlTable.Text[slider->IdGUIControl];
	tunerText.Text			= {slider->ValueString, (uint32_t)snprintf(slider->ValueString, ::llc::size(slider->ValueString) - 2, "%lli", (long long int)slider->ValueCurrent)};
	return index;
}
::llc::error_t			llc::sliderSetValue		(::llc::SDialogSlider & slider, int64_t value)				{
	::llc::SDialog				& dialog				= *slider.Dialog;
	::llc::SControlTable		& controlTable			= dialog.GUI->Controls;
	const int64_t				newValue				= ::llc::max(::llc::min(value, slider.ValueLimits.Max), slider.ValueLimits.Min);;
	bool						updateMetrics			= slider.ValueCurrent != newValue;
	slider.ValueCurrent		= newValue;

	const double				proportion				= (slider.ValueCurrent - slider.ValueLimits.Min) * (1.0 / (slider.ValueLimits.Max - slider.ValueLimits.Min));

	::llc::SControlPlacement	& controlButton			= controlTable.Placement[slider.IdButton];
	::llc::SControlPlacement	& controlSlider			= controlTable.Placement[slider.IdGUIControl];
	::llc::n2f3_t				ncSpacing				= ::llc::controlNCSpacing(controlSlider).f3();
	if(slider.Vertical)	
		controlButton.Area.Offset.y	= int16_t((controlSlider.Area.Size.y - controlButton.Area.Size.y - ncSpacing.y) * proportion);
	else				
		controlButton.Area.Offset.x	= int16_t((controlSlider.Area.Size.x - controlButton.Area.Size.x - ncSpacing.x) * proportion);

	::llc::SControlConstraints	& buttonConstraints		= controlTable.Constraints[slider.IdButton];
	if(slider.Vertical) {
		buttonConstraints.AttachSizeToControl.y	= -1;
		buttonConstraints.AttachSizeToControl.x	= slider.IdButton;
	}
	else {
		buttonConstraints.AttachSizeToControl.x	= -1;
		buttonConstraints.AttachSizeToControl.y	= slider.IdButton;
	}
	::llc::vcst_t					valueString;
	llc_necs(slider.FuncValueFormat(valueString, slider.ValueCurrent, slider.ValueLimits));
	llc_necs(slider.FuncGetString(valueString, slider.ValueCurrent, slider.ValueLimits));
	llc_necs(llc::controlTextSet(*dialog.GUI, slider.IdGUIControl, valueString));
	if(updateMetrics)
		llc_necs(llc::controlMetricsInvalidate(*dialog.GUI, slider.IdGUIControl));
	return 0;
}

::llc::error_t			llc::sliderUpdate			(::llc::SDialogSlider	& slider)								{
	::llc::SDialog				& dialog					= *slider.Dialog;
	::llc::SControlTable		& controlTable				= dialog.GUI->Controls;
	const int64_t				valueRange					= slider.ValueLimits.Max - slider.ValueLimits.Min;
	if(controlTable.States[slider.IdButton].IsPressed()) {
		if(dialog.Input->MouseCurrent.Position != dialog.Input->MousePrevious.Position) {
			::llc::SInput											& input										= *dialog.Input;
			//const ::llc::SGUIZoom									& zoom										= dialog.GUI->Zoom;
			//::llc::n2d												scale										=
			//	{ 1.0 / (zoom.ZoomLevel * zoom.DPI.x)
			//	, 1.0 / (zoom.ZoomLevel * zoom.DPI.y)
			//	};
			const ::llc::n2f3_t			controlSliderPos			= controlTable.Area[slider.IdGUIControl].Client.Global.Offset.f3();
			const ::llc::n2f3_t			controlSliderSize			= controlTable.Area[slider.IdGUIControl].Client.Global.Size.f3();
			::llc::n2f3_t				effectiveSize				= controlSliderSize - (controlTable.Placement[slider.IdButton].Area.Size).f3();;
			//

			::llc::n2f3_t				currentValue				= input.MouseCurrent.Position.f3();
			const ::llc::n2f3_t			valueUnit					= {1.0 / effectiveSize.x, 1.0 / effectiveSize.y};
			currentValue			-= controlSliderPos + controlTable.Placement[slider.IdButton].Area.Size.f3() / 2.0;
			currentValue.x			*= valueUnit.x;
			currentValue.y			*= valueUnit.y;

			llc_necs(llc::sliderSetValue(slider, (int64_t)((slider.Vertical ? currentValue.y : currentValue.x) * valueRange + slider.ValueLimits.Min)));
		}
	}
	else
		if(dialog.Input->ButtonDown(0) && controlTable.States[slider.IdGUIControl].IsPressed()) {	//
			::llc::SInput				& input						= *dialog.Input;
			const int64_t				valuePage					= ::llc::max((int64_t)1LL, (valueRange > 10) ? (int64_t)(valueRange * .1) : valueRange / 2);
			const ::llc::n2<int16_t>	& controlButtonPosition		= controlTable.Area[slider.IdButton].Total.Global.Offset;
			if(slider.Vertical) {
				const int64_t				finalValue					= slider.ValueCurrent + ((input.MouseCurrent.Position.y > controlButtonPosition.y) ? valuePage : -valuePage);
				llc_necs(llc::sliderSetValue(slider, finalValue));
			}
			else {
				const int64_t				finalValue					= slider.ValueCurrent + ((input.MouseCurrent.Position.x > controlButtonPosition.x) ? valuePage : -valuePage);
				llc_necs(llc::sliderSetValue(slider, finalValue));
			}
		}
	return 0;
}

::llc::cid_t			llc::editBoxCreate		(::llc::SDialog & dialog)								{
	::llc::cid_t				index					= -1;
	::llc::pobj<::llc::SDialogEditBox>	editBox;
	::llc::SControlTable		& controlTable			= dialog.GUI->Controls;
	llc_necs(index = dialog.Create(editBox));
	controlTable.Draw[editBox->IdGUIControl].UseNewPalettes	= true;
	controlTable.Draw[editBox->IdGUIControl].ColorMode		= ::llc::GUI_COLOR_MODE_Flat;
	::llc::memcpy_s(controlTable.Draw[editBox->IdGUIControl].Palettes.Storage, dialog.Colors->CheckBox.Storage);
	return index;
}
::llc::error_t			llc::editBoxUpdate		(::llc::SDialogEditBox	& control)								{
	::llc::SDialog				& dialog				= *control.Dialog;
	if(dialog.Input->ButtonDown(0)) {
	}
	return 0;
}
::llc::cid_t			llc::viewportCreate		(::llc::SDialog & dialog) {
	::llc::cid_t				index					= -1;
	::llc::pobj<::llc::SDialogViewport>	viewport;
	::llc::SControlTable		& controlTable			= dialog.GUI->Controls;
	llc_necs(index = dialog.Create(viewport));
	controlTable.Draw[viewport->IdGUIControl].UseNewPalettes	= true;
	controlTable.Draw[viewport->IdGUIControl].FrameOut			= true;
	//controlTable.Modes[viewport->IdGUIControl].NoHover			= true;
	controlTable.Placement[viewport->IdGUIControl].Margin		= {};
	::llc::memcpy_s(controlTable.Draw[viewport->IdGUIControl].Palettes.Storage, dialog.Colors->Viewport.Storage);
	{ // Create field group
		// Set up client area control
		::llc::cid_t				idControl;
		llc_necs(viewport->IdClient = idControl = ::llc::controlCreateChild(*dialog.GUI, viewport->IdGUIControl));		// Create group control array
		::llc::SControlPlacement	& control				= controlTable.Placement[idControl];
		control.Area.Offset		= {0, heightOfField + 1};
		control.Area.Size		= {2, 2};
		control.Margin			= {};
		controlTable.Constraints[idControl].AttachSizeToControl.x	= idControl;
		controlTable.States[idControl].Mask		= dialog.DefaultControlMask;
		controlTable.Draw [idControl].Palettes	= dialog.Colors->Viewport;
	}
	{ // Create section title
		::llc::cid_t				idControl;
		llc_necall(viewport->IdTitle = idControl = ::llc::controlCreateChild(*dialog.GUI, viewport->IdGUIControl), "%s", "????");	// Create group control array

		::llc::SControlPlacement	& control				= controlTable.Placement[idControl];
		control.Area.Offset		= {};
		control.Area.Size		= {2, heightOfField};
		control.Align			= ::llc::ALIGN_TOP_LEFT;
		control.Margin.Left		= 4;
		controlTable.Constraints[idControl].AttachSizeToControl.x	= idControl;
		controlTable.States[idControl].Mask		= dialog.DefaultControlMask;
		controlTable.Draw [idControl].Palettes	= dialog.Colors->ViewportTitle;
		controlTable.Text [idControl].Text		= "Viewport";
	}
	viewport->Settings.Unfolded	= true;
	return index;
}

::llc::error_t			llc::viewportAdjustSize	(::llc::n2<int16_t> & sizeViewport, const ::llc::n2<int16_t> & sizeClient)		{
	sizeViewport			= sizeClient;
	sizeViewport.x			+= 4;
	sizeViewport.y			+= 4 + 1 + heightOfField;
	return 0;
}

::llc::error_t			llc::viewportFold		(::llc::SDialogViewport	& control, bool fold)														{
	if((fold && control.Settings.Unfolded) || (false == control.Settings.Unfolded && false == fold)) {
		::llc::SDialog				& dialog				= *control.Dialog;
		::llc::SGUI					& gui					= *dialog.GUI;
		::llc::SControlTable		& controlTable			= gui.Controls;
		::llc::SControlPlacement	& placementMain			= controlTable.Placement[control.IdGUIControl];
		::llc::SControlPlacement	& placementTitle		= controlTable.Placement[control.IdTitle];
		if(fold && control.Settings.Unfolded)
			placementMain.Area.Size.y	= placementTitle.Area.Size.y + ::llc::controlNCSpacing(placementMain).y;
		else if(false == control.Settings.Unfolded && false == fold) {
			::llc::SControlPlacement	& controlClient			= controlTable.Placement[control.IdClient];
			placementMain.Area.Size.y	= controlClient.Area.Size.y + placementTitle.Area.Size.y + ::llc::controlNCSpacing(placementMain).y + 1;
		}

		::llc::SControlState			& controlMain	= controlTable.States[control.IdGUIControl];
		if(-1 != controlMain.Parent)
			llc_necs(llc::controlMetricsInvalidate(gui, controlMain.Parent));
		else
			llc_necs(llc::controlMetricsInvalidate(gui, control.IdGUIControl));

		::llc::SControlState		& stateClient			= controlTable.States[control.IdClient];
		stateClient.SetHidden(control.IdClient, controlTable.EventQueue, fold);
		control.Settings.Unfolded	= !fold;
	}
	return one_if(false == control.Settings.Unfolded);
}

static	::llc::error_t	viewportDrag			(::llc::SDialogViewport	& control, ::llc::SControlPlacement & controlMain, ::llc::SDialog & dialog, ::llc::n2<bool> locked, ::llc::n2s1_t mouseDeltas) {
	mouseDeltas.InPlaceScale(1 / dialog.GUI->Zoom.DPI.x, 1 / dialog.GUI->Zoom.DPI.y);
	mouseDeltas.InPlaceScale(1 / dialog.GUI->Zoom.ZoomLevel);
	control.Settings.Dragging	= true;
	if(false == locked.x)
		controlMain.Area.Offset.x	+= ((controlMain.Align & ::llc::ALIGN_HCENTER) == ::llc::ALIGN_RIGHT) ? -mouseDeltas.x : mouseDeltas.x;
	if(false == locked.y)
		controlMain.Area.Offset.y	+= ((controlMain.Align & ::llc::ALIGN_VCENTER) == ::llc::ALIGN_BOTTOM) ? -mouseDeltas.y : mouseDeltas.y;
	::llc::controlMetricsInvalidate(*dialog.GUI, control.IdGUIControl);
	return 0;
}

::llc::error_t			llc::viewportUpdate		(::llc::SDialogViewport	& control)																{
	::llc::SDialog				& dialog				= *control.Dialog;
	::llc::SControlTable		& controlTable			= dialog.GUI->Controls;
	::llc::SControlPlacement	& controlMain			= controlTable.Placement[control.IdGUIControl];
	::llc::SControlState		& controlTitleState		= controlTable.States[control.IdTitle];
	if(controlTitleState.IsPressed()) {
		const ::llc::n2<bool>		locked					= {control.Settings.DisplacementLockX, control.Settings.DisplacementLockY};
		if(false == locked.x || false == locked.y) {
			::llc::n2s1_t				mouseDeltas				= (dialog.Input->MouseCurrent.Position - dialog.Input->MousePrevious.Position).s1(); //{dialog.Input->MouseCurrent.Deltas.x, dialog.Input->MouseCurrent.Deltas.y};
			if(mouseDeltas.x || mouseDeltas.y)
				llc_necs(::viewportDrag(control, controlMain, dialog, locked, mouseDeltas));
		}
	}
	else {
		if(controlTitleState.IsAction() && false == control.Settings.Dragging)
			llc_necs(llc::viewportFold(control, control.Settings.Unfolded));

		control.Settings.Dragging	= false;
	}
	if(control.Settings.Unfolded != control.SettingsOld.Unfolded)
		if(true == control.Settings.Unfolded) {
			::llc::n2s1_t				clientFinalSize			= {controlMain.Area.Size};
			clientFinalSize			-= ::llc::controlNCSpacing(controlMain).s1();
			clientFinalSize.y		-=  heightOfField + 1;
			::llc::SControlPlacement	& controlClient			= controlTable.Placement[control.IdClient];
			controlClient.Area.Size	= clientFinalSize;
		}
	control.SettingsOld		= control.Settings;
	return 0;
}
