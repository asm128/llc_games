#include "../llc_gpftw_port.h"
#include "llc_app_impl.h"

#include <cmath>

namespace {
	struct SWeapon {
		::llc::f2_t	Speed;
		::llc::f2_t	Delay;
		::llc::s2_t	Damage;
		::llc::SColorBGRA Color;
	};
	stacxpr SWeapon Weapons[] = {
		{200, .32f, 10, {100, 110, 130}}, // Rock
		{700, .24f, 20, {220, 220, 220}}, // Arrow
		{500, .28f, 30, {70, 110, 255}}, // Fireball
		{1000,.20f, 40, {80, 255, 255}}, // Laser
		{300, .26f, 50, {75, 210, 90}}, // Poison
		{850, .16f, 40, {210, 210, 210}}, // Bullet
		{800, .22f, 60, {230, 240, 80}}, // Plasma
		{170, .42f, 70, {50, 55, 190}}, // Bomb
	};

	struct SShip {
		::llc::n2f2_t Position = {};
		::llc::n2f2_t Velocity = {};
		::llc::s2_t Health = 200;
		::llc::s2_t Shield = 200;
		::llc::f2_t Age = 0;
		::llc::f2_t FireTime = 0;
		bool Alive = false;
	};
	struct SShot {
		::llc::n2f2_t Position = {}, Previous = {}, Velocity = {};
		::llc::f2_t Age = 0;
		::llc::u0_t Weapon = 0;
		bool Enemy = false, Alive = false;
	};
	struct SParticle {
		::llc::n2f2_t Position = {}, Velocity = {};
		::llc::f2_t Life = 0;
		::llc::SColorBGRA Color = {};
	};
	struct SGame {
		SShip Player = {};
		SShip Enemies[24] = {};
		SShot Shots[256] = {};
		SParticle Particles[128] = {};
		::llc::u2_t Random = 0x315A2F19u;
		::llc::u2_t Score = 0;
		::llc::u0_t Weapon = 0;
		::llc::f2_t SpawnTime = 0;
	};

	sttc ::llc::u2_t random(SGame & game) nxpt {
		game.Random ^= game.Random << 13;
		game.Random ^= game.Random >> 17;
		game.Random ^= game.Random << 5;
		rtrn game.Random;
	}
	sttc void reset(SGame & game) {
		game = {};
		game.Player = {{170, 300}, {}, 200, 200, 0, 0, true};
		game.SpawnTime = 2.2f;
	}
	sttc ::llc::f2_t distanceSquared(::llc::n2f2_c & a, ::llc::n2f2_c & b) nxpt {
		cnst ::llc::f2_t x = a.x - b.x, y = a.y - b.y;
		rtrn x * x + y * y;
	}
	sttc ::llc::f2_t segmentDistanceSquared(::llc::n2f2_c & p, ::llc::n2f2_c & a, ::llc::n2f2_c & b) nxpt {
		cnst ::llc::f2_t dx = b.x - a.x, dy = b.y - a.y, length = dx * dx + dy * dy;
		cnst ::llc::f2_t fraction = length ? ::llc::max(0.0f, ::llc::min(1.0f, ((p.x - a.x) * dx + (p.y - a.y) * dy) / length)) : 0;
		rtrn ::distanceSquared(p, {a.x + dx * fraction, a.y + dy * fraction});
	}
	sttc void particleAdd(SGame & game, ::llc::n2f2_c & p, ::llc::SColorBGRA color) {
		for(SParticle & particle : game.Particles) {
			if(particle.Life > 0) continue;
			particle = {p, {((::llc::s2_t)(::random(game) % 160) - 80) * 1.0f, ((::llc::s2_t)(::random(game) % 160) - 80) * 1.0f}, .35f, color};
			break;
		}
	}
	sttc void shotAdd(SGame & game, ::llc::n2f2_c & p, ::llc::n2f2_c & direction, ::llc::u0_t weapon, bool enemy) {
		for(SShot & shot : game.Shots) {
			if(shot.Alive) continue;
			cnst SWeapon & spec = Weapons[weapon];
			shot = {p, p, {direction.x * spec.Speed, direction.y * spec.Speed}, 0, weapon, enemy, true};
			break;
		}
	}
	sttc void spawn(SGame & game) {
		for(SShip & enemy : game.Enemies) {
			if(enemy.Alive) continue;
			enemy = {{760, 105.0f + (::random(game) % 390)}, {}, 80, 80, 0, 0, true};
			break;
		}
	}
	sttc void damage(SShip & ship, ::llc::s2_t value) {
		cnst ::llc::s2_t shieldDamage = ::llc::min(ship.Shield, value * 2 / 3);
		ship.Shield -= shieldDamage;
		ship.Health = ::llc::max(0, ship.Health - (value - shieldDamage));
		ship.Alive = ship.Health > 0;
	}
	sttc void updateGame(SGame & game, ::llc::SInput const & input, ::llc::f2_t elapsed) {
		if(input.KeyDown('R')) ::reset(game);
		if(!game.Player.Alive) rtrn;
		cnst ::llc::f2_t dt = ::llc::max(0.0f, ::llc::min(.05f, elapsed));
		cnst auto & keys = input.KeyboardCurrent.KeyState;
		::llc::n2f2_t direction = {(keys['D'] ? 1.0f : 0) - (keys['A'] ? 1.0f : 0), (keys['S'] ? 1.0f : 0) - (keys['W'] ? 1.0f : 0)};
		direction.InPlaceNormalize();
		cnst ::llc::f2_t thrust = keys[::llc::SYSTEM_KEY_Shift] ? 1800.0f : 900.0f;
		game.Player.Velocity.x = (game.Player.Velocity.x + direction.x * thrust * dt) * .94f;
		game.Player.Velocity.y = (game.Player.Velocity.y + direction.y * thrust * dt) * .94f;
		game.Player.Position.x = ::llc::max(24.0f, ::llc::min(776.0f, game.Player.Position.x + game.Player.Velocity.x * dt));
		game.Player.Position.y = ::llc::max(90.0f, ::llc::min(510.0f, game.Player.Position.y + game.Player.Velocity.y * dt));
		if(direction.LengthSquared() > 0 && (::random(game) % 3) == 0)
			::particleAdd(game, {game.Player.Position.x - 14, game.Player.Position.y}, {75, 155, 255});
		for(::llc::u0_t i = 0; i < ::llc::size(Weapons); ++i)
			if(input.KeyDown((::llc::u0_t)('1' + i))) game.Weapon = i;
		game.Player.FireTime -= dt;
		if(keys[::llc::SYSTEM_KEY_Space] && game.Player.FireTime <= 0) {
			::shotAdd(game, {game.Player.Position.x + 16, game.Player.Position.y}, {1, 0}, game.Weapon, false);
			game.Player.FireTime = Weapons[game.Weapon].Delay;
		}
		game.SpawnTime += dt;
		if(game.SpawnTime >= 3.0f) { game.SpawnTime -= 3.0f; ::spawn(game); }
		for(SShip & enemy : game.Enemies) {
			if(!enemy.Alive) continue;
			enemy.Age += dt;
			enemy.Position.x -= 48 * dt;
			enemy.Position.y += ::sinf(enemy.Age * 2) * 18 * dt;
			if(enemy.Position.x < -20) { enemy.Alive = false; continue; }
			enemy.FireTime -= dt;
			if(enemy.FireTime <= 0) {
				::llc::n2f2_t aim = {game.Player.Position.x - enemy.Position.x, game.Player.Position.y - enemy.Position.y};
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
				if(::segmentDistanceSquared(game.Player.Position, shot.Previous, shot.Position) < 14 * 14) {
					::damage(game.Player, Weapons[shot.Weapon].Damage);
					::particleAdd(game, shot.Position, {45, 120, 255});
					shot.Alive = false;
				}
			}
			else for(SShip & enemy : game.Enemies) {
				if(!enemy.Alive || ::segmentDistanceSquared(enemy.Position, shot.Previous, shot.Position) >= 14 * 14) continue;
				::damage(enemy, Weapons[shot.Weapon].Damage);
				::particleAdd(game, shot.Position, Weapons[shot.Weapon].Color);
				shot.Alive = false;
				if(!enemy.Alive) game.Score += 100;
				break;
			}
			if(shot.Age > 3 || shot.Position.x < -20 || shot.Position.x > 820 || shot.Position.y < 60 || shot.Position.y > 540)
				shot.Alive = false;
		}
		for(SParticle & particle : game.Particles) {
			if(particle.Life <= 0) continue;
			particle.Position.x += particle.Velocity.x * dt;
			particle.Position.y += particle.Velocity.y * dt;
			particle.Life -= dt;
		}
	}
	sttc ::llc::err_t drawGame(SGame const & game, ::llc::g8bgra target) {
		if_fail_fe(target.fill(::llc::SColorBGRA{10, 9, 18}));
		::gpftw_port::rect(target, 15, 70, 770, 460, {24, 20, 35});
		for(::llc::u0_t y = 0; y < 46; ++y)
		for(::llc::u0_t x = 0; x < 77; ++x)
			if(((x * 73 + y * 137) % 97) == 0) ::gpftw_port::rect(target, 15 + x * 10, 70 + y * 10, 2, 2, {85, 90, 110});
		for(cnst SParticle & particle : game.Particles)
			if(particle.Life > 0) ::gpftw_port::rect(target, (::llc::s2_t)particle.Position.x, (::llc::s2_t)particle.Position.y, 3, 3, particle.Color);
		for(cnst SShip & enemy : game.Enemies) {
			if(!enemy.Alive) continue;
			::gpftw_port::rect(target, (::llc::s2_t)enemy.Position.x - 11, (::llc::s2_t)enemy.Position.y - 8, 22, 16, {55, 70, 215});
			::gpftw_port::rect(target, (::llc::s2_t)enemy.Position.x - 6, (::llc::s2_t)enemy.Position.y - 13, 12, 5, {70, 130, 255});
		}
		for(cnst SShot & shot : game.Shots)
			if(shot.Alive) ::gpftw_port::rect(target, (::llc::s2_t)shot.Position.x - 3, (::llc::s2_t)shot.Position.y - 2, 8, 4, shot.Enemy ? ::llc::SColorBGRA{50, 130, 255} : Weapons[shot.Weapon].Color);
		if(game.Player.Alive) {
			::gpftw_port::rect(target, (::llc::s2_t)game.Player.Position.x - 13, (::llc::s2_t)game.Player.Position.y - 8, 26, 16, {255, 175, 45});
			::gpftw_port::rect(target, (::llc::s2_t)game.Player.Position.x + 8, (::llc::s2_t)game.Player.Position.y - 4, 13, 8, {255, 245, 120});
		}
		::gpftw_port::rect(target, 22, 535, 250, 7, {35, 35, 35});
		::gpftw_port::rect(target, 22, 535, game.Player.Health * 250 / 200, 7, {50, 205, 70});
		::gpftw_port::rect(target, 22, 545, game.Player.Shield * 250 / 200, 5, {250, 190, 45});
		rtrn 0;
	}
}

stct SApplication {
	::gpftw_port::SHost Host;
	::SGame Game = {};
	SApplication(::llc::SRuntimeValues & runtime) : Host(runtime) {}
};

::llc::err_t setup(SApplication & app) {
	if_fail_fe(::gpftw_port::setup(app.Host, "GPFTW ADVANCED - TUTORIAL 6", "WASD THRUST | SHIFT BOOST | SPACE FIRE | 1-8 WEAPON | R RESTART | ESC EXIT"));
	::reset(app.Game);
	rtrn 0;
}
::llc::err_t cleanup(SApplication & app) { rtrn ::llc::mainWindowDestroy(app.Host.Framework.RootWindow); }
::llc::err_t update(SApplication & app, bool systemRequestedExit) {
	if(systemRequestedExit) rtrn ::llc::APPLICATION_STATE_EXIT;
	cnst ::llc::err_t result = ::gpftw_port::update(app.Host);
	if(result) rtrn result;
	::updateGame(app.Game, *app.Host.Framework.RootWindow.Input, (::llc::f2_t)app.Host.Framework.FrameInfo.Seconds.LastFrame);
	::snprintf(app.Host.StatusText, ::llc::size(app.Host.StatusText), "SCORE %u  HEALTH %d  SHIELD %d  WEAPON %u  %s", app.Game.Score, app.Game.Player.Health, app.Game.Player.Shield, app.Game.Weapon + 1, app.Game.Player.Alive ? "FIGHT" : "GAME OVER");
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

LLC_DEFINE_APPLICATION_ENTRY_POINT(SApplication, "LLC GPFTW Advanced")
