/* Flappy Dunk on a computer, for tests and pictures (make host, make test).
 *
 *   play test          a bot plays 300 runs: it must get deep into the course (tilted, moving and
 *                      narrow hoops), every basket and swish scored as the rules say, the ball always
 *                      on the field; random flapping for 300 more runs never breaks a rule; a save
 *                      comes back, and a damaged one starts afresh
 *   play film DIR N    N frames of the bot's run, a picture every 2 (DIR/f_NNNN.ppm) */
#define NP_MODULE 1 /* (its drawing pushed to fb, its saves kept in disk, below) */
#define main fd_main
#include "../src/main.c"
#undef main
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static uint16_t fb[SCREEN_H][SCREEN_W];
static uint8_t disk[sizeof sv];
static uint32_t disk_len;
void np_push(int x, int y, int w, int h, const color_t *p) {
  for (int j = 0; j < h; j++)
    for (int i = 0; i < w; i++) fb[y + j][x + i] = p[j * w + i];
}
bool eadk_display_wait_for_vblank(void) { return true; }
uint64_t eadk_keyboard_scan(void) { return 0; }
uint64_t eadk_timing_millis(void) { return 0; }
void eadk_timing_msleep(uint32_t ms) { (void)ms; }
void *np_alloc(uint32_t size) { return malloc(size); }
static uint32_t seed = 1;
uint32_t eadk_random(void) { return seed = seed * 1664525u + 1013904223u; }
const uint8_t *np_file_read(const char *name, uint32_t *len) { *len = disk_len; return disk_len ? disk : 0; }
bool np_file_write(const char *name, const void *d, uint32_t len) { memcpy(disk, d, disk_len = len); return true; }

static int fails;
#define CHECK(c, ...) do { if (!(c)) { printf(__VA_ARGS__); printf("\n"); fails++; } } while (0)

/* the next hoop to go through */
static hoop_t *next_hoop(void) {
  hoop_t *n = 0;
  for (int i = 0; i < NH; i++)
    if (!RUN.hoop[i].scored && (!n || RUN.hoop[i].x < n->x)) n = &RUN.hoop[i];
  return n;
}
/* the bot: where the ball would cross the hoop's height, with a flap now or without; it flaps when
   that keeps it closer above the middle of the hoop */
static float cross_at(float y, float vy, float hx, float hy) {
  float x = RUN.x;
  for (int n = 0; n < 200; n++) {
    vy -= G * DT;
    float ny = y + vy * DT;
    x += SCROLL * DT;
    if (ny < -TOP + R) ny = -TOP + R, vy = -vy * BOUNCE;
    if (ny > TOP - R) ny = TOP - R, vy = -vy * BOUNCE;
    if (vy < 0 && y >= hy && ny < hy) return x - hx;
    y = ny;
    if (x > hx + 2) break;
  }
  return 99;
}
static bool bot(void) {
  hoop_t *h = next_hoop();
  if (!h) return false;
  float hy = h->type == H_VERTICAL ? h->y + h->v * (h->x - RUN.x) / SCROLL * 0.5f : h->y;
  float a = cross_at(RUN.y, RUN.vy, h->x, hy), b = cross_at(RUN.y, FLAP, h->x, hy);
  float da = a < 0 ? -a : a, db = b < 0 ? -b : b;
  if (RUN.y > 7.0f) return false;
  return db + 0.15f < da || (a == 99 && RUN.y < hy + 1.5f);
}

static uint32_t score_check, streak_check;
static int play(bool smart, int max_steps) {
  seed = seed * 7 + 13;
  now = seed;
  new_run();
  score_check = streak_check = 0;
  int steps = 0;
  for (; steps < max_steps && !over; steps++) {
    if (smart ? bot() : (eadk_random() >> 8) % 13 == 0) flap();
    uint32_t before = RUN.score, sw = RUN.swishes;
    step();
    if (RUN.score != before) { /* a basket: 1, or n + 1 for the n-th swish in a row */
      if (RUN.swishes != sw) streak_check++, score_check += streak_check + 1;
      else streak_check = 0, score_check += 1;
      CHECK(RUN.score == score_check, "score %u, by the rules %u", RUN.score, score_check);
      CHECK(RUN.streak == streak_check, "streak %u, by the rules %u", RUN.streak, streak_check);
    }
    CHECK(RUN.y == RUN.y && RUN.y >= -TOP + R - 0.001f && RUN.y <= TOP - R + 0.001f, "ball off the field: %f", RUN.y);
    for (int i = 0; i < NH; i++) {
      hoop_t *h = &RUN.hoop[i];
      CHECK(h->x == h->x && h->y > -9 && h->y < 9 && h->x > RUN.x - 12 && h->x < RUN.x + 40, "hoop %d at %f %f", i, h->x, h->y);
    }
  }
  return steps;
}

static void picture(const char *path) {
  FILE *f = fopen(path, "wb");
  if (!f) exit(2);
  fprintf(f, "P6 320 240 255\n");
  for (int y = 0; y < SCREEN_H; y++)
    for (int x = 0; x < SCREEN_W; x++) {
      uint16_t c = fb[y][x];
      fputc((c >> 11) * 255 / 31, f), fputc((c >> 5 & 63) * 255 / 63, f), fputc((c & 31) * 255 / 31, f);
    }
  fclose(f);
}
static void render(void) {
  gfx_buf = strip;
  gfx_render(scene, 0, 0, SCREEN_H);
}

int main(int argc, char **argv) {
  if (argc >= 4 && !strcmp(argv[1], "film")) {
    int frames = atoi(argv[3]);
    seed = argc > 4 ? (uint32_t)atoi(argv[4]) : 5;
    now = 1000;
    new_run();
    char path[256];
    for (int f = 0, k = 0; f < frames; f++) {
      now += 33;
      if (f > 20) { /* hovering first, then the bot's run: 50 steps a second, a frame every 33 ms */
        static uint32_t acc;
        for (acc += 33; acc >= 20; acc -= 20) {
          if (bot()) flap();
          step();
          float a = spin * DT, c = spin_c - spin_s * a, s = spin_s + spin_c * a, m = 1.0f / sqrt_(c * c + s * s);
          spin_c = c * m, spin_s = s * m;
        }
      } else {
        float t = (float)(now % 1200) / 600.0f - 1.0f;
        RUN.y = 0.35f * (t < 0 ? -t : t) - 0.17f;
      }
      cam = RUN.x - BALL_SX / PX;
      for (int i = 7; i > 0; i--) trail_x[i] = trail_x[i - 1] - SCROLL * 0.033f * PX, trail_y[i] = trail_y[i - 1];
      trail_x[0] = BALL_SX, trail_y[0] = SY(RUN.y);
      if (over) break;
      if (f % 2) continue;
      render();
      snprintf(path, sizeof path, "%s/f_%04d.ppm", argv[2], k++);
      picture(path);
    }
    printf("score %u\n", RUN.score);
    return 0;
  }
  /* the bot: deep into the course */
  long total = 0;
  uint32_t best = 0, deep = 0;
  for (int r = 0; r < 300; r++) {
    play(true, 50 * 600);
    total += RUN.score, best = RUN.score > best ? RUN.score : best;
    if (RUN.n > 45) deep++;
  }
  printf("bot: 300 runs, %ld points on average, best %u, %u past the narrow hoops\n", total / 300, best, deep);
  CHECK(deep >= 30, "the bot should get past hoop 45 in 30 runs of 300");
  /* random flapping */
  for (int r = 0; r < 300; r++) play(false, 50 * 120);
  /* the save: back as it was; damaged, reset */
  sv.best = 77, sv.flags |= 1;
  save();
  uint32_t best_saved = sv.best;
  memset(&sv, 0, sizeof sv);
  load();
  CHECK(sv.best == best_saved && (sv.flags & 1), "save not read back");
  disk[sizeof sv - 2] ^= 0x40; /* one bit of a hoop's speed */
  load();
  CHECK(!(sv.flags & 1) && sv.best == 0, "a damaged file should start afresh");
  disk_len = 5;
  load();
  CHECK(sv.best == 0, "a short file should reset");
  render();
  printf(fails ? "FAILED (%d)\n" : "ok\n", fails);
  return fails != 0;
}
