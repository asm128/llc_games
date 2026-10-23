#include "llc_gontrill.h"

#include "llc_grid_copy.h"
#include "llc_png.h"
#include "llc_size.h"
#include "llc_system_key.h"

#include <cmath>
#include <cstdio>

namespace {
    struct SWeaponSpec {
        ::llc::f2_t       Speed;
        ::llc::f2_t       Delay;
        ::llc::s2_t       Damage;
        ::llc::SColorBGRA Color;
    };

    stacxpr SWeaponSpec WeaponSpecs[] = {
        {  800, .20f, 2500, { 160, 220, 255}}, // Bow
        {  900, .10f, 2000, { 210, 210, 210}}, // Gun
        {10000, .10f, 2000, { 255, 240,  80}}, // Laser
        { 5000, .15f, 4000, { 255, 120, 255}}, // Plasma
        { 2500, .10f, 6000, { 255, 220,  80}}, // Bubble
        { 8000, .10f, 2000, { 255, 255, 120}}, // Spark
        {  800, .10f, 2000, {  80, 255, 120}}, // Poison
        {  100, .05f, 2750, {  40, 140, 255}}, // Fire
        {  100, .05f, 4750, { 120, 120, 160}}, // Rock
        {  250, .05f,  750, { 255, 100, 180}}, // Gravity
    };

    stacxpr ::llc::SColorBGRA Transparent = {0xFF, 0, 0xFF, 0xFF};

    sttc ::llc::u2_t gontrillRandom(::llc::SGontrill & game) nxpt {
        game.RandomState ^= game.RandomState << 13;
        game.RandomState ^= game.RandomState >> 17;
        game.RandomState ^= game.RandomState << 5;
        rtrn game.RandomState;
    }

    sttc ::llc::f2_t gontrillClamp(::llc::f2_t value, ::llc::f2_t low, ::llc::f2_t high) nxpt {
        rtrn value < low ? low : value > high ? high : value;
    }

    sttc ::llc::f2_t gontrillDistanceSquared(::llc::n2f2_c & a, ::llc::n2f2_c & b) nxpt {
        cnst ::llc::f2_t x = a.x - b.x, y = a.y - b.y;
        rtrn x * x + y * y;
    }

    sttc ::llc::f2_t gontrillSegmentDistanceSquared(::llc::n2f2_c & point, ::llc::n2f2_c & from, ::llc::n2f2_c & to) nxpt {
        cnst ::llc::f2_t dx = to.x - from.x, dy = to.y - from.y;
        cnst ::llc::f2_t lengthSquared = dx * dx + dy * dy;
        cnst ::llc::f2_t t = lengthSquared ? ::gontrillClamp(((point.x - from.x) * dx + (point.y - from.y) * dy) / lengthSquared, 0, 1) : 0;
        rtrn ::gontrillDistanceSquared(point, {from.x + dx * t, from.y + dy * t});
    }

    sttc void gontrillParticleAdd(::llc::SGontrill & game, ::llc::n2f2_c & position, ::llc::n2f2_c & velocity, ::llc::SColorBGRA color, ::llc::f2_t life) nxpt {
        for(::llc::SGontrillParticle & particle : game.Particles) {
            if(particle.Age < particle.Life)
                continue;
            particle = {position, velocity, color, 0, life};
            break;
        }
    }

    sttc void gontrillBurst(::llc::SGontrill & game, ::llc::n2f2_c & position, ::llc::SColorBGRA color, ::llc::u0_t count) nxpt {
        for(::llc::u0_t i = 0; i < count; ++i) {
            cnst ::llc::f2_t angle = (::gontrillRandom(game) % 6284) * .001f;
            cnst ::llc::f2_t speed = (::llc::f2_t)(40 + (::gontrillRandom(game) % 240));
            ::gontrillParticleAdd(game, position, {::cosf(angle) * speed, ::sinf(angle) * speed}, color, .18f + (::gontrillRandom(game) % 45) * .01f);
        }
    }

    sttc void gontrillShotAdd(::llc::SGontrill & game, ::llc::n2f2_c & position, ::llc::n2f2_c & direction, ::llc::u0_t weapon, ::llc::u0_t owner, bool enemy) nxpt {
        for(::llc::SGontrillShot & shot : game.Shots) {
            if(shot.Alive)
                continue;
            cnst ::SWeaponSpec & spec = ::WeaponSpecs[weapon % ::llc::size(::WeaponSpecs)];
            shot = {position, position, {direction.x * spec.Speed, direction.y * spec.Speed}, 0, weapon, owner, enemy, true};
            break;
        }
    }

    sttc void gontrillPowerupAdd(::llc::SGontrill & game, ::llc::n2f2_c & position) nxpt {
        for(::llc::SGontrillPowerup & powerup : game.Powerups) {
            if(powerup.Alive)
                continue;
            powerup = {position, (::llc::u0_t)(::gontrillRandom(game) % 4), (::llc::u0_t)(::gontrillRandom(game) % 10), true};
            break;
        }
    }

    sttc void gontrillEnemyAdd(::llc::SGontrill & game, ::llc::n2u2_c & screenSize) nxpt {
        for(::llc::SGontrillEnemy & enemy : game.Enemies) {
            if(enemy.Alive)
                continue;
            cnst ::llc::u2_t edge = ::gontrillRandom(game) % 4;
            cnst ::llc::f2_t along = .15f + (::gontrillRandom(game) % 701) * .001f;
            enemy = {};
            enemy.Position = edge == 0 ? ::llc::n2f2_t{-16, screenSize.y * along}
                : edge == 1 ? ::llc::n2f2_t{screenSize.x + 16.0f, screenSize.y * along}
                : edge == 2 ? ::llc::n2f2_t{screenSize.x * along, -16}
                : ::llc::n2f2_t{screenSize.x * along, screenSize.y + 16.0f};
            enemy.PathStep = (::llc::u0_t)(::gontrillRandom(game) % 10);
            enemy.Alive = true;
            break;
        }
    }

    sttc void gontrillDamage(::llc::s2_t & health, ::llc::s2_t & shield, ::llc::s2_t damage) nxpt {
        cnst ::llc::s2_t shieldPart = ::llc::min(shield, damage / 2);
        shield = ::llc::max(0, shield - shieldPart);
        health = ::llc::max(0, health - (damage - shieldPart));
    }

    sttc void gontrillPixel(::llc::g8bgra target, ::llc::s2_t x, ::llc::s2_t y, ::llc::SColorBGRA color) nxpt {
        if(x >= 0 && y >= 0 && (::llc::u2_t)x < target.metrics().x && (::llc::u2_t)y < target.metrics().y)
            target[y][x] = color;
    }

    sttc void gontrillLine(::llc::g8bgra target, ::llc::n2f2_c & start, ::llc::n2f2_c & end, ::llc::SColorBGRA color) nxpt {
        ::llc::s2_t x0 = (::llc::s2_t)start.x, y0 = (::llc::s2_t)start.y;
        cnst ::llc::s2_t x1 = (::llc::s2_t)end.x, y1 = (::llc::s2_t)end.y;
        cnst ::llc::s2_t dx = ::llc::abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
        cnst ::llc::s2_t dy = -::llc::abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
        ::llc::s2_t error = dx + dy;
        for(::llc::u2_t step = 0; step < 2048; ++step) {
            ::gontrillPixel(target, x0, y0, color);
            if(x0 == x1 && y0 == y1)
                break;
            cnst ::llc::s2_t e2 = error * 2;
            if(e2 >= dy) { error += dy; x0 += sx; }
            if(e2 <= dx) { error += dx; y0 += sy; }
        }
    }

    sttc void gontrillBar(::llc::g8bgra target, ::llc::n2f2_c & center, ::llc::s2_t y, ::llc::s2_t amount, ::llc::SColorBGRA color) nxpt {
        for(::llc::s2_t x = 0; x < 24; ++x)
            ::gontrillPixel(target, (::llc::s2_t)center.x - 12 + x, (::llc::s2_t)center.y + y, x < amount ? color : ::llc::SColorBGRA{28, 28, 28});
    }

    sttc ::llc::err_t gontrillSprite(::llc::g8bgra target, ::llc::img8bgra const & image, ::llc::n2f2_c & center) {
        if(!image.size())
            rtrn 0;
        cnst ::llc::n2s2_t offset = {(::llc::s2_t)center.x - (::llc::s2_t)image.metrics().x / 2, (::llc::s2_t)center.y - (::llc::s2_t)image.metrics().y / 2};
        if(offset.x >= (::llc::s2_t)target.metrics().x || offset.y >= (::llc::s2_t)target.metrics().y
        || offset.x + (::llc::s2_t)image.metrics().x <= 0 || offset.y + (::llc::s2_t)image.metrics().y <= 0)
            rtrn 0;
        rtrn ::llc::grid_copy_alpha(target, image.View, offset, ::Transparent);
    }
}

::llc::err_t llc::gontrillLoad(::llc::SGontrill & game) {
    if(game.ImagesLoaded)
        rtrn 0;
    stacxpr cnst char * names[GONTRILL_SPRITE_COUNT] = {
        "Codepage_437_24_12x12.png", "ship_0.png", "ship_1.png", "pow_core_0.png",
        "pow_core_1.png", "crosshair_template.png", "pow_icon_0.png", "enemy_0.png"
    };
    for(::llc::u0_t i = 0; i < GONTRILL_SPRITE_COUNT; ++i) {
        char filename[128] = {};
        ::snprintf(filename, ::llc::size(filename), "../llc_games_data/images/%s", names[i]);
        if_fail_fef(::llc::pngFileLoad(::llc::vcst_t{filename}, game.Sprites[i]), "Cannot load Gontrill sprite: %s.", filename);
    }
    game.ImagesLoaded = true;
    rtrn 0;
}

::llc::err_t llc::gontrillReset(::llc::SGontrill & game, ::llc::n2u2_c & screenSize) {
    for(::llc::SGontrillEnemy & enemy : game.Enemies) enemy = {};
    for(::llc::SGontrillShot & shot : game.Shots) shot = {};
    for(::llc::SGontrillPowerup & powerup : game.Powerups) powerup = {};
    for(::llc::SGontrillParticle & particle : game.Particles) particle = {};
    game.SpawnTime = game.TotalTime = 0;
    game.Score = 0;
    game.Paused = game.Debugging = game.GameOver = false;
    for(::llc::u0_t i = 0; i < 2; ++i) {
        game.Ships[i] = {};
        game.Ships[i].Position = {screenSize.x * .25f, screenSize.y * .5f + (i ? 42.0f : -42.0f)};
        game.Ships[i].Weapon = i;
    }
    for(::llc::SGontrillStar & star : game.Stars)
        star = {{(::llc::f2_t)(::gontrillRandom(game) % ::llc::max(1u, screenSize.x)), (::llc::f2_t)(::gontrillRandom(game) % ::llc::max(1u, screenSize.y))}, 25.0f + (::gontrillRandom(game) % 175), (::llc::u0_t)(90 + ::gontrillRandom(game) % 166)};
    rtrn 0;
}

::llc::err_t llc::gontrillUpdate(::llc::SGontrill & game, ::llc::SInput const & input, ::llc::f2_t seconds, ::llc::n2u2_c & screenSize) {
    if(input.KeyDown('P')) game.Paused = !game.Paused;
    if(input.KeyDown(::llc::SYSTEM_KEY_F2)) game.Debugging = !game.Debugging;
    if(game.Paused || game.GameOver || !screenSize.x || !screenSize.y)
        rtrn 0;
    cnst ::llc::f2_t dt = ::gontrillClamp(seconds, 0, .05f);
    game.TotalTime += dt;
    for(::llc::u0_t i = 0; i < 2; ++i) {
        ::llc::SGontrillShip & ship = game.Ships[i];
        if(!ship.Alive) continue;
        cnst auto & keys = input.KeyboardCurrent.KeyState;
        ship.Direction = i == 0
            ? ::llc::n2f2_t{(keys['D'] ? 1.0f : 0) - (keys['A'] ? 1.0f : 0), (keys['S'] ? 1.0f : 0) - (keys['W'] ? 1.0f : 0)}
            : ::llc::n2f2_t{(keys[::llc::SYSTEM_KEY_Right] ? 1.0f : 0) - (keys[::llc::SYSTEM_KEY_Left] ? 1.0f : 0), (keys[::llc::SYSTEM_KEY_Down] ? 1.0f : 0) - (keys[::llc::SYSTEM_KEY_Up] ? 1.0f : 0)};
        ship.Direction.InPlaceNormalize();
        ship.Firing = !!keys[i ? ::llc::SYSTEM_KEY_Numpad1 : 'T'];
        ship.Thrust = !!keys[i ? ::llc::SYSTEM_KEY_Numpad2 : 'Y'];
        ship.Brakes = !!keys[i ? ::llc::SYSTEM_KEY_Numpad3 : 'U'];
        cnst ::llc::f2_t speed = ship.Brakes ? 85.0f : ship.Thrust ? 350.0f : 185.0f;
        ship.Position.x = ::gontrillClamp(ship.Position.x + ship.Direction.x * speed * dt, 12, screenSize.x - 12.0f);
        ship.Position.y = ::gontrillClamp(ship.Position.y + ship.Direction.y * speed * dt, 42, screenSize.y - 16.0f);
        ship.ShotDelay -= dt;
        if(ship.Firing && ship.ShotDelay <= 0) {
            ::gontrillShotAdd(game, {ship.Position.x + 12, ship.Position.y}, {1, 0}, ship.Weapon, i, false);
            ship.ShotDelay = ::WeaponSpecs[ship.Weapon].Delay;
        }
        if(ship.Thrust && (::gontrillRandom(game) % 3))
            ::gontrillParticleAdd(game, {ship.Position.x - 11, ship.Position.y}, {-90.0f - (::gontrillRandom(game) % 160), (::llc::f2_t)((::llc::s2_t)(::gontrillRandom(game) % 100) - 50)}, {45, 140, 255}, .20f);
    }
    for(::llc::u0_t weapon = 0; weapon < ::llc::size(::WeaponSpecs); ++weapon)
        if(input.KeyDown((::llc::u0_t)('0' + weapon))) { game.Ships[0].Weapon = weapon; break; }

    game.SpawnTime += dt;
    if(game.SpawnTime >= 5.5f) { game.SpawnTime -= 5.5f; ::gontrillEnemyAdd(game, screenSize); }
    stacxpr ::llc::n2f2_t path[10] = {{.03f,.04f},{.50f,.33f},{.08f,.37f},{.76f,.03f},{.23f,.04f},{.28f,.59f},{.31f,.09f},{.03f,.88f},{.08f,.09f},{.03f,.22f}};
    for(::llc::SGontrillEnemy & enemy : game.Enemies) {
        if(!enemy.Alive) continue;
        enemy.Age += dt;
        cnst ::llc::n2f2_t target = {path[enemy.PathStep].x * screenSize.x, path[enemy.PathStep].y * screenSize.y};
        ::llc::n2f2_t direction = {target.x - enemy.Position.x, target.y - enemy.Position.y};
        if(direction.LengthSquared() < 100) enemy.PathStep = (::llc::u0_t)((enemy.PathStep + 1) % ::llc::size(path));
        else { direction.InPlaceNormalize(); enemy.Position.x += direction.x * 100 * dt; enemy.Position.y += direction.y * 100 * dt; }
        enemy.ShotDelay -= dt;
        if(enemy.ShotDelay <= 0) {
            cnst ::llc::u0_t targetShip = game.Ships[::gontrillRandom(game) % 2].Alive ? (::llc::u0_t)(::gontrillRandom(game) % 2) : (game.Ships[0].Alive ? 0 : 1);
            ::llc::n2f2_t aim = {game.Ships[targetShip].Position.x - enemy.Position.x, game.Ships[targetShip].Position.y - enemy.Position.y};
            aim.InPlaceNormalize();
            ::gontrillShotAdd(game, enemy.Position, aim, 1, 0, true);
            enemy.ShotDelay = 1.5f;
        }
    }

    for(::llc::SGontrillShot & shot : game.Shots) {
        if(!shot.Alive) continue;
        shot.Previous = shot.Position;
        shot.Position.x += shot.Velocity.x * dt;
        shot.Position.y += shot.Velocity.y * dt;
        shot.Age += dt;
        cnst ::llc::s2_t damage = ::WeaponSpecs[shot.Weapon].Damage;
        if(shot.Enemy) {
            for(::llc::SGontrillShip & ship : game.Ships) {
                if(!ship.Alive || ::gontrillSegmentDistanceSquared(ship.Position, shot.Previous, shot.Position) > 14 * 14) continue;
                ::gontrillDamage(ship.Health, ship.Shield, damage);
                ::gontrillBurst(game, shot.Position, {40, 120, 255}, 8);
                shot.Alive = false;
                if(!ship.Health) { ship.Alive = false; ::gontrillBurst(game, ship.Position, {40, 120, 255}, 32); }
                break;
            }
        }
        else {
            for(::llc::SGontrillEnemy & enemy : game.Enemies) {
                if(!enemy.Alive || ::gontrillSegmentDistanceSquared(enemy.Position, shot.Previous, shot.Position) > 14 * 14) continue;
                ::gontrillDamage(enemy.Health, enemy.Shield, damage);
                ::gontrillBurst(game, shot.Position, ::WeaponSpecs[shot.Weapon].Color, 8);
                shot.Alive = false;
                if(!enemy.Health) {
                    enemy.Alive = false; game.Score += 100;
                    ::gontrillBurst(game, enemy.Position, {40, 180, 255}, 32);
                    if(::gontrillRandom(game) & 1) ::gontrillPowerupAdd(game, enemy.Position);
                }
                break;
            }
        }
        if(shot.Age > 3 || shot.Position.x < -50 || shot.Position.y < -50 || shot.Position.x > screenSize.x + 50 || shot.Position.y > screenSize.y + 50)
            shot.Alive = false;
    }
    for(::llc::SGontrillPowerup & powerup : game.Powerups) {
        if(!powerup.Alive) continue;
        for(::llc::SGontrillShip & ship : game.Ships) {
            if(!ship.Alive || ::gontrillDistanceSquared(ship.Position, powerup.Position) > 22 * 22) continue;
            switch(powerup.Family) {
            case 0: ship.Weapon = powerup.Type % ::llc::size(::WeaponSpecs); break;
            case 1: ship.Health = ::llc::min(5000, ship.Health + 1500); break;
            case 2: ship.Shield = ::llc::min(5000, ship.Shield + 1500); break;
            default: game.Score += 250; break;
            }
            powerup.Alive = false;
            ::gontrillBurst(game, powerup.Position, {80, 255, 180}, 16);
            break;
        }
    }
    for(::llc::SGontrillParticle & particle : game.Particles) {
        if(particle.Age >= particle.Life) continue;
        particle.Age += dt;
        particle.Position.x += particle.Velocity.x * dt;
        particle.Position.y += particle.Velocity.y * dt;
    }
    for(::llc::SGontrillStar & star : game.Stars) {
        star.Position.x -= star.Speed * dt;
        if(star.Position.x < 0) { star.Position.x = (::llc::f2_t)screenSize.x; star.Position.y = (::llc::f2_t)(::gontrillRandom(game) % screenSize.y); }
    }
    game.GameOver = !game.Ships[0].Alive && !game.Ships[1].Alive;
    rtrn 0;
}

::llc::err_t llc::gontrillDraw(::llc::SGontrill const & game, ::llc::g8bgra target) {
    if_fail_fe(target.fill(::llc::SColorBGRA{12, 10, 8}));
    for(::llc::SGontrillStar const & star : game.Stars)
        ::gontrillPixel(target, (::llc::s2_t)star.Position.x, (::llc::s2_t)star.Position.y, {star.Brightness, star.Brightness, star.Brightness});
    for(::llc::SGontrillPowerup const & powerup : game.Powerups) {
        if(!powerup.Alive) continue;
        cnst ::llc::u0_t sprite = powerup.Family & 1 ? GONTRILL_SPRITE_POWER_DIAGONAL : GONTRILL_SPRITE_POWER_SQUARE;
        if_fail_fe(::gontrillSprite(target, game.Sprites[sprite], powerup.Position));
        if_fail_fe(::gontrillSprite(target, game.Sprites[GONTRILL_SPRITE_POWER_ICON], powerup.Position));
    }
    for(::llc::SGontrillEnemy const & enemy : game.Enemies) {
        if(!enemy.Alive) continue;
        for(::llc::u0_t i = 0; i < 5; ++i) {
            cnst ::llc::f2_t angle = game.TotalTime + i * 1.256637f;
            cnst ::llc::n2f2_t ghost = {enemy.Position.x + ::cosf(angle) * 24, enemy.Position.y + ::sinf(angle) * 24};
            if_fail_fe(::gontrillSprite(target, game.Sprites[GONTRILL_SPRITE_ENEMY], ghost));
        }
        if_fail_fe(::gontrillSprite(target, game.Sprites[GONTRILL_SPRITE_ENEMY], enemy.Position));
        ::gontrillBar(target, enemy.Position, -20, enemy.Shield * 24 / 5000, {255, 200, 40});
        ::gontrillBar(target, enemy.Position, -18, enemy.Health * 24 / 5000, {40, 220, 80});
    }
    for(::llc::u0_t i = 0; i < 2; ++i) {
        ::llc::SGontrillShip const & ship = game.Ships[i];
        if(!ship.Alive) continue;
        if_fail_fe(::gontrillSprite(target, game.Sprites[GONTRILL_SPRITE_SHIP0 + i], ship.Position));
        ::gontrillBar(target, ship.Position, 16, ship.Shield * 24 / 5000, {255, 200, 40});
        ::gontrillBar(target, ship.Position, 18, ship.Health * 24 / 5000, {40, 220, 80});
        cnst ::llc::n2f2_t crosshair = {ship.Position.x + 96, ship.Position.y};
        if_fail_fe(::gontrillSprite(target, game.Sprites[GONTRILL_SPRITE_CROSSHAIR], crosshair));
    }
    for(::llc::SGontrillShot const & shot : game.Shots)
        if(shot.Alive) ::gontrillLine(target, shot.Previous, shot.Position, shot.Enemy ? ::llc::SColorBGRA{30, 90, 255} : ::WeaponSpecs[shot.Weapon].Color);
    for(::llc::SGontrillParticle const & particle : game.Particles)
        if(particle.Age < particle.Life) ::gontrillPixel(target, (::llc::s2_t)particle.Position.x, (::llc::s2_t)particle.Position.y, particle.Color);
    rtrn 0;
}
