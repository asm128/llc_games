#include "llc_games.h"

#include "llc_app_impl.h"
#include "llc_framework.h"
#include "llc_runtime.h"
#include "llc_system_key.h"
#include "llc_size.h"

#include <cstdio>

sttc ::llc::err_t gamesControlCreate(SLLCGames & app, ::llc::cid_t & id, ::llc::vcst_t text, cnst ::llc::rect2s1_t & area, ::llc::s1_t font) {
	if_fail_fe(id = ::llc::controlCreate(app.GUI));
	::llc::SControlPlacement & placement = app.GUI.Controls.Placement[id];
	placement.Area = area;
	placement.Align = ::llc::ALIGN_CENTER;
	placement.Margin = {2, 2, 2, 2};
	placement.Border = {3, 3, 3, 3};
	app.GUI.Controls.Text[id].Align = ::llc::ALIGN_CENTER;
	app.GUI.Controls.Text[id].FontSelected = font;
	rtrn ::llc::controlTextSet(app.GUI, id, text);
}

sttc void gamesScreenSet(SLLCGames & app, GAMES_SCREEN screen) {
	app.Screen = screen;
	app.GUI.Controls.SetHidden(app.Title, screen != GAMES_SCREEN_MENU);
	app.GUI.Controls.SetHidden(app.Play, screen != GAMES_SCREEN_MENU);
	app.GUI.Controls.SetHidden(app.PlayGontrill, screen != GAMES_SCREEN_MENU);
	app.GUI.Controls.SetHidden(app.Exit, screen != GAMES_SCREEN_MENU);
	app.GUI.Controls.SetHidden(app.Status, screen != GAMES_SCREEN_TICTACTOE);
	app.GUI.Controls.SetHidden(app.Restart, screen != GAMES_SCREEN_TICTACTOE);
	app.GUI.Controls.SetHidden(app.Back, screen != GAMES_SCREEN_TICTACTOE);
	for(cnst ::llc::cid_t id : app.Cells)
		app.GUI.Controls.SetHidden(id, screen != GAMES_SCREEN_TICTACTOE);
	app.GUI.Controls.SetHidden(app.GontrillHud, screen != GAMES_SCREEN_GONTRILL);
	app.GUI.Controls.SetHidden(app.GontrillHelp, screen != GAMES_SCREEN_GONTRILL);
	app.GUI.Controls.SetHidden(app.GontrillBack, screen != GAMES_SCREEN_GONTRILL);
}

sttc ::llc::err_t gamesBoardRefresh(SLLCGames & app) {
	for(::llc::u0_t i = 0; i < app.Cells.size(); ++i) {
		cnst ::llc::CELL_VALUE value = app.Game.Board.GetCellValue(i);
		if_fail_fe(::llc::controlTextSet(app.GUI, app.Cells[i], value == ::llc::CELL_VALUE_X ? ::llc::vcst_t{"X"} : value == ::llc::CELL_VALUE_O ? ::llc::vcst_t{"O"} : ::llc::vcst_t{}));
	}
	cnst ::llc::vcst_t status = app.Game.Board.GetWinner() == ::llc::CELL_VALUE_X ? ::llc::vcst_t{"YOU WIN"}
		: app.Game.Board.GetWinner() == ::llc::CELL_VALUE_O ? ::llc::vcst_t{"COMPUTER WINS"}
		: app.Game.Board.MovesConsumed == 9 ? ::llc::vcst_t{"DRAW"}
		: ::llc::vcst_t{"YOUR TURN (X)"};
	rtrn ::llc::controlTextSet(app.GUI, app.Status, status);
}

sttc ::llc::err_t gamesStart(SLLCGames & app) {
	app.Game = {};
	if_fail_fe(::gamesBoardRefresh(app));
	::gamesScreenSet(app, GAMES_SCREEN_TICTACTOE);
	rtrn ::llc::guiUpdateMetrics(app.GUI, app.Window.Size, false);
}

sttc ::llc::err_t gamesStartGontrill(SLLCGames & app) {
	if_fail_fe(::llc::gontrillLoad(app.Gontrill));
	if_fail_fe(::llc::gontrillReset(app.Gontrill, app.Window.Size.u2()));
	::gamesScreenSet(app, GAMES_SCREEN_GONTRILL);
	rtrn ::llc::guiUpdateMetrics(app.GUI, app.Window.Size, false);
}

::llc::err_t gamesSetup(SLLCGames & app) {
	::llc::SRasterFontManager fonts = {};
	if_fail_fe(::llc::rasterFontDefaults(fonts));
	app.GUI.Fonts = fonts.Fonts;
	app.GUI.SelectedFont = 2;
	if_fail_fe(::gamesControlCreate(app, app.Title, "LLC GAMES", {{0, -220}, {420, 60}}, 3));
	if_fail_fe(::gamesControlCreate(app, app.Play, "TIC-TAC-TOE", {{0, -68}, {280, 64}}, 2));
	if_fail_fe(::gamesControlCreate(app, app.PlayGontrill, "GONTRILL", {{0, 12}, {280, 64}}, 2));
	if_fail_fe(::gamesControlCreate(app, app.Exit, "EXIT", {{0, 100}, {280, 56}}, 2));
	if_fail_fe(::gamesControlCreate(app, app.Status, "YOUR TURN (X)", {{0, -152}, {420, 50}}, 2));
	for(::llc::u0_t i = 0; i < app.Cells.size(); ++i) {
		cnst ::llc::s1_t x = (::llc::s1_t)(((::llc::s1_t)(i % 3) - 1) * 96);
		cnst ::llc::s1_t y = (::llc::s1_t)(((::llc::s1_t)(i / 3) - 1) * 96 + 18);
		if_fail_fe(::gamesControlCreate(app, app.Cells[i], {}, {{x, y}, {88, 88}}, 3));
	}
	if_fail_fe(::gamesControlCreate(app, app.Restart, "RESTART", {{-100, 190}, {180, 52}}, 2));
	if_fail_fe(::gamesControlCreate(app, app.Back, "BACK TO GAMES", {{100, 190}, {180, 52}}, 2));
	::snprintf(app.GontrillHudText, ::llc::size(app.GontrillHudText), "GONTRILL | P: PAUSE | F2: DEBUG | ESC: MENU");
	if_fail_fe(::gamesControlCreate(app, app.GontrillHud, ::llc::vcst_t{app.GontrillHudText}, {{0, -268}, {680, 42}}, 1));
	if_fail_fe(::gamesControlCreate(app, app.GontrillHelp, "P1 WASD/TYU | P2 ARROWS/NUM123 | 0-9 WEAPON | P PAUSE | F2 STATS", {{0, 218}, {760, 32}}, 1));
	if_fail_fe(::gamesControlCreate(app, app.GontrillBack, "BACK TO GAMES", {{0, 258}, {210, 36}}, 1));
	::gamesScreenSet(app, GAMES_SCREEN_MENU);
	rtrn ::llc::guiUpdateMetrics(app.GUI, app.Window.Size, true);
}

::llc::err_t gamesUpdate(SLLCGames & app, ::llc::f2_t secondsLastFrame) {
	cnst ::llc::SInput & input = *app.Window.Input;
	if(app.Window.Closed)
		rtrn ::llc::APPLICATION_STATE_EXIT;
	if(input.KeyDown(::llc::SYSTEM_KEY_Escape)) {
		if(app.Screen == GAMES_SCREEN_MENU)
			rtrn ::llc::APPLICATION_STATE_EXIT;
		::gamesScreenSet(app, GAMES_SCREEN_MENU);
		rtrn ::llc::guiUpdateMetrics(app.GUI, app.Window.Size, false);
	}
	if(app.Screen == GAMES_SCREEN_MENU) {
		if(app.GUI.Controls.IsAction(app.Exit))
			rtrn ::llc::APPLICATION_STATE_EXIT;
		if(app.GUI.Controls.IsAction(app.Play))
			if_fail_fe(::gamesStart(app));
		if(app.GUI.Controls.IsAction(app.PlayGontrill))
			if_fail_fe(::gamesStartGontrill(app));
	}
	else if(app.Screen == GAMES_SCREEN_TICTACTOE) {
		if(app.GUI.Controls.IsAction(app.Back))
			::gamesScreenSet(app, GAMES_SCREEN_MENU);
		else if(app.GUI.Controls.IsAction(app.Restart)) {
			if_fail_fe(::gamesStart(app));
		}
		else if(app.Game.Board.GetWinner() == ::llc::CELL_VALUE_EMPTY && app.Game.Board.MovesConsumed < 9) {
			for(::llc::u0_t i = 0; i < app.Cells.size(); ++i) {
				if(!app.GUI.Controls.IsAction(app.Cells[i]) || app.Game.Board.GetCellValue(i) != ::llc::CELL_VALUE_EMPTY)
					continue;
				app.Game.Tick(i);
				if(app.Game.Board.GetWinner() == ::llc::CELL_VALUE_EMPTY && app.Game.Board.MovesConsumed < 9 && app.Game.Board.GetPlayerControlCurrent() == ::llc::PLAYER_CONTROL_AI)
					app.Game.Tick(0);
				if_fail_fe(::gamesBoardRefresh(app));
				break;
			}
		}
	}
	else if(app.Screen == GAMES_SCREEN_GONTRILL) {
		if(app.GUI.Controls.IsAction(app.GontrillBack))
			::gamesScreenSet(app, GAMES_SCREEN_MENU);
		else {
			if_fail_fe(::llc::gontrillUpdate(app.Gontrill, input, secondsLastFrame, app.Window.Size.u2()));
			if(app.Gontrill.Debugging) {
				::llc::u2_t enemies = 0, shots = 0, powerups = 0;
				for(cnst ::llc::SGontrillEnemy & enemy : app.Gontrill.Enemies) enemies += enemy.Alive;
				for(cnst ::llc::SGontrillShot & shot : app.Gontrill.Shots) shots += shot.Alive;
				for(cnst ::llc::SGontrillPowerup & powerup : app.Gontrill.Powerups) powerups += powerup.Alive;
				::snprintf(app.GontrillHudText, ::llc::size(app.GontrillHudText), "GONTRILL DEBUG  SCORE %u  ENEMIES %u  SHOTS %u  POWERUPS %u", app.Gontrill.Score, enemies, shots, powerups);
			}
			else
				::snprintf(app.GontrillHudText, ::llc::size(app.GontrillHudText), "GONTRILL  SCORE %u  P1 %d/%d  P2 %d/%d  %s", app.Gontrill.Score, app.Gontrill.Ships[0].Health, app.Gontrill.Ships[0].Shield, app.Gontrill.Ships[1].Health, app.Gontrill.Ships[1].Shield, app.Gontrill.GameOver ? "GAME OVER" : app.Gontrill.Paused ? "PAUSED" : "P: PAUSE");
			if_fail_fe(::llc::controlTextSet(app.GUI, app.GontrillHud, ::llc::vcst_t{app.GontrillHudText}));
			app.GUI.Controls.SetUpdated(app.GontrillHud, false);
		}
	}
	rtrn ::llc::guiUpdateMetrics(app.GUI, app.Window.Size, false);
}

::llc::err_t gamesDraw(SLLCGames & app, ::llc::g8bgra target) {
	if(app.Screen == GAMES_SCREEN_GONTRILL) {
		if_fail_fe(::llc::gontrillDraw(app.Gontrill, target));
	}
	else {
		if_fail_fe(target.fill(::llc::SColorBGRA{16, 14, 12}));
	}
	rtrn ::llc::guiDraw(app.GUI, target);
}

#define LLC_GAMES_TITLE "LLC games"

stct SApplication {
	::llc::SFramework	Framework;
	SLLCGames		Games;

	SApplication(::llc::SRuntimeValues & runtimeValues)
		: Framework(runtimeValues, {}), Games(Framework.RootWindow, *Framework.GUI) {}
};

::llc::err_t setup(SApplication & app) {
	::llc::SWindow & window = app.Framework.RootWindow;
	window.Size = {800, 600};
	if_null_ve(::llc::OS_NO_MEMORY, window.BackBuffer.create());
	if_fail_fe(window.BackBuffer->resize(window.Size.u2()));
	if_fail_fe(::llc::mainWindowCreate(window, app.Framework.RuntimeValues.PlatformDetail, window.Input));
	rtrn ::gamesSetup(app.Games);
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
	cnst ::llc::err_t resultGames = ::gamesUpdate(app.Games, (::llc::f2_t)app.Framework.FrameInfo.Seconds.LastFrame);
	::llc::sleep(1);
	rtrn resultGames;
}

::llc::err_t draw(SApplication & app) {
	::llc::SWindow & window = app.Framework.RootWindow;
	if(window.NoDraw)
		rtrn 0;
	if_fail_fe(window.BackBuffer->resize(window.Size.u2()));
	rtrn ::gamesDraw(app.Games, window.BackBuffer->Color.View);
}

LLC_DEFINE_APPLICATION_ENTRY_POINT(SApplication, LLC_GAMES_TITLE)
