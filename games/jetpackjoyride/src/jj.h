/* Jetpack Joyride for the NumWorks calculator: the run, stepped 60 times a second in the original's points
 * (the iPhone's 480 x 320 screen) with the original's numbers (data.h, from the game's own files).
 *
 * The world: Barry stays at x = 128 on screen; everything else is kept in screen points and moves left by
 * the run's speed. y grows downwards; the ceiling is at 30, the floor at 290. Drawn at 2/3 (320 x 213). */
#ifndef JJ_MAIN_H
#define JJ_MAIN_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define JJ_W 480
#define JJ_H 320
#define JJ_CEIL 30
#define JJ_FLOOR 290
#define JJ_MID 160
#define JJ_BX 128
#define JJ_COIN_GAP 18
#define JJ_SPAWN_X 520      /* where things come in, off the right of the screen */

enum { JJ_FOOT, JJ_MECH, JJ_HOG, JJ_BIRD, JJ_GRAV, JJ_CFT, JJ_RIDES };
enum { JJ_ALIVE, JJ_ZAPPED, JJ_TUMBLE, JJ_STOPPED };            /* Barry */
enum { JJ_BY_ZAPPER = 1, JJ_BY_MISSILE, JJ_BY_LASER };          /* what got him */
enum { JJ_WARN, JJ_LOCK, JJ_FLY };                              /* a missile */

#define JJ_ZAPPERS 8
#define JJ_COINSETS 4
#define JJ_MISSILES 8
#define JJ_TOKENS 2
#define JJ_FX 6
#define JJ_SCIENTISTS 3

typedef struct {
  float x, y;           /* the middle */
  uint16_t ang;         /* 65536 a turn; 0 = horizontal, 16384 = vertical */
  int16_t spin;         /* per frame (rotating zappers) */
  uint16_t len;         /* between the two orbs */
  uint8_t on, dud;      /* dud: switched off by a vehicle */
} jj_zapper;

typedef struct {
  float x;              /* the first column */
  int16_t y;            /* the top row */
  uint8_t on, pat;
  uint8_t bits[48];     /* the coins still there, rows of (w + 7) / 8 bytes */
} jj_coinset;

typedef struct {
  float x, y, chase;
  uint8_t on, state;
  uint16_t t;
} jj_missile;

typedef struct {
  float x, y;           /* y: the middle of the bob */
  uint16_t phase;
  uint8_t on, pad;
} jj_token;

typedef struct {
  float x, y;
  uint8_t kind, t;      /* kind 0 a missile or Barry hit, 1 a vehicle breaking */
} jj_fx;

typedef struct {
  float x;
  int8_t dir;           /* -1 left, 1 right */
  uint8_t on, state, t; /* state 0 walking, 1 running, 2 knocked down */
} jj_scientist;

typedef struct {
  /* Barry */
  float y, vy;
  uint8_t held, ground, state, by;
  uint8_t ride, jumps, boost, flip;  /* flip: the Gravity Suit upside down */
  uint16_t cft;                      /* the Teleporter's arrow phase */
  uint16_t safe;                     /* frames he can't be hit */
  float dvx;                         /* knocked out: his own speed (points a frame) */
  float ride_speed;                  /* points a second on a vehicle */
  uint16_t stop_t;                   /* frames since he came to rest */
  uint8_t fire;                      /* frames to the next bullet */
  /* the run */
  uint32_t rng, t;
  uint32_t dist;                     /* 16ths of a point */
  uint32_t coins;
  uint8_t tokens, set, streak, laser;
  uint16_t laser_t;                  /* frames into the laser pattern; laser = pattern + 1 */
  float next;                        /* points to the next spawn */
  uint32_t token_last, token_km;     /* metres of the last token, of the kilometre counted */
  uint8_t token_count, revived, double_coins, head_start;
  uint32_t vehicle_at;               /* metres where the next vehicle box comes */
  jj_zapper zap[JJ_ZAPPERS];
  jj_coinset coin[JJ_COINSETS];
  jj_missile mis[JJ_MISSILES];
  jj_token token[JJ_TOKENS];
  jj_token box;                      /* the vehicle box */
  jj_fx fx[JJ_FX];
  jj_scientist sci[JJ_SCIENTISTS];
  /* counted for the results */
  uint32_t zappers_dodged, missiles_dodged, tokens_taken;
} jj_run;

void jj_start(jj_run *r, uint32_t seed, bool double_coins, bool head_start);
void jj_step(jj_run *r, bool held);
bool jj_over(const jj_run *r);          /* he's come to rest: the Final Spin or the results */
uint32_t jj_metres(const jj_run *r);
float jj_speed(const jj_run *r);        /* points a second */
void jj_revive(jj_run *r);              /* the Second Chance */
bool jj_valid(const jj_run *r);         /* a run read back from a save is sane */

/* shared with the drawing */
int jj_sin(uint16_t ang);               /* x 1024 */
int jj_cos(uint16_t ang);
void jj_beam_state(int pattern, int beam, int t, int *y, int *phase, int *k); /* phase 0 off, 1 charging, 2 firing */
int jj_coin_at(const jj_coinset *c, int col, int row);
int jj_box_h(int ride);                 /* Barry's (or his vehicle's) hit box */
int jj_box_w(int ride);
uint32_t jj_rand(uint32_t *s);

/* the Final Spin */
typedef struct {
  uint16_t weight[10];
  uint8_t won;                          /* the last spin's prize + 1, or 0 */
} jj_spin;
void jj_spin_start(jj_spin *s);
int jj_spin_roll(jj_spin *s, uint32_t *rng);   /* a prize, or -1 */
#endif
