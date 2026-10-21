/* Tower Blocks on a computer, for tests and pictures (make host, make test).
 *
 *   play test          a bot builds 300 towers, aiming a little off: every block kept is the part over the
 *                      one below, landing within 0.3 keeps it whole, and missing ends the game; pressing OK
 *                      at random never breaks those rules; a save comes back, and a damaged one starts afresh
 *   play film DIR N    N frames of the bot's tower, a picture every 2 (DIR/f_NNNN.ppm) */
#define NP_MODULE 1 /* (its drawing pushed to fb, its saves kept in disk, below) */
#define main tb_main
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
uint32_t eadk_random(void) { return 0x2545F491u; }
const uint8_t *np_file_read(const char *name, uint32_t *len) { *len = disk_len; return disk_len ? disk : 0; }
bool np_file_write(const char *name, const void *d, uint32_t len) { memcpy(disk, d, disk_len = len); return true; }

static int fails;
#define CHECK(c, ...) do { if (!(c)) { printf(__VA_ARGS__); printf("\n"); fails++; } } while (0)
static float r01(void) { static uint32_t s = 7; s = s * 1664525u + 1013904223u; return (float)(s >> 8) / 16777216.0f; }

/* one block: ticks until it is within `off` of the one below (or never, for a miss), then placed;
   the result checked against the original's rules */
static bool drop(float off, bool random_press) {
  uint32_t i = MOVING;
  slab_t below = AT(i - 1);
  int axis = i % 2 ? 0 : 1;
  float bp = axis ? below.z : below.x, dim = axis ? below.d : below.w;
  for (int t = 0; t < 4000; t++) {
    tick();
    if (random_press ? r01() < 0.02f : fabs_(sv.pos - (bp + off)) <= fabs_(sv.dir) / 2) break;
  }
  float pos = sv.pos, overlap = dim - fabs_(pos - bp);
  uint32_t score = sv.score;
  place();
  if (dim - overlap < SNAP) {
    slab_t b = AT(i);
    CHECK(sv.score == score + 1 && b.x == below.x && b.z == below.z && b.w == below.w && b.d == below.d, "a near miss should keep the whole block");
  } else if (overlap > 0) {
    slab_t b = AT(i);
    float p = axis ? b.z : b.x, w = axis ? b.d : b.w, q = axis ? b.x : b.z, v = axis ? b.w : b.d;
    float qb = axis ? below.x : below.z, vb = axis ? below.w : below.d;
    CHECK(sv.score == score + 1, "score");
    CHECK(fabs_(w - overlap) < 1e-4f && p >= bp - 1e-4f && p + w <= bp + dim + 1e-4f, "kept part %f..%f over %f..%f", p, p + w, bp, bp + dim);
    CHECK(q == qb && v == vb, "the other axis must not change");
  } else {
    CHECK(state == S_OVER && sv.score == score && !(sv.flags & 1), "a miss should end the game");
    return false;
  }
  return true;
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
  seed = 99;
  if (argc >= 4 && !strcmp(argv[1], "film")) {
    int frames = atoi(argv[3]), k = 0;
    seed = argc > 4 ? (uint32_t)atoi(argv[4]) : 99;
    now = 1000;
    sv.flags = 2;
    new_game();
    cam = cam_to;
    float target = 0;
    bool aimed = false;
    char path[256];
    for (int f = 0; f < frames; f++) {
      now += 17;
      if (state == S_PLAY) {
        tick();
        uint32_t i = MOVING;
        slab_t below = AT(i - 1);
        float bp = i % 2 ? below.x : below.z;
        if (!aimed) target = bp + (r01() < 0.55f ? 0 : (r01() - 0.5f) * 3.0f), aimed = true;
        if (argc > 5 && sv.score >= (uint32_t)atoi(argv[5])) target = bp > 0 ? bp - 10.5f : bp + 10.5f; /* a miss, to film the end */
        if (fabs_(sv.pos - target) <= fabs_(sv.dir) / 2 && now - cam_t > 350) place(), aimed = false;
      }
      if (state == S_OVER && now - over_t > 900) /* OK: the tower goes */
        state = S_RESET, reset_t = now, reset_n = TOP, shown = sv.score, camera_to(LAYER * 2 + 2);
      if (state == S_RESET) {
        float dur = 0.4f + (float)reset_n * 0.02f, t = (float)(now - reset_t) / 1000.0f / dur;
        shown = (uint32_t)((float)sv.score * (1 - (t > 1 ? 1 : t)) + 0.5f);
        cam = cam_from + (cam_to - cam_from) * ease_io(t);
        if (t >= 1) break;
      } else
        cam = cam_from + (cam_to - cam_from) * ease_io((float)(now - cam_t) / 300.0f);
      if (f % 2) continue;
      render();
      snprintf(path, sizeof path, "%s/f_%04d.ppm", argv[2], k++);
      picture(path);
    }
    printf("score %u\n", sv.score);
    return 0;
  }
  /* the bot, a little off each time */
  uint32_t best = 0;
  long total = 0;
  for (int g = 0; g < 300; g++) {
    new_game();
    while (drop((r01() - 0.5f) * (0.2f + 1.6f * r01()), false) && sv.score < 2000) {}
    total += sv.score, best = sv.score > best ? sv.score : best;
  }
  printf("bot: 300 towers, %ld blocks on average, best %u\n", total / 300, best);
  CHECK(best > 100, "the bot should build past 100 at least once");
  /* random presses */
  for (int g = 0; g < 300; g++) {
    new_game();
    while (drop(0, true) && sv.score < 500) {}
  }
  /* the save: back as it was; damaged, a fresh start */
  new_game();
  for (int n = 0; n < 5; n++) drop(0.5f, false);
  sv.best = 77;
  save();
  uint32_t score = sv.score;
  memset(&sv, 0, sizeof sv);
  load();
  CHECK(sv.best == 77 && sv.score == score && (sv.flags & 1), "save not read back");
  disk[30] ^= 4;
  load();
  CHECK(sv.best == 0 && !(sv.flags & 1), "a damaged file should start afresh");
  render();
  printf(fails ? "FAILED (%d)\n" : "ok\n", fails);
  return fails != 0;
}
