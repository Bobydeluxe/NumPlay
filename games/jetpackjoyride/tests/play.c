/* Jetpack Joyride on a computer, for tests and pictures (make host, make test).
 *
 *   play test           the rules against the original's numbers (the spec), a bot's runs (no broken state, every
 *                       hazard and vehicle met), saves written and read back exactly, damaged saves refused, and a few
 *                       hundred sessions of the whole app with random keys, quitting with Home each time
 *   play shots DIR      pictures of the screens and of each hazard and vehicle (DIR/<name>.ppm)
 *   play film DIR S A N N frames of a bot's run (seed S), every third step, after A (DIR/f_NNNN.ppm)
 */
#define main jj_app_main
#include "../src/main.c"
#undef main
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

/* ---------------------------------------------------------------- the calculator, on a computer */
static uint16_t fb[SCREEN_H][SCREEN_W];
void np_push(int x, int y, int w, int h, const color_t *px) {
  for (int j = 0; j < h; j++)
    for (int i = 0; i < w; i++) fb[y + j][x + i] = px[j * w + i];
}
void *np_alloc(uint32_t size) { return malloc(size); }   /* (gfx_init's; the game has its own strip) */
static double clock_ms;
static uint32_t (*script)(uint32_t frame);
static uint32_t frame_no, quit_at;
uint32_t plat_keys(void) {
  clock_ms += 1000.0 / 30;   /* the app's loop at 30 a second */
  frame_no++;
  if (frame_no >= quit_at) return JK_HOME;
  return script ? script(frame_no) : 0;
}
uint32_t plat_millis(void) { return (uint32_t)clock_ms; }
void plat_sleep(uint32_t ms) { clock_ms += ms; }
uint32_t plat_random(void) { return 0x9E3779B9u ^ frame_no; }
void plat_begin(void) {}
int plat_end(void) { return 0; }

/* files: a few named records in memory */
static struct { char name[32]; uint8_t data[4096]; uint32_t len; bool on; } files[4];
bool plat_save(const char *name, const void *data, uint32_t len) {
  int k = -1;
  for (int i = 0; i < 4; i++)
    if (files[i].on && !strcmp(files[i].name, name)) k = i;
  for (int i = 0; i < 4 && k < 0; i++)
    if (!files[i].on) k = i;
  if (k < 0 || len > sizeof files[k].data) return false;
  strcpy(files[k].name, name), memcpy(files[k].data, data, len), files[k].len = len, files[k].on = true;
  return true;
}
const uint8_t *plat_load(const char *name, uint32_t *len) {
  for (int i = 0; i < 4; i++)
    if (files[i].on && !strcmp(files[i].name, name)) return *len = files[i].len, files[i].data;
  return 0;
}
static void forget(const char *name) {
  for (int i = 0; i < 4; i++)
    if (files[i].on && !strcmp(files[i].name, name)) files[i].on = false;
}

static void shot(const char *path) {
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

static int fails;
#define CHECK(c, ...) do { if (!(c)) { fails++; printf("FAIL %s:%d: ", __FILE__, __LINE__); printf(__VA_ARGS__); printf("\n"); } } while (0)

/* ---------------------------------------------------------------- a bot: plans (hold n, let go m) tried ahead */
static bool greedy;   /* the film's bot also goes for coins, tokens and vehicles */
static int survive(const jj_run *r0, int hold, int rel, int horizon) {
  static jj_run r;
  r = *r0;
  for (int f = 0; f < horizon; f++) {
    int cyc = hold + rel, p = cyc ? f % cyc : 0;
    jj_step(&r, p < hold);
    if (r.state != JJ_ALIVE) return f * 10;
  }
  return horizon * 10 + (greedy ? (int)(r.coins - r0->coins) + 30 * (r.tokens - r0->tokens) + 40 * (r.ride != r0->ride) : 0);
}
static int bot_h, bot_r, bot_left;
static bool bot(const jj_run *r) {
  if (bot_left <= 0) {
    static const int plans[][2] = {{0, 1}, {1, 0}, {2, 2}, {4, 4}, {6, 3}, {3, 6}, {8, 2}, {2, 8}, {1, 12}};
    int best = -1;
    for (unsigned k = 0; k < sizeof plans / sizeof plans[0] && (greedy || best < 1000); k++) {
      int v = survive(r, plans[k][0], plans[k][1], 100);
      if (v > best) best = v, bot_h = plans[k][0], bot_r = plans[k][1];
    }
    bot_left = 5;
  }
  bot_left--;
  int cyc = bot_h + bot_r, p = cyc ? (int)(r->t % (uint32_t)cyc) : 0;
  return p < bot_h;
}

/* ---------------------------------------------------------------- the rules against the spec */
static void units(void) {
  jj_run r;
  jj_start(&r, 7, 0, 0);
  /* falling and flying settle at gravity/(1-friction) and (boost-gravity)/(1-friction) points a frame */
  r.y = 200, r.vy = 0, r.ground = 0;
  float v = 0;
  for (int i = 0; i < 400; i++) v = (v + JJ_GRAVITY) * JJ_FRICTION;
  CHECK(fabsf(v - 7.35f) < 0.05f, "terminal fall %f", v);
  v = 0;
  for (int i = 0; i < 400; i++) v = (v + JJ_GRAVITY - JJ_BOOST) * JJ_FRICTION;
  CHECK(fabsf(v + 10.29f) < 0.05f, "terminal climb %f", v);
  /* held from the floor, Barry reaches the ceiling and stays under it */
  jj_start(&r, 7, 0, 0);
  for (int i = 0; i < 120; i++) {
    float before = r.y;
    r.safe = 1000;
    jj_step(&r, true);
    CHECK(r.y <= before + 0.001f || i == 0, "climbing");
  }
  CHECK(fabsf(r.y - (JJ_CEIL + jj_box_h(JJ_FOOT))) < 0.01f, "ceiling %f", r.y);
  for (int i = 0; i < 200; i++) r.safe = 1000, jj_step(&r, false);
  CHECK(r.y == JJ_FLOOR && r.ground, "floor %f", r.y);
  /* speed: 200 points a second at the start, 505 from 2000 m, linear between */
  jj_start(&r, 7, 0, 0);
  CHECK(fabsf(jj_speed(&r) - 200) < 0.01f, "start speed");
  r.dist = 1000u * 16 * JJ_DIST_DIV;
  CHECK(fabsf(jj_speed(&r) - 352.5f) < 0.01f, "speed at 1000 m %f", jj_speed(&r));
  r.dist = 2500u * 16 * JJ_DIST_DIV;
  CHECK(fabsf(jj_speed(&r) - 505) < 0.01f, "speed at 2500 m");
  /* a head start starts at 500 m */
  jj_start(&r, 7, 0, 1);
  CHECK(jj_metres(&r) == 500, "head start");
  /* every laser shot ends inside its pattern; every pattern fires */
  for (int p = 0; p < JJ_LASER_PATTERNS; p++) {
    int fired = 0;
    for (int b = 0; b < jj_lpats[p].count; b++)
      for (int t = 0; t <= jj_lpats[p].time + 20; t++) {
        int y, ph, k;
        jj_beam_state(p, b, t, &y, &ph, &k);
        if (ph == 2) {
          fired = 1;
          CHECK(t < jj_lpats[p].time, "laser %d beam %d fires at %d", p, b, t);
        }
        CHECK(y >= JJ_MID - 127 && y <= JJ_MID + 127, "laser height");
      }
    CHECK(fired, "laser pattern %d never fires", p);
  }
  /* coin patterns: the files' coins (coinpattern1.txt has 41) */
  {
    jj_coinset c = {0};
    int n = 0;
    c.pat = 0, memcpy(c.bits, jj_pat_bits + jj_pat_off[0], sizeof c.bits);
    for (int row = 0; row < jj_pat_h[0]; row++)
      for (int col = 0; col < jj_pat_w[0]; col++) n += jj_coin_at(&c, col, row);
    CHECK(n == 41, "coinpattern1 has %d coins", n);
  }
  /* the Final Spin: weights as slots.xml; a Second Chance, once won, is 40 times rarer; double coins and the head
     start come once */
  {
    jj_spin s;
    jj_spin_start(&s);
    CHECK(s.weight[JJ_PRIZE_REVIVE] == 100 && s.weight[JJ_PRIZE_START_500M] == 200, "spin weights");
    uint32_t g = 12345;
    int wins = 0, n = 40000, dc = 0;
    for (int i = 0; i < n; i++) {
      jj_spin_start(&s);
      int p = jj_spin_roll(&s, &g);
      if (p >= 0) wins++;
      if (p == JJ_PRIZE_DOUBLE_COINS) {
        dc++;
        CHECK(s.weight[JJ_PRIZE_DOUBLE_COINS] == 0, "double coins once");
      }
      if (p == JJ_PRIZE_REVIVE) CHECK(s.weight[JJ_PRIZE_REVIVE] == 2, "revive weight after a win %d", s.weight[JJ_PRIZE_REVIVE]);
    }
    CHECK(wins > n / 5 && wins < n * 3 / 10, "one spin in four wins: %d of %d", wins, n);
    CHECK(dc > 0, "double coins never won");
  }
  /* a fresh run is valid, and broken ones are refused */
  jj_start(&r, 99, 0, 0);
  CHECK(jj_valid(&r), "fresh run invalid");
  jj_run b = r;
  b.y = NAN;
  CHECK(!jj_valid(&b), "NaN accepted");
  b = r, b.coin[0].on = 1, b.coin[0].pat = 200;
  CHECK(!jj_valid(&b), "bad pattern accepted");
}

/* ---------------------------------------------------------------- the bot's runs */
static void bots(void) {
  int seen_ride[JJ_RIDES] = {0}, lasers = 0, missiles = 0, rotating = 0, tokens = 0, swarm = 0;
  uint32_t total_m = 0;
  for (int s = 1; s <= 24; s++) {
    jj_run r;
    jj_start(&r, (uint32_t)s * 2654435761u, s % 5 == 0, 0);
    bot_left = 0, greedy = s % 2 == 0;   /* half of them go for coins and tokens too */
    while (r.state == JJ_ALIVE && jj_metres(&r) < 5000) {
      jj_step(&r, bot(&r));
      seen_ride[r.ride] = 1;
      if (r.laser) lasers++;
      if (r.set == JJ_SET_MISSILE) swarm++;
      for (int i = 0; i < JJ_MISSILES; i++) missiles += r.mis[i].on;
      for (int i = 0; i < JJ_ZAPPERS; i++) rotating += r.zap[i].on && r.zap[i].spin;
      if (r.t % 60 == 0) CHECK(jj_valid(&r), "seed %d: invalid at %u", s, r.t);
    }
    for (int i = 0; i < 6000 && !jj_over(&r); i++) jj_step(&r, false);
    CHECK(jj_over(&r) || r.state == JJ_ALIVE, "seed %d: never came to rest", s);
    CHECK(jj_valid(&r), "seed %d: invalid at the end", s);
    tokens += r.tokens_taken;
    total_m += jj_metres(&r);
  }
  greedy = false;
  for (int v = 0; v < JJ_RIDES; v++) CHECK(seen_ride[v], "vehicle %d never ridden", v);
  CHECK(lasers && missiles && rotating && swarm, "hazards: lasers %d missiles %d rotating %d swarm %d", lasers, missiles, rotating, swarm);
  CHECK(tokens > 12, "tokens %d", tokens);
  printf("bot: 24 runs, %u m in all, %d tokens\n", total_m, tokens);
}

/* ---------------------------------------------------------------- saves */
static void saves(void) {
  jj_run r, back;
  jj_profile p = {1234, 567, 8, 1, 1, 0, 0}, q;
  jj_start(&r, 4242, 0, 0);
  bot_left = 0;
  for (int i = 0; i < 3000 && r.state == JJ_ALIVE; i++) jj_step(&r, bot(&r));
  forget(JJ_SAVE_NAME), forget(JJ_COPY_NAME);
  CHECK(save_write(&p, &r), "save failed");
  memset(&back, 0, sizeof back);
  save_load(&q, &back);
  CHECK(q.best == 1234 && q.coins == 567 && q.has_run == 1, "profile back");
  CHECK(!memcmp(&back, &r, sizeof r), "run back");
  /* the same run goes on the same way */
  for (int i = 0; i < 600; i++) {
    bool h = bot(&r);
    jj_step(&r, h), jj_step(&back, h);
  }
  CHECK(!memcmp(&back, &r, sizeof r), "run diverged after loading");
  /* the file gone: the copy in the script brings it back */
  forget(JJ_SAVE_NAME);
  memset(&q, 0, sizeof q);
  save_load(&q, &back);
  CHECK(q.best == 1234 && q.has_run, "copy back");
  uint32_t n;
  CHECK(plat_load(JJ_SAVE_NAME, &n) != 0, "save restored from the copy");
  /* every byte damaged in turn: refused (or, by chance, still valid), never a crash */
  forget(JJ_COPY_NAME);
  save_write(&p, &r);
  uint8_t keep[4096];
  const uint8_t *d = plat_load(JJ_SAVE_NAME, &n);
  memcpy(keep, d, n);
  forget(JJ_COPY_NAME);
  int refused = 0;
  for (uint32_t i = 0; i < n; i++) {
    uint8_t bad[4096];
    memcpy(bad, keep, n);
    bad[i] ^= (uint8_t)(1 + i % 255);
    plat_save(JJ_SAVE_NAME, bad, n);
    forget(JJ_COPY_NAME);
    memset(&q, 0xAB, sizeof q);
    save_load(&q, &back);
    refused += q.best == 0 && q.has_run == 0;
    if (q.has_run) CHECK(jj_valid(&back), "damaged save gave a broken run");
  }
  CHECK(refused == (int)n, "damaged saves refused: %d of %u", refused, n);
  /* too short, too long, empty */
  plat_save(JJ_SAVE_NAME, keep, n - 1), save_load(&q, &back);
  CHECK(q.best == 0, "short save");
  printf("saves: %u bytes, every damaged byte refused\n", n);
}

/* ---------------------------------------------------------------- the whole app with random keys */
static uint32_t fuzz_rng = 77;
static uint32_t fuzz_keys(uint32_t f) {
  static uint32_t held, until;
  if (f >= until) {
    uint32_t x = jj_rand(&fuzz_rng);
    static const uint32_t pick[] = {0, JK_OK, JK_OK, JK_OK, JK_UP, JK_DOWN, JK_BACK, JK_OK | JK_UP, 0, JK_OK};
    held = pick[x % 10];
    until = f + 1 + (x >> 8) % 40;
  }
  return held;
}
static void fuzz(void) {
  int sessions = 200;
  for (int s = 0; s < sessions; s++) {
    if (s % 50 == 0) forget(JJ_SAVE_NAME), forget(JJ_COPY_NAME);
    memset(&run, 0, sizeof run), memset(&prof, 0, sizeof prof), memset(&view, 0, sizeof view);
    fuzz_rng = (uint32_t)s * 7919u + 1;
    script = fuzz_keys, frame_no = 0, quit_at = 100 + jj_rand(&fuzz_rng) % 1200;
    jj_app_main();
    uint32_t n;
    const uint8_t *d = plat_load(JJ_SAVE_NAME, &n);
    CHECK(d && n == sizeof(jj_file), "session %d: no save", s);
    if (d) {
      jj_file f;
      memcpy(&f, d, sizeof f);
      CHECK(jj_file_ok(&f), "session %d: the save doesn't read back", s);
    }
  }
  printf("fuzz: %d sessions of random keys, each quit with Home and its save read back\n", sessions);
}

/* ---------------------------------------------------------------- the Final Spin, through the app */
static int spin_wins, spin_losses, spin_revives;
static int8_t last_msg;
static uint8_t last_screen;
static uint32_t spin_keys(uint32_t f) {
  if (view.spin_msg != last_msg) {
    if (view.spin_msg >= 0) spin_wins++;
    if (view.spin_msg == -2) spin_losses++;
  }
  if (last_screen == JJ_SCREEN_SPIN && view.screen == JJ_SCREEN_PLAY) spin_revives++;
  last_msg = view.spin_msg, last_screen = view.screen;
  if (f == 10) return JK_BACK;                       /* resume the run left with Home */
  return (f % 8) < 2 && f > 20 ? JK_OK : 0;          /* then OK: spins, results, play again... */
}
static void spins(void) {
  jj_run end;
  for (uint32_t seed = 3;; seed++) {   /* a run to its end */
    jj_start(&end, seed * 40503u, 0, 0);
    bot_left = 0;
    while (end.state == JJ_ALIVE) jj_step(&end, bot(&end));
    break;
  }
  for (int i = 0; i < 5000 && end.state != JJ_STOPPED; i++) jj_step(&end, false);
  int sessions = 60;
  spin_wins = spin_losses = spin_revives = 0;
  for (int s = 0; s < sessions; s++) {
    jj_profile p = {100, 1000, (uint32_t)s * 17, 1, 0, 0, 1};
    end.tokens = 6;
    forget(JJ_SAVE_NAME), forget(JJ_COPY_NAME);
    save_write(&p, &end);
    memset(&run, 0, sizeof run), memset(&prof, 0, sizeof prof), memset(&view, 0, sizeof view);
    script = spin_keys, frame_no = 0, quit_at = 900, last_msg = -1, last_screen = 0;
    jj_app_main();
    uint32_t n;
    const uint8_t *d = plat_load(JJ_SAVE_NAME, &n);
    jj_file f;
    CHECK(d && n == sizeof f, "spin session %d: no save", s);
    if (!d) continue;
    memcpy(&f, d, sizeof f);
    CHECK(jj_file_ok(&f), "spin session %d: the save doesn't read back", s);
    /* the tokens were all spent or cashed in: the coins grew, or a run came back */
    CHECK(f.p.coins > 1000 || f.p.has_run || f.p.head_start || f.p.double_coins, "spin session %d: nothing happened", s);
  }
  int spins = spin_wins + spin_losses;
  CHECK(spins > sessions * 5 && spin_wins * 5 > spins && spin_wins * 3 < spins, "spins %d, wins %d", spins, spin_wins);
  CHECK(spin_revives > 0, "no Second Chance brought a run back");
  printf("final spin: %d spins, %d won, %d Second Chances back to the run\n", spins, spin_wins, spin_revives);
}

/* ---------------------------------------------------------------- pictures */
static void picture(const char *dir, const char *name) {
  char path[512];
  snprintf(path, sizeof path, "%s/%s.ppm", dir, name);
  sync_view();
  jj_draw(&run, &view);
  shot(path);
}

/* the bot plays until `want` is true, then a picture */
static bool until_seen(uint32_t seed, bool (*want)(const jj_run *), int max_frames) {
  jj_start(&run, seed, 0, 0);
  bot_left = 0;
  for (int i = 0; i < max_frames && run.state == JJ_ALIVE; i++) {
    jj_step(&run, bot(&run));
    if (want(&run)) return true;
  }
  return false;
}
static bool w_laser(const jj_run *r) {
  if (!r->laser) return false;
  int y, ph, k;
  jj_beam_state(r->laser - 1, 0, r->laser_t, &y, &ph, &k);
  return ph == 2 && k > 10 && jj_lpats[r->laser - 1].count >= 3;
}
static bool w_missile(const jj_run *r) {
  for (int i = 0; i < JJ_MISSILES; i++)
    if (r->mis[i].on && r->mis[i].state == JJ_FLY && r->mis[i].x < 380 && r->mis[i].x > 250) return true;
  return false;
}
static bool w_warn(const jj_run *r) {
  int n = 0;
  for (int i = 0; i < JJ_MISSILES; i++) n += r->mis[i].on && r->mis[i].state != JJ_FLY;
  return n >= 1 && r->coin[0].on;
}
static bool w_rotating(const jj_run *r) {
  for (int i = 0; i < JJ_ZAPPERS; i++)
    if (r->zap[i].on && r->zap[i].spin && r->zap[i].x > 200 && r->zap[i].x < 380) return true;
  return false;
}
static bool w_coins(const jj_run *r) {
  for (int i = 0; i < JJ_COINSETS; i++)
    if (r->coin[i].on && r->coin[i].x < 300 && r->coin[i].x > 150) return true;
  return false;
}
static int want_ride;
static bool w_ride(const jj_run *r) { return r->ride == want_ride && r->t % 7 == 0 && r->ride_speed > 0 && r->next < 200; }
static bool w_token(const jj_run *r) { return r->token[0].on && r->token[0].x < 400; }
static bool w_box(const jj_run *r) { return r->box.on && r->box.x < 420; }

static void shots(const char *dir) {
  memset(&prof, 0, sizeof prof);
  prof.best = 2380, prof.coins = 1264;
  memset(&view, 0, sizeof view);
  view.screen = JJ_SCREEN_TITLE, view.clock = 200;
  picture(dir, "title");
  view.screen = JJ_SCREEN_PLAY;
  struct { const char *name; bool (*w)(const jj_run *); } cases[] = {
      {"coins", w_coins}, {"rotating", w_rotating}, {"warning", w_warn}, {"missile", w_missile},
      {"laser", w_laser}, {"token", w_token}, {"box", w_box}};
  for (unsigned c = 0; c < sizeof cases / sizeof cases[0]; c++) {
    bool ok = false;
    for (uint32_t seed = 1; seed < 400 && !ok; seed++) ok = until_seen(seed * 2654435761u, cases[c].w, 60 * 300);
    CHECK(ok, "no %s seen", cases[c].name);
    picture(dir, cases[c].name);
  }
  static const char *const rides[JJ_RIDES] = {"", "stomper", "hog", "bird", "gravity", "teleporter"};
  for (want_ride = 1; want_ride < JJ_RIDES; want_ride++) {
    bool ok = false;
    for (uint32_t seed = 1; seed < 400 && !ok; seed++) ok = until_seen(seed * 2246822519u, w_ride, 60 * 300);
    CHECK(ok, "no %s seen", rides[want_ride]);
    picture(dir, rides[want_ride]);
  }
  view.screen = JJ_SCREEN_PAUSE, view.sel = 0;
  picture(dir, "pause");
  view.screen = JJ_SCREEN_GUIDE;
  picture(dir, "guide");
  /* a run to its end, then the Final Spin and the results */
  for (uint32_t seed = 3;; seed++) {
    jj_start(&run, seed * 40503u, 0, 0);
    bot_left = 0;
    while (run.state == JJ_ALIVE) jj_step(&run, bot(&run));
    if (run.tokens) break;
  }
  for (int i = 0; i < 5000 && !jj_over(&run); i++) jj_step(&run, false);
  view.screen = JJ_SCREEN_SPIN, view.spinning = 0, view.spin_msg = JJ_PRIZE_REVIVE;
  view.reel[0] = view.reel[1] = view.reel[2] = 48 * JJ_PRIZE_REVIVE;
  picture(dir, "spin");
  view.screen = JJ_SCREEN_RESULTS, view.sel = 0, view.run_m = jj_metres(&run), view.run_coins = run.coins, view.new_best = 1;
  picture(dir, "results");
}

static void film(const char *dir, int seed, int start, int frames) {
  memset(&view, 0, sizeof view);
  view.screen = JJ_SCREEN_PLAY;
  jj_start(&run, (uint32_t)seed * 2654435761u, 0, 0);
  bot_left = 0, greedy = true;
  char path[512];
  for (int f = -start; f < frames; f++) {
    for (int k = 0; k < 3; k++) jj_step(&run, run.state == JJ_ALIVE ? bot(&run) : false);
    if (f < 0) continue;
    snprintf(path, sizeof path, "%s/f_%04d.ppm", dir, f);
    sync_view();
    jj_draw(&run, &view);
    shot(path);
  }
}

int main(int argc, char **argv) {
  gfx_buf = strip;
  if (argc >= 2 && !strcmp(argv[1], "test")) {
    clock_t c0 = clock();
    units();
    bots();
    clock_t c1 = clock();
    saves();
    spins();
    fuzz();
    fprintf(stderr, "(%.0f s rules and bots, %.0f s saves and keys)\n", (double)(c1 - c0) / CLOCKS_PER_SEC,
            (double)(clock() - c1) / CLOCKS_PER_SEC);
    printf(fails ? "%d FAILED\n" : "all passed\n", fails);
    return fails != 0;
  }
  if (argc >= 3 && !strcmp(argv[1], "shots")) {
    shots(argv[2]);
    return fails != 0;
  }
  if (argc >= 6 && !strcmp(argv[1], "film")) {
    film(argv[2], atoi(argv[3]), atoi(argv[4]), atoi(argv[5]));
    return 0;
  }
  fprintf(stderr, "usage: play test | shots DIR | film DIR SEED START FRAMES\n");
  return 2;
}
