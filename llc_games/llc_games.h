#include "llc_gui.h"
#include "llc_window.h"
#include "llc_tictactoe.h"
#include "llc_gontrill.h"

#ifndef LLC_GAMES_H_23627
#define LLC_GAMES_H_23627

enum GAMES_SCREEN : ::llc::u0_t { GAMES_SCREEN_MENU, GAMES_SCREEN_TICTACTOE, GAMES_SCREEN_GONTRILL };

stct SLLCGames {
	::llc::SWindow			& Window;
	::llc::SGUI				& GUI;
	::llc::TicTacToe		Game			= {};
	::llc::SGontrill		Gontrill		= {};
	::llc::cid_t			Title			= ::llc::CID_INVALID;
	::llc::cid_t			Play			= ::llc::CID_INVALID;
	::llc::cid_t			PlayGontrill	= ::llc::CID_INVALID;
	::llc::cid_t			Exit			= ::llc::CID_INVALID;
	::llc::cid_t			Status			= ::llc::CID_INVALID;
	::llc::cid_t			Restart			= ::llc::CID_INVALID;
	::llc::cid_t			Back			= ::llc::CID_INVALID;
	::llc::astatic<::llc::cid_t, 9>
						Cells			= {};
	::llc::cid_t			GontrillHud		= ::llc::CID_INVALID;
	::llc::cid_t			GontrillHelp	= ::llc::CID_INVALID;
	::llc::cid_t			GontrillBack	= ::llc::CID_INVALID;
	char					GontrillHudText[160] = {};
	GAMES_SCREEN			Screen			= GAMES_SCREEN_MENU;

	SLLCGames(::llc::SWindow & window, ::llc::SGUI & gui) : Window(window), GUI(gui) {}
};

::llc::err_t	gamesSetup	(SLLCGames & app);
::llc::err_t	gamesUpdate	(SLLCGames & app, ::llc::f2_t secondsLastFrame);
::llc::err_t	gamesDraw	(SLLCGames & app, ::llc::g8bgra target);

#endif // LLC_GAMES_H_23627
