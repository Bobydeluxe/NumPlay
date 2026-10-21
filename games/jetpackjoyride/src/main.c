/* Jetpack Joyride (Halfbrick, 2011) on the NumWorks calculator.
 *
 * Barry Steakfries breaks into Legitimate Research's lab, steals the machine gun jetpack and flies as far as
 * he can: hold to fly, let go to fall, through zappers, missiles and lasers, grabbing coins, spin tokens and
 * vehicle boxes (the Lil' Stomper, the Profit Bird, the Crazy Freaking Teleporter, the Bad As Hog, the Gravity
 * Suit). The run steps 60 times a second with the original's numbers (sim.c, data.h); the Final Spin plays the
 * tokens after it.
 *
 * Keys: OK, EXE or Up fly (or a vehicle's action), Back pauses, Home saves and quits. */
#include <string.h>
#include "../../../launcher/src/gfx.h"
#include "jj.h"
#include "draw.h"
#include "plat.h"
#include "save.h"
#include "data.h"

static jj_run run;
static jj_profile prof;
static jj_view view;
static jj_spin spin;
static uint32_t rng, blast_m;
static uint8_t back_to, banked, spin_t;
static color_t strip[SCREEN_W * STRIP_H];
/* frames drawn, the slowest (ms) and those over 33 ms: read by tools/emu.py */
#ifndef HOST
__attribute__((used, externally_visible))
#endif
uint32_t perf_frames, perf_max, perf_slow;

static const uint16_t prize_coins[JJ_PRIZES] = {100, 500, 1000};
static const uint16_t prize_blast[JJ_PRIZES] = {[JJ_PRIZE_EXPLOSION1] = 160, [JJ_PRIZE_EXPLOSION2] = 280, [JJ_PRIZE_EXPLOSION3] = 400};

static void new_run(void) {
  jj_start(&run, jj_rand(&rng), prof.double_coins, prof.head_start);
  prof.double_coins = prof.head_start = 0;
  prof.has_run = 1;
  if (prof.runs < 0xffffffffu) prof.runs++;
  blast_m = 0, banked = 0;
  view.screen = JJ_SCREEN_PLAY;
}

/* the run's over: its coins go in the bank, and its distance may be the best */
static void bank(void) {
  if (banked) return;
  banked = 1;
  prof.coins += run.coins;
  prof.has_run = 0;
}

static void results(void) {
  bank();
  uint32_t m = jj_metres(&run) + blast_m;
  view.run_m = m, view.run_coins = run.coins;
  view.new_best = m > prof.best;
  if (m > prof.best) prof.best = m;
  view.screen = JJ_SCREEN_RESULTS, view.sel = 0;
  save_write(&prof, 0);
}

static void cash_in(void) {
  prof.coins += run.tokens * 50u;
  run.tokens = 0;
}

static void sync_view(void) {
  view.best = prof.best, view.coins = prof.coins;
  view.head_start = prof.head_start, view.double_coins = prof.double_coins;
  view.tokens = run.tokens;
}

/* a 60th of a second of the Final Spin: the reels turn, then stop one after the other on the result */
static void spin_tick(void) {
  spin_t++;
  for (int i = 0; i < 3; i++) {
    int stop = 40 + i * 14;
    if (spin_t < stop) {
      view.reel[i] = (uint16_t)((view.reel[i] + 12) % (48 * JJ_PRIZES));
    } else if (spin_t == stop) {
      int icon = spin.won ? spin.won - 1 : (int)((jj_rand(&rng) % JJ_PRIZES + (unsigned)i * 3) % JJ_PRIZES);
      if (!spin.won && i == 2 && view.reel[0] / 48 == view.reel[1] / 48 && (int)(view.reel[0] / 48) == icon)
        icon = (icon + 1) % JJ_PRIZES;   /* a losing spin never shows three of a kind */
      view.reel[i] = (uint16_t)(48 * icon);
    }
  }
  if (spin_t < 40 + 2 * 14 + 4) return;
  view.spinning = 0;
  int p = spin.won ? spin.won - 1 : -1;
  view.spin_msg = (int8_t)(p >= 0 ? p : -2);
  if (p < 0) return;
  prof.coins += prize_coins[p];
  blast_m += prize_blast[p];
  if (p == JJ_PRIZE_TOKENS) run.tokens = (uint8_t)(run.tokens + 3);
  if (p == JJ_PRIZE_DOUBLE_COINS) prof.double_coins = 1;
  if (p == JJ_PRIZE_START_500M) prof.head_start = 1;
}

/* a Second Chance: the run goes on, its coins out of the bank again */
static void second_chance(void) {
  jj_revive(&run);
  banked = 0, prof.coins -= run.coins, prof.has_run = 1;
  view.screen = JJ_SCREEN_PLAY, view.spin_msg = -1;
}

int main(void) {
  plat_begin();
  gfx_buf = strip;
  save_load(&prof, &run);
  rng = plat_random() ^ prof.runs * 0x9E3779B9u;
  if (!rng) rng = 1;
  view.screen = prof.has_run ? JJ_SCREEN_PAUSE : JJ_SCREEN_TITLE;
  view.spin_msg = -1;
  uint32_t stale = plat_keys(), prev = stale, last = plat_millis(), acc = 0, saved_at = last;
  uint8_t shown = 0xff;
  bool redraw = true;
  for (;;) {
    uint32_t now = plat_millis(), keys = plat_keys();
    stale &= keys;                      /* keys still down from the screen before don't count */
    uint32_t live = keys & ~stale, hit = live & ~prev;
    prev = live;
    if (keys & JK_HOME) {
      if (view.screen == JJ_SCREEN_SPIN) {
        while (view.spinning) spin_tick();   /* the spin under way counts; the other tokens are cashed in */
        if (view.spin_msg == JJ_PRIZE_REVIVE) second_chance();
        else cash_in(), results();
      }
      save_write(&prof, prof.has_run ? &run : 0);
      break;
    }
    uint32_t dt = now - last;
    last = now;
    if (dt > 100) dt = 100;
    /* 60ths of a second gone by: the run (and the reels) step that many times, 4 at most */
    acc += dt * 60;
    int ticks = 0;
    while (acc >= 1000 && ticks < 4) acc -= 1000, ticks++;
    if (acc >= 1000) acc = 0;   /* too slow: the game slows down rather than skip */
    int steps = 0;
    switch (view.screen) {
    case JJ_SCREEN_TITLE:
      view.clock += dt / 8;
      redraw = true;
      if (hit & JK_BACK) {
        save_write(&prof, 0);
        return plat_end();
      }
      if (hit & JK_OK) {
        if (!prof.guide_seen) back_to = JJ_SCREEN_PLAY, view.screen = JJ_SCREEN_GUIDE;
        else new_run();
      }
      break;
    case JJ_SCREEN_GUIDE:
      if (hit & (JK_OK | JK_BACK)) {
        prof.guide_seen = 1;
        if (back_to == JJ_SCREEN_PLAY) new_run();
        else view.screen = back_to, view.sel = 2;
      }
      break;
    case JJ_SCREEN_PLAY:
      if (hit & JK_BACK) {
        view.screen = JJ_SCREEN_PAUSE, view.sel = 0;
        save_write(&prof, &run), saved_at = now;
        break;
      }
      for (; steps < ticks; steps++) jj_step(&run, (live & (JK_OK | JK_UP)) != 0);
      if (jj_over(&run)) {
        bank();
        if (run.tokens) {
          jj_spin_start(&spin);
          view.screen = JJ_SCREEN_SPIN, view.spinning = 0, view.spin_msg = -1;
          for (int i = 0; i < 3; i++) view.reel[i] = (uint16_t)(48 * (i * 3 + 1));
        } else {
          results();
        }
      } else if (now - saved_at > 10000) {
        save_write(&prof, &run), saved_at = now;
      }
      break;
    case JJ_SCREEN_PAUSE:
      if (hit & JK_UP) view.sel = (uint8_t)((view.sel + 3) % 4);
      if (hit & JK_DOWN) view.sel = (uint8_t)((view.sel + 1) % 4);
      if (hit & JK_BACK) view.screen = JJ_SCREEN_PLAY, acc = 0;
      else if (hit & JK_OK) {
        if (view.sel == 0) view.screen = JJ_SCREEN_PLAY, acc = 0;
        else if (view.sel == 2) back_to = JJ_SCREEN_PAUSE, view.screen = JJ_SCREEN_GUIDE;
        else view.screen = JJ_SCREEN_CONFIRM, view.sel = view.sel == 3;
      }
      break;
    case JJ_SCREEN_CONFIRM:
      if (hit & JK_BACK) view.sel = view.sel ? 3 : 1, view.screen = JJ_SCREEN_PAUSE;
      else if (hit & JK_OK) {
        bank();
        uint32_t m = jj_metres(&run);
        if (m > prof.best) prof.best = m;
        if (view.sel) view.screen = JJ_SCREEN_TITLE, save_write(&prof, 0);
        else new_run();
      }
      break;
    case JJ_SCREEN_SPIN:
      if (view.spinning) {
        for (int k = 0; k < ticks && view.spinning; k++) spin_tick();
        redraw = true;
        break;
      }
      if (view.spin_msg == JJ_PRIZE_REVIVE && (hit & (JK_OK | JK_BACK))) {
        second_chance();
        acc = 0;
        break;
      }
      if (hit & JK_BACK) cash_in(), results();
      else if (hit & JK_OK) {
        if (!run.tokens) results();
        else {
          run.tokens--;
          jj_spin_roll(&spin, &rng);
          view.spinning = 1, spin_t = 0, view.spin_msg = -1;
        }
      }
      break;
    case JJ_SCREEN_RESULTS:
      if (hit & (JK_UP | JK_DOWN)) view.sel ^= 1;
      if (hit & JK_BACK) view.screen = JJ_SCREEN_TITLE;
      else if (hit & JK_OK) {
        if (view.sel == 0) new_run();
        else view.screen = JJ_SCREEN_TITLE;
      }
      break;
    }
    if (view.screen != shown) {
      stale = keys, prev = 0;           /* a new screen starts with all keys up */
      shown = view.screen, redraw = true;
    }
    if (steps || redraw || hit) {
      sync_view();
      uint32_t t0 = plat_millis();
      jj_draw(&run, &view);
      uint32_t spent = plat_millis() - t0;
      if (view.screen == JJ_SCREEN_PLAY) {
        perf_frames++;
        if (spent > perf_max) perf_max = spent;
        if (spent > 33) perf_slow++;
      }
      redraw = false;
    } else {
      plat_sleep(2);
    }
  }
  return plat_end();
}
