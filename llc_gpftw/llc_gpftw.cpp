#include "../llc_gpftw_port.h"
#include "llc_app_impl.h"

#include <cmath>

namespace {
	stacxpr ::llc::s2_t MapWidth = 32, MapHeight = 17, CellSize = 20;

	struct SEnemy {
		::llc::n2f2_t	Position	= {};
		::llc::s2_t		Health		= 0;
		::llc::f2_t		AttackTime	= 0;
	};

	struct SShot {
		::llc::n2f2_t	Position	= {};
		::llc::n2f2_t	Direction	= {};
		::llc::f2_t		Age			= 0;
		bool				Alive		= false;
	};

	struct SGame {
		::llc::u0_t		Floor[MapHeight][MapWidth] = {};
		::llc::n2f2_t	Player		= {5, 5};
		::llc::n2f2_t	Facing		= {1, 0};
		SEnemy			Enemies[5]	= {};
		SShot			Shots[64]	= {};
		::llc::s2_t		Health		= 100;
		::llc::s2_t		Experience	= 0;
		::llc::f2_t		FireTime	= 0;
	};

	sttc void reset(SGame & game) {
		game = {};
		for(::llc::s2_t y = 0; y < MapHeight; ++y)
		for(::llc::s2_t x = 0; x < MapWidth; ++x)
			game.Floor[y][x] = (x == 0 || y == 0 || x == MapWidth - 1 || y == MapHeight - 1) ? 1 : 0;
		for(::llc::u0_t i = 0; i < ::llc::size(game.Enemies); ++i)
			game.Enemies[i] = {{23.0f + (i % 2) * 4.0f, 3.0f + i * 2.5f}, (::llc::s2_t)(10 + 10 * i), 0};
	}

	sttc ::llc::f2_t distanceSquared(::llc::n2f2_c & a, ::llc::n2f2_c & b) nxpt {
		cnst ::llc::f2_t dx = a.x - b.x, dy = a.y - b.y;
		rtrn dx * dx + dy * dy;
	}

	sttc void shoot(SGame & game) {
		for(SShot & shot : game.Shots) {
			if(shot.Alive) continue;
			shot = {{game.Player.x + game.Facing.x * .55f, game.Player.y + game.Facing.y * .55f}, game.Facing, 0, true};
			break;
		}
	}

	sttc void updateGame(SGame & game, ::llc::SInput const & input, ::llc::f2_t elapsed) {
		if(input.KeyDown('R')) ::reset(game);
		if(game.Health <= 0) rtrn;
		::llc::u0_t living = 0;
		for(cnst SEnemy & enemy : game.Enemies) living += enemy.Health > 0;
		if(!living) rtrn;
		cnst ::llc::f2_t dt = ::llc::min(elapsed, .05f);
		cnst auto & keys = input.KeyboardCurrent.KeyState;
		::llc::n2f2_t direction = {(keys['D'] ? 1.0f : 0) - (keys['A'] ? 1.0f : 0), (keys['S'] ? 1.0f : 0) - (keys['W'] ? 1.0f : 0)};
		if(direction.LengthSquared() > 0) {
			direction.InPlaceNormalize();
			game.Facing = direction;
			cnst ::llc::f2_t speed = keys[::llc::SYSTEM_KEY_Shift] ? 8.25f : 5.5f;
			game.Player.x = ::llc::max(1.0f, ::llc::min((::llc::f2_t)MapWidth - 2, game.Player.x + direction.x * speed * dt));
			game.Player.y = ::llc::max(1.0f, ::llc::min((::llc::f2_t)MapHeight - 2, game.Player.y + direction.y * speed * dt));
		}
		game.FireTime -= dt;
		if(keys[::llc::SYSTEM_KEY_Space] && game.FireTime <= 0) {
			::shoot(game);
			game.FireTime = .2f;
		}
		for(::llc::u0_t i = 0; i < ::llc::size(game.Enemies); ++i) {
			SEnemy & enemy = game.Enemies[i];
			if(enemy.Health <= 0) continue;
			::llc::n2f2_t toward = {game.Player.x - enemy.Position.x, game.Player.y - enemy.Position.y};
			cnst ::llc::f2_t range = toward.LengthSquared();
			if(range > .5f) {
				toward.InPlaceNormalize();
				cnst ::llc::f2_t speed = .5f + i * .25f;
				enemy.Position.x += toward.x * speed * dt;
				enemy.Position.y += toward.y * speed * dt;
			}
			enemy.AttackTime -= dt;
			if(range < .7f && enemy.AttackTime <= 0) { game.Health = ::llc::max(0, game.Health - 5); enemy.AttackTime = .7f; }
		}
		for(SShot & shot : game.Shots) {
			if(!shot.Alive) continue;
			shot.Position.x += shot.Direction.x * 10 * dt;
			shot.Position.y += shot.Direction.y * 10 * dt;
			shot.Age += dt;
			for(SEnemy & enemy : game.Enemies) {
				if(enemy.Health <= 0 || ::distanceSquared(shot.Position, enemy.Position) > .5f) continue;
				enemy.Health = ::llc::max(0, enemy.Health - 10);
				game.Experience += 10;
				shot.Alive = false;
				break;
			}
			if(shot.Age > 3 || shot.Position.x < 1 || shot.Position.x >= MapWidth - 1 || shot.Position.y < 1 || shot.Position.y >= MapHeight - 1)
				shot.Alive = false;
		}
	}

	sttc ::llc::err_t drawGame(SGame const & game, ::llc::g8bgra target) {
		if_fail_fe(target.fill(::llc::SColorBGRA{12, 13, 10}));
		cnst ::llc::s2_t left = ((::llc::s2_t)target.metrics().x - MapWidth * CellSize) / 2;
		cnst ::llc::s2_t top = ((::llc::s2_t)target.metrics().y - MapHeight * CellSize) / 2 + 12;
		for(::llc::s2_t y = 0; y < MapHeight; ++y)
		for(::llc::s2_t x = 0; x < MapWidth; ++x)
			::gpftw_port::rect(target, left + x * CellSize, top + y * CellSize, CellSize - 1, CellSize - 1, game.Floor[y][x] ? ::llc::SColorBGRA{80, 83, 88} : ::llc::SColorBGRA{31, 67, 35});
		for(cnst SEnemy & enemy : game.Enemies)
			if(enemy.Health > 0) ::gpftw_port::rect(target, left + (::llc::s2_t)(enemy.Position.x * CellSize) - 7, top + (::llc::s2_t)(enemy.Position.y * CellSize) - 7, 15, 15, {40, 65, 230});
		for(cnst SShot & shot : game.Shots)
			if(shot.Alive) ::gpftw_port::rect(target, left + (::llc::s2_t)(shot.Position.x * CellSize) - 3, top + (::llc::s2_t)(shot.Position.y * CellSize) - 3, 6, 6, {45, 230, 245});
		::gpftw_port::rect(target, left + (::llc::s2_t)(game.Player.x * CellSize) - 8, top + (::llc::s2_t)(game.Player.y * CellSize) - 8, 17, 17, game.Health > 0 ? ::llc::SColorBGRA{245, 180, 50} : ::llc::SColorBGRA{60, 60, 60});
		::gpftw_port::rect(target, left, top - 14, MapWidth * CellSize, 7, {30, 30, 30});
		::gpftw_port::rect(target, left, top - 14, game.Health * MapWidth * CellSize / 100, 7, {50, 210, 70});
		rtrn 0;
	}
}

stct SApplication {
	::gpftw_port::SHost	Host;
	::SGame				Game = {};
	SApplication(::llc::SRuntimeValues & runtime) : Host(runtime) {}
};

::llc::err_t setup(SApplication & app) {
	if_fail_fe(::gpftw_port::setup(app.Host, "GPFTW - TUTORIAL 9", "WASD MOVE | SHIFT RUN | SPACE FIRE | R RESTART | ESC EXIT"));
	::reset(app.Game);
	rtrn 0;
}

::llc::err_t cleanup(SApplication & app) { rtrn ::llc::mainWindowDestroy(app.Host.Framework.RootWindow); }

::llc::err_t update(SApplication & app, bool systemRequestedExit) {
	if(systemRequestedExit) rtrn ::llc::APPLICATION_STATE_EXIT;
	cnst ::llc::err_t result = ::gpftw_port::update(app.Host);
	if(result) rtrn result;
	::updateGame(app.Game, *app.Host.Framework.RootWindow.Input, (::llc::f2_t)app.Host.Framework.FrameInfo.Seconds.LastFrame);
	::llc::u2_t living = 0;
	for(cnst ::SEnemy & enemy : app.Game.Enemies) living += enemy.Health > 0;
	::snprintf(app.Host.StatusText, ::llc::size(app.Host.StatusText), "HP %d  XP %d  ENEMIES %u  %s", app.Game.Health, app.Game.Experience, living, app.Game.Health <= 0 ? "GAME OVER" : living ? "FIGHT" : "YOU WIN");
	if_fail_fe(::gpftw_port::statusSet(app.Host, ::llc::vcst_t{app.Host.StatusText}));
	::llc::sleep(1);
	rtrn ::llc::guiUpdateMetrics(*app.Host.Framework.GUI, app.Host.Framework.RootWindow.Size, false);
}

::llc::err_t draw(SApplication & app) {
	::llc::SWindow & window = app.Host.Framework.RootWindow;
	if(window.NoDraw) rtrn 0;
	if_fail_fe(window.BackBuffer->resize(window.Size.u2()));
	if_fail_fe(::drawGame(app.Game, window.BackBuffer->Color.View));
	rtrn ::gpftw_port::draw(app.Host, window.BackBuffer->Color.View);
}

LLC_DEFINE_APPLICATION_ENTRY_POINT(SApplication, "LLC GPFTW")
