#include "llc_img_color.h"
#include "llc_grid_color.h"
#include "llc_input.h"

#ifndef LLC_GONTRILL_H_23627
#define LLC_GONTRILL_H_23627

namespace llc {
    enum GONTRILL_SPRITE : u0_t {
        GONTRILL_SPRITE_FONT, GONTRILL_SPRITE_SHIP0, GONTRILL_SPRITE_SHIP1,
        GONTRILL_SPRITE_POWER_SQUARE, GONTRILL_SPRITE_POWER_DIAGONAL,
        GONTRILL_SPRITE_CROSSHAIR, GONTRILL_SPRITE_POWER_ICON,
        GONTRILL_SPRITE_ENEMY, GONTRILL_SPRITE_COUNT
    };

    struct SGontrillShip {
        n2f2_t     Position        = {};
        n2f2_t     Direction       = {};
        s2_t       Health          = 5000;
        s2_t       Shield          = 5000;
        u0_t       Weapon          = 0;
        f2_t       ShotDelay       = 0;
        bool       Alive           = true;
        bool       Firing          = false;
        bool       Thrust          = false;
        bool       Brakes          = false;
    };

    struct SGontrillEnemy {
        n2f2_t     Position        = {};
        s2_t       Health          = 5000;
        s2_t       Shield          = 5000;
        f2_t       Age             = 0;
        f2_t       ShotDelay       = 0;
        u0_t       PathStep        = 0;
        bool       Alive           = false;
    };

    struct SGontrillShot {
        n2f2_t     Position        = {};
        n2f2_t     Previous        = {};
        n2f2_t     Velocity        = {};
        f2_t       Age             = 0;
        u0_t       Weapon          = 0;
        u0_t       Owner           = 0;
        bool       Enemy           = false;
        bool       Alive           = false;
    };

    struct SGontrillPowerup {
        n2f2_t     Position        = {};
        u0_t       Family          = 0;
        u0_t       Type            = 0;
        bool       Alive           = false;
    };

    struct SGontrillParticle {
        n2f2_t     Position        = {};
        n2f2_t     Velocity        = {};
        SColorBGRA Color           = {};
        f2_t       Age             = 0;
        f2_t       Life            = 0;
    };

    struct SGontrillStar {
        n2f2_t     Position        = {};
        f2_t       Speed           = 0;
        u0_t       Brightness      = 0;
    };

    struct SGontrill {
        img8bgra               Sprites [GONTRILL_SPRITE_COUNT] = {};
        SGontrillShip          Ships   [2]     = {};
        SGontrillEnemy         Enemies [128]   = {};
        SGontrillShot          Shots   [512]   = {};
        SGontrillPowerup       Powerups[64]    = {};
        SGontrillParticle      Particles[1024] = {};
        SGontrillStar          Stars   [96]    = {};
        f2_t                   SpawnTime       = 0;
        f2_t                   TotalTime       = 0;
        u2_t                   RandomState     = 0x71C23A5Du;
        u2_t                   Score           = 0;
        bool                   Paused          = false;
        bool                   Debugging       = false;
        bool                   GameOver        = false;
        bool                   ImagesLoaded    = false;
    };

    err_t   gontrillLoad    (SGontrill & game);
    err_t   gontrillReset   (SGontrill & game, n2u2_c & screenSize);
    err_t   gontrillUpdate  (SGontrill & game, SInput const & input, f2_t seconds, n2u2_c & screenSize);
    err_t   gontrillDraw    (SGontrill const & game, g8bgra target);
}

#endif // LLC_GONTRILL_H_23627
