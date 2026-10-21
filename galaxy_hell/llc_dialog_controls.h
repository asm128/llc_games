#include "llc_dialog.h"
#include "llc_label.h"

#ifndef LLC_DIALOG_CONTROLS_H_23627
#define LLC_DIALOG_CONTROLS_H_23627

namespace llc
{
	struct SDialogSlider;
	::llc::cid_t			sliderCreate		(::llc::SDialog					& dialog);
	::llc::error_t			sliderSetValue		(::llc::SDialogSlider			& control, int64_t value);
	::llc::error_t			sliderUpdate		(::llc::SDialogSlider			& control);

	struct SDialogCheckBox;
	::llc::cid_t			checkBoxCreate		(::llc::SDialog					& dialog);
	::llc::error_t			checkBoxUpdate		(::llc::SDialogCheckBox			& control);

	struct SDialogEditBox;
	::llc::cid_t			editBoxCreate		(::llc::SDialog					& dialog);
	::llc::error_t			editBoxUpdate		(::llc::SDialogEditBox			& control);

	struct SDialogViewport;
	::llc::cid_t			viewportCreate		(::llc::SDialog					& dialog);
	::llc::error_t			viewportUpdate		(::llc::SDialogViewport			& control);
	::llc::error_t			viewportFold		(::llc::SDialogViewport			& control, bool fold);
	::llc::error_t			viewportAdjustSize	(::llc::n2<int16_t>		& sizeViewport, const ::llc::n2<int16_t> & sizeClient);

	stainli	::llc::cid_t	sliderCreate		(::llc::SDialog			& dialog, ::llc::pnco<SDialogSlider		>	& createdControl)	 { ::llc::cid_t index = -1; llc_necs(index = sliderCreate	(dialog)); dialogControlAs(dialog.Controls[index], createdControl); return index; }
	stainli	::llc::cid_t	checkBoxCreate		(::llc::SDialog			& dialog, ::llc::pnco<SDialogCheckBox	>	& createdControl)	 { ::llc::cid_t index = -1; llc_necs(index = checkBoxCreate	(dialog)); dialogControlAs(dialog.Controls[index], createdControl); return index; }
	stainli	::llc::cid_t	editBoxCreate		(::llc::SDialog			& dialog, ::llc::pnco<SDialogEditBox	>	& createdControl)	 { ::llc::cid_t index = -1; llc_necs(index = editBoxCreate	(dialog)); dialogControlAs(dialog.Controls[index], createdControl); return index; }
	stainli	::llc::cid_t	viewportCreate		(::llc::SDialog			& dialog, ::llc::pnco<SDialogViewport	>	& createdControl)	 { ::llc::cid_t index = -1; llc_necs(index = viewportCreate	(dialog)); dialogControlAs(dialog.Controls[index], createdControl); return index; }

#pragma pack(push, 1)
	struct SDialogViewportSettings {
		bool					Dragging			: 1;
		bool					Unfolded			: 1;
		bool					DisplacementLockX	: 1;
		bool					DisplacementLockY	: 1;
		bool					ScrollBarX			: 1;
		bool					ScrollBarY			: 1;
	};

	struct SDialogViewport : public ::llc::IDialogControl {
		cid_t					IdClose				= -1;
		cid_t					IdMinimize			= -1;
		cid_t					IdMaximize			= -1;
		cid_t					IdScrollBarX		= -1;
		cid_t					IdScrollBarY		= -1;
		cid_t					IdTitle				= -1;
		cid_t					IdClient			= -1;
		SDialogViewportSettings	Settings			= {};
		SDialogViewportSettings	SettingsOld			= {};

		virtual	::llc::error_t	Update				(::llc::view<const ::llc::pobj<::llc::SEventSystem>> )	{ return ::llc::viewportUpdate(*this); }
	};
#pragma pack(pop)

	struct SDialogEditBox : public ::llc::IDialogControl {
		::llc::minmax<uint32_t>	Selection			= {};
		::llc::apod<sc_t>		String				= {};

		virtual	::llc::error_t	Update				(::llc::view<const ::llc::pobj<::llc::SEventSystem>> )	{ return ::llc::editBoxUpdate(*this); }
	};

	struct SDialogCheckBox : public ::llc::IDialogControl {
		bool					Checked				= false;

		virtual	::llc::error_t	Update				(::llc::view<const ::llc::pobj<::llc::SEventSystem>> )	{ return ::llc::checkBoxUpdate(*this); }
	};

	struct SDialogSlider : public ::llc::IDialogControl {
		typedef	::llc::function<::llc::error_t(::llc::vcst_t & format, int64_t value, const ::llc::minmax<int64_t> & limits)>	
								TCallback;

		cid_t					IdButton			= (cid_t)-1;
		bool					Vertical			= false;
		::llc::minmax<int64_t>	ValueLimits			= {0, 100};// 0x7fFFffFF};
		int64_t					ValueCurrent		= -1;
		sc_t					ValueString	[64]	= {};

		TCallback				FuncValueFormat		= []	(::llc::vcst_t & format, int64_t, const ::llc::minmax<int64_t> &)			mutable	{ format = ::llc::vcs("%lli"); return 0; };
		TCallback				FuncGetString		= [this](::llc::vcst_t & inouts, int64_t value, const ::llc::minmax<int64_t> &)			{ 
			inouts	= {ValueString, (uint32_t)snprintf(ValueString, ::llc::size(ValueString) - 2, ::llc::label(inouts).begin(), value)}; 
			return 0; 
		};

		virtual	::llc::error_t	Update				(::llc::view<const ::llc::pobj<::llc::SEventSystem>> )	{ return ::llc::sliderUpdate(*this); }
	};

	tplt<tpnm _tValue>
	struct SDialogTuner : public ::llc::IDialogControl {
		typedef	::llc::function<::llc::error_t(::llc::vcst_t & format, _tValue value, const ::llc::minmax<_tValue> & limits)>	
								TCallback;

		cid_t					IdDecrease			= -1;
		cid_t					IdIncrease			= -1;

		::llc::minmax<_tValue>	ValueLimits			= {0, 0x7F};
		_tValue					ValueCurrent		= 0;
		sc_t					ValueString	[64]	= {};

		TCallback				FuncValueFormat	= [this](::llc::vcst_t & format, _tValue/*value*/, const ::llc::minmax<_tValue> &)	mutable	{ format = ::llc::vcs("%lli"); return 0; };
		TCallback				FuncGetString	= [this](::llc::vcst_t & inouts, _tValue value, const ::llc::minmax<_tValue> &)			{ 
			inouts	= {ValueString, (uint32_t)snprintf(ValueString, ::llc::size(ValueString) - 2, ::llc::label(inouts).begin(), value)}; 
			return 0; 
		};

		virtual	::llc::error_t	Update				(::llc::view<const ::llc::pobj<::llc::SEventSystem>> eventsIn) { 
			::llc::SDialog				& dialog			= *Dialog;
			::llc::SControlTable		& controlTable		= dialog.GUI->Controls;
			for(uint32_t iEvent = 0; iEvent < eventsIn.size(); ++iEvent) {
				if(eventsIn[iEvent]->Type == ::llc::SYSTEM_EVENT_GUI) {
					::llc::eventExtractAndHandle<::llc::EVENT_GUI_CONTROL>(*eventsIn[iEvent], [this, &controlTable](const ::llc::SEventView<::llc::EVENT_GUI_CONTROL> & eventGui) {
						switch(eventGui.Type) {
						default: break;
						case ::llc::EVENT_GUI_CONTROL_StateChange: {
							const ::llc::SChangeControlState	stateChange		= *(const ::llc::SChangeControlState*)eventGui.Data.begin();
							if((stateChange.Set & ::llc::GUI_CONTROL_FLAG_Action) && stateChange.Id == IdDecrease) 
								 return SetValue(ValueCurrent - 1);
							else if((stateChange.Set & ::llc::GUI_CONTROL_FLAG_Action) && stateChange.Id == IdIncrease) 
								return SetValue(ValueCurrent + 1);
							break;
						}
						}
						return 0;
					});
				}
			}

			return 0;
		}

		::llc::error_t			SetValue			(_tValue value)	{
			if(value < ValueLimits.Min)
				value					= ValueLimits.Min;
			else if(value > ValueLimits.Max)
				value					= ValueLimits.Max;
			ValueCurrent			= value;

			::llc::vcst_t				valueString;
			FuncValueFormat	(valueString, value, ValueLimits);
			FuncGetString	(valueString, ValueCurrent, ValueLimits);
			llc_necs(llc::controlTextSet(*Dialog->GUI, IdGUIControl, valueString));
			llc_necs(llc::controlMetricsInvalidate(*Dialog->GUI, IdGUIControl));
			return 0;
		}
	};

	tplt<tpnm _tValue>
	::llc::cid_t			tunerCreate							(::llc::SDialog	& dialog)								{
		::llc::cid_t				index								= -1;
		::llc::pobj<::llc::SDialogTuner<_tValue>>	tuner;
		llc_necs(index = dialog.Create(tuner));
		tuner->ValueCurrent		= 0; //tuner->ValueLimits.Min;
		llc_necs(tuner->IdDecrease = ::llc::controlCreateChild(*dialog.GUI, tuner->IdGUIControl));
		llc_necs(tuner->IdIncrease = ::llc::controlCreateChild(*dialog.GUI, tuner->IdGUIControl));
		::llc::SControlTable		& controlTable						= dialog.GUI->Controls;
		controlTable.Placement[tuner->IdGUIControl].Margin	= {};
		for(int32_t iControl = tuner->IdDecrease; iControl < tuner->IdIncrease + 1; ++iControl) {
			::llc::SControlPlacement	& controlButton						= controlTable.Placement[iControl];
			controlButton.Align		= (iControl == tuner->IdDecrease) ? ::llc::ALIGN_CENTER_LEFT : ::llc::ALIGN_CENTER_RIGHT;
			::llc::memcpy_s(controlTable.Draw[iControl].Palettes.Storage, dialog.Colors->Button.Storage);
			controlTable.Text			[iControl].Text						= (iControl == tuner->IdDecrease) ? "-" : "+";
			controlTable.Draw			[iControl].UseNewPalettes			= true;
			controlTable.Draw			[iControl].ColorMode				= ::llc::GUI_COLOR_MODE_3D;
			controlTable.Constraints	[iControl].AttachSizeToControl.y	= (::llc::cid_t)iControl;
		}
		tuner->SetValue(tuner->ValueCurrent);
		controlTable.Text[tuner->IdGUIControl].Align		= ::llc::ALIGN_CENTER;
		return index;
	}

	tplt<tpnm _tValue>	
	stainli	::llc::cid_t	tunerCreate							(::llc::SDialog & dialog, ::llc::pnco<::llc::SDialogTuner<_tValue>>	& createdControl)	 { 
		::llc::cid_t				index								= -1; 
		llc_necs(index = ::llc::tunerCreate<_tValue>(dialog)); 
		dialogControlAs(dialog.Controls[index], createdControl); 
		return index;
	}

}

#endif // LLC_DIALOG_CONTROLS_H_23627
