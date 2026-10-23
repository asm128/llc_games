#include "../llc_gpftw_port.h"
#include "llc_app_impl.h"

#include <cmath>

namespace {
	struct SWeapon {
		::llc::f2_t Speed, Delay;
		::llc::s2_t Damage;
		::llc::SColorBGRA Color;
	};
	stacxpr SWeapon Weapons[] = {
		{ 800, .20f, 1500, {180, 220, 255}}, // Bow
		{ 900, .10f, 1000, {220, 220, 220}}, // Gun
		{1000, .10f, 1000, { 70, 245, 255}}, // Laser
		{ 900, .15f, 1500, {230, 130, 255}}, // Plasma
		{1000, .10f, 1000, {255, 215,  80}}, // Bubble
		{1000, .10f, 1000, {160, 255, 120}}, // Spark
		{ 800, .10f, 1000, { 80, 210, 110}}, // Poison
		{ 700, .05f,  750, { 60, 140, 255}}, // Fire
		{ 100, .05f,  750, {120, 120, 140}}, // Rock
		{ 250, .05f,  750, {255, 100, 180}}, // Gravity
	};
	struct SShip {
		::llc::n2f2_t Position = {};
		::llc::s2_t Health = 5000, Shield = 5000;
		::llc::f2_t FireTime = 0;
		::llc::u0_t Weapon = 0;
		bool Alive = true;
	};
	struct SEnemy {
		::llc::n2f2_t Position = {};
		::llc::s2_t Health = 5000, Shield = 5000;
		::llc::f2_t Age = 0, FireTime = 0;
		bool Alive = false;
	};
	struct SShot {
		::llc::n2f2_t Position = {}, Previous = {}, Velocity = {};
		::llc::f2_t Age = 0;
		::llc::u0_t Weapon = 0;
		bool Enemy = false, Alive = false;
	};
	struct SPowerup {
		::llc::n2f2_t Position = {};
		::llc::u0_t Family = 0, Weapon = 0;
		bool Alive = false;
	};
	struct SGame {
		SShip Ships[2] = {};
		SEnemy Enemies[32] = {};
		SShot Shots[512] = {};
		SPowerup Powerups[32] = {};
		::llc::u2_t Random = 0xA51E37D9u, Score = 0;
		::llc::f2_t Time = 0, SpawnTime = 0;
		bool Paused = false;
	};
	sttc ::llc::u2_t random(SGame & game) nxpt {
		game.Random ^= game.Random << 13;
		game.Random ^= game.Random >> 17;
		game.Random ^= game.Random << 5;
		rtrn game.Random;
	}
	sttc void reset(SGame & game) {
		game = {};
		game.Ships[0].Position = {190, 250};
		game.Ships[1].Position = {190, 350};
		game.Ships[1].Weapon = 1;
		game.SpawnTime = 2.4f;
	}
	sttc ::llc::f2_t distanceSquared(::llc::n2f2_c & a, ::llc::n2f2_c & b) nxpt {
		cnst ::llc::f2_t dx = a.x - b.x, dy = a.y - b.y;
		rtrn dx * dx + dy * dy;
	}
	sttc ::llc::f2_t segmentDistanceSquared(::llc::n2f2_c & point, ::llc::n2f2_c & a, ::llc::n2f2_c & b) nxpt {
		cnst ::llc::f2_t dx = b.x - a.x, dy = b.y - a.y, length = dx * dx + dy * dy;
		cnst ::llc::f2_t fraction = length ? ::llc::max(0.0f, ::llc::min(1.0f, ((point.x - a.x) * dx + (point.y - a.y) * dy) / length)) : 0;
		rtrn ::distanceSquared(point, {a.x + dx * fraction, a.y + dy * fraction});
	}
	sttc void damage(::llc::s2_t & health, ::llc::s2_t & shield, ::llc::s2_t value) {
		cnst ::llc::s2_t shieldPart = ::llc::min(shield, value * 2 / 3);
		shield -= shieldPart;
		health = ::llc::max(0, health - (value - shieldPart));
	}
	sttc void shotAdd(SGame & game, ::llc::n2f2_c & position, ::llc::n2f2_c & direction, ::llc::u0_t weapon, bool enemy) {
		for(SShot & shot : game.Shots) {
			if(shot.Alive) continue;
			shot = {position, position, {direction.x * Weapons[weapon].Speed, direction.y * Weapons[weapon].Speed}, 0, weapon, enemy, true};
			break;
		}
	}
	sttc void enemyAdd(SGame & game) {
		for(SEnemy & enemy : game.Enemies) {
			if(enemy.Alive) continue;
			enemy = {{770, 105.0f + (::random(game) % 385)}, 5000, 5000, 0, .5f, true};
			break;
		}
	}
	sttc void powerupAdd(SGame & game, ::llc::n2f2_c & position) {
		for(SPowerup & powerup : game.Powerups) {
			if(powerup.Alive) continue;
			powerup = {position, (::llc::u0_t)(::random(game) % 4), (::llc::u0_t)(::random(game) % ::llc::size(Weapons)), true};
			break;
		}
	}
	sttc void updateGame(SGame & game, ::llc::SInput const & input, ::llc::f2_t elapsed) {
		if(input.KeyDown('R')) ::reset(game);
		if(input.KeyDown('P')) game.Paused = !game.Paused;
		if(game.Paused || (!game.Ships[0].Alive && !game.Ships[1].Alive)) rtrn;
		cnst ::llc::f2_t dt = ::llc::max(0.0f, ::llc::min(.05f, elapsed));
		cnst auto & keys = input.KeyboardCurrent.KeyState;
		game.Time += dt;
		for(::llc::u0_t i = 0; i < 2; ++i) {
			SShip & ship = game.Ships[i];
			if(!ship.Alive) continue;
			::llc::n2f2_t direction = i
				? ::llc::n2f2_t{(keys[::llc::SYSTEM_KEY_Right] ? 1.0f : 0) - (keys[::llc::SYSTEM_KEY_Left] ? 1.0f : 0), (keys[::llc::SYSTEM_KEY_Down] ? 1.0f : 0) - (keys[::llc::SYSTEM_KEY_Up] ? 1.0f : 0)}
				: ::llc::n2f2_t{(keys['D'] ? 1.0f : 0) - (keys['A'] ? 1.0f : 0), (keys['S'] ? 1.0f : 0) - (keys['W'] ? 1.0f : 0)};
			direction.InPlaceNormalize();
			cnst bool boost = !!keys[i ? ::llc::SYSTEM_KEY_Numpad2 : 'Y'];
			cnst bool brake = !!keys[i ? ::llc::SYSTEM_KEY_Numpad3 : 'U'];
			cnst ::llc::f2_t speed = brake ? 90.0f : boost ? 340.0f : 190.0f;
			ship.Position.x = ::llc::max(22.0f, ::llc::min(778.0f, ship.Position.x + direction.x * speed * dt));
			ship.Position.y = ::llc::max(95.0f, ::llc::min(505.0f, ship.Position.y + direction.y * speed * dt));
			ship.FireTime -= dt;
			if(keys[i ? ::llc::SYSTEM_KEY_Numpad1 : 'T'] && ship.FireTime <= 0) {
				::shotAdd(game, {ship.Position.x + 15, ship.Position.y}, {1, 0}, ship.Weapon, false);
				ship.FireTime = Weapons[ship.Weapon].Delay;
			}
		}
		for(::llc::u0_t weapon = 0; weapon < ::llc::size(Weapons); ++weapon)
			if(input.KeyDown((::llc::u0_t)('0' + weapon))) game.Ships[0].Weapon = weapon;
		game.SpawnTime += dt;
		if(game.SpawnTime >= 3.0f) { game.SpawnTime -= 3.0f; ::enemyAdd(game); }
		for(SEnemy & enemy : game.Enemies) {
			if(!enemy.Alive) continue;
			enemy.Age += dt;
			enemy.Position.x -= 55 * dt;
			enemy.Position.y += ::sinf(enemy.Age * 2) * 24 * dt;
			if(enemy.Position.x < -20) { enemy.Alive = false; continue; }
			enemy.FireTime -= dt;
			if(enemy.FireTime <= 0) {
				cnst ::llc::u0_t target = game.Ships[0].Alive ? 0 : 1;
				::llc::n2f2_t aim = {game.Ships[target].Position.x - enemy.Position.x, game.Ships[target].Position.y - enemy.Position.y};
				aim.InPlaceNormalize();
				::shotAdd(game, enemy.Position, aim, 1, true);
				enemy.FireTime = 1.2f;
			}
		}
		for(SShot & shot : game.Shots) {
			if(!shot.Alive) continue;
			shot.Previous = shot.Position;
			shot.Position.x += shot.Velocity.x * dt;
			shot.Position.y += shot.Velocity.y * dt;
			shot.Age += dt;
			if(shot.Enemy) {
				for(SShip & ship : game.Ships) {
					if(!ship.Alive || ::segmentDistanceSquared(ship.Position, shot.Previous, shot.Position) >= 14 * 14) continue;
					::damage(ship.Health, ship.Shield, Weapons[shot.Weapon].Damage);
					ship.Alive = ship.Health > 0;
					shot.Alive = false;
					break;
				}
			}
			else for(SEnemy & enemy : game.Enemies) {
				if(!enemy.Alive || ::segmentDistanceSquared(enemy.Position, shot.Previous, shot.Position) >= 14 * 14) continue;
				::damage(enemy.Health, enemy.Shield, Weapons[shot.Weapon].Damage);
				shot.Alive = false;
				if(enemy.Health <= 0) {
					enemy.Alive = false;
					game.Score += 100;
					if(::random(game) & 1) ::powerupAdd(game, enemy.Position);
				}
				break;
			}
			if(shot.Age > 3 || shot.Position.x < -20 || shot.Position.x > 820 || shot.Position.y < 60 || shot.Position.y > 540) shot.Alive = false;
		}
		for(SPowerup & powerup : game.Powerups) {
			if(!powerup.Alive) continue;
			powerup.Position.x -= 22 * dt;
			if(powerup.Position.x < 0) { powerup.Alive = false; continue; }
			for(SShip & ship : game.Ships) {
				if(!ship.Alive || ::distanceSquared(ship.Position, powerup.Position) >= 20 * 20) continue;
				switch(powerup.Family) {
				case 0: ship.Weapon = powerup.Weapon; break;
				case 1: ship.Health = ::llc::min(5000, ship.Health + 1500); break;
				case 2: ship.Shield = ::llc::min(5000, ship.Shield + 1500); break;
				default:
					for(SEnemy & enemy : game.Enemies) if(enemy.Alive) { enemy.Alive = false; game.Score += 100; }
					break;
				}
				powerup.Alive = false;
				break;
			}
		}
	}
	sttc ::llc::err_t drawGame(SGame const & game, ::llc::g8bgra target) {
		if_fail_fe(target.fill(::llc::SColorBGRA{8, 8, 12}));
		::gpftw_port::rect(target, 10, 70, 780, 460, {15, 12, 22});
		for(::llc::s2_t i = 0; i < 180; ++i) {
			cnst ::llc::s2_t x = (i * 347 + 19 - (::llc::s2_t)(game.Time * (15 + i % 5))) % 780;
			cnst ::llc::s2_t y = 76 + (i * 151) % 440;
			::gpftw_port::rect(target, 10 + (x < 0 ? x + 780 : x), y, 2, 2, {100, 100, 110});
		}
		for(cnst SPowerup & powerup : game.Powerups) {
			if(!powerup.Alive) continue;
			::gpftw_port::rect(target, (::llc::s2_t)powerup.Position.x - 9, (::llc::s2_t)powerup.Position.y - 9, 18, 18, {40, 170, 220});
			::gpftw_port::rect(target, (::llc::s2_t)powerup.Position.x - 4, (::llc::s2_t)powerup.Position.y - 4, 8, 8, {130, 255, 255});
		}
		for(cnst SEnemy & enemy : game.Enemies) {
			if(!enemy.Alive) continue;
			cnst ::llc::s2_t x = (::llc::s2_t)enemy.Position.x, y = (::llc::s2_t)enemy.Position.y;
			::gpftw_port::rect(target, x - 12, y - 7, 24, 14, {70, 70, 215});
			::gpftw_port::rect(target, x - 6, y - 12, 12, 24, {65, 100, 255});
			::gpftw_port::rect(target, x - 12, y - 17, enemy.Health * 24 / 5000, 3, {55, 210, 75});
		}
		for(cnst SShip & ship : game.Ships) {
			if(!ship.Alive) continue;
			cnst ::llc::s2_t x = (::llc::s2_t)ship.Position.x, y = (::llc::s2_t)ship.Position.y;
			::gpftw_port::rect(target, x - 12, y - 7, 24, 14, {255, 175, 50});
			::gpftw_port::rect(target, x + 8, y - 3, 13, 6, {255, 245, 130});
			::gpftw_port::rect(target, x - 12, y + 15, ship.Health * 24 / 5000, 3, {60, 210, 75});
			::gpftw_port::rect(target, x - 12, y + 19, ship.Shield * 24 / 5000, 3, {230, 195, 45});
		}
		for(cnst SShot & shot : game.Shots)
			if(shot.Alive) ::gpftw_port::rect(target, (::llc::s2_t)shot.Position.x - 3, (::llc::s2_t)shot.Position.y - 2, 7, 4, shot.Enemy ? ::llc::SColorBGRA{65, 100, 255} : Weapons[shot.Weapon].Color);
		rtrn 0;
	}
}

stct SApplication {
	::gpftw_port::SHost Host;
	::SGame Game = {};
	SApplication(::llc::SRuntimeValues & runtime) : Host(runtime) {}
};
::llc::err_t setup(SApplication & app) {
	if_fail_fe(::gpftw_port::setup(app.Host, "GPFTW PROFESSIONAL - TUTORIAL 8", "P1 WASD + T/Y/U | P2 ARROWS + NUM 1/2/3 | 0-9 WEAPON | P PAUSE | R RESTART"));
	::reset(app.Game);
	rtrn 0;
}
::llc::err_t cleanup(SApplication & app) { rtrn ::llc::mainWindowDestroy(app.Host.Framework.RootWindow); }
::llc::err_t update(SApplication & app, bool systemRequestedExit) {
	if(systemRequestedExit) rtrn ::llc::APPLICATION_STATE_EXIT;
	cnst ::llc::err_t result = ::gpftw_port::update(app.Host);
	if(result) rtrn result;
	::updateGame(app.Game, *app.Host.Framework.RootWindow.Input, (::llc::f2_t)app.Host.Framework.FrameInfo.Seconds.LastFrame);
	::snprintf(app.Host.StatusText, ::llc::size(app.Host.StatusText), "SCORE %u  P1 %d/%d  P2 %d/%d  %s", app.Game.Score, app.Game.Ships[0].Health, app.Game.Ships[0].Shield, app.Game.Ships[1].Health, app.Game.Ships[1].Shield, app.Game.Paused ? "PAUSED" : (!app.Game.Ships[0].Alive && !app.Game.Ships[1].Alive) ? "GAME OVER" : "FIGHT");
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

LLC_DEFINE_APPLICATION_ENTRY_POINT(SApplication, "LLC GPFTW Professional")
