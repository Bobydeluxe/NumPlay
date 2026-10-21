/* The run: Barry, his vehicles, and what the level pacing sends at him, 60 times a second, with the
 * original's numbers (data.h). Where the files don't say how a number is used, the choice is the spec's
 * (marked [P] there). */
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "jj.h"
#include "data.h"

#define FRAMES_SAFE ((JJ_IMMUNITY_MS * 60 + 999) / 1000)
#define WARN_FRAMES 75
#define LOCK_FRAMES 30
#define CHARGE 90
#define SLIDE 20
#define GAP 72

/* ---------------------------------------------------------------- small things */
uint32_t jj_rand(uint32_t *s) {
  uint32_t x = *s;
  x ^= x << 13, x ^= x >> 17, x ^= x << 5;
  return *s = x ? x : 0x2545F491u;
}
static int range(jj_run *r, int a, int b) { return b <= a ? a : a + (int)(jj_rand(&r->rng) % (uint32_t)(b - a + 1)); }

static const int16_t quarter[65] = {
  0, 25, 50, 75, 100, 125, 150, 175, 200, 224, 249, 273, 297, 321, 345, 369, 392, 415, 438, 460, 483, 505,
  526, 548, 569, 590, 610, 630, 650, 669, 688, 706, 724, 742, 759, 775, 792, 807, 822, 837, 851, 865, 878,
  891, 903, 915, 926, 936, 946, 955, 964, 972, 980, 987, 993, 999, 1004, 1009, 1013, 1016, 1019, 1021, 1023,
  1024, 1024};
int jj_sin(uint16_t ang) {
  unsigned i = ang >> 8;   /* 256 steps a turn */
  unsigned q = i >> 6, k = i & 63;
  int v = q & 1 ? quarter[64 - k] : quarter[k];
  return q & 2 ? -v : v;
}
int jj_cos(uint16_t ang) { return jj_sin((uint16_t)(ang + 16384)); }

uint32_t jj_metres(const jj_run *r) { return r->dist / (16 * JJ_DIST_DIV); }

static int weight(unsigned sched, uint32_t metres) {
  int w = 0;
  for (unsigned i = sched; jj_sched[2 * i] != 0xffff; i++)
    if (jj_sched[2 * i] <= metres) w = jj_sched[2 * i + 1];
  return w;
}

float jj_speed(const jj_run *r) {
  if (r->ride != JJ_FOOT) return r->ride_speed;
  float m = (float)r->dist / (16.0f * JJ_DIST_DIV);
  float k = m >= JJ_SPEED_DIST ? 1.0f : m / JJ_SPEED_DIST;
  return JJ_START_SPEED + (JJ_END_SPEED - JJ_START_SPEED) * k;
}

int jj_box_h(int ride) {
  static const uint8_t h[JJ_RIDES] = {30, 56, 40, 40, 40, 48};
  return h[ride];
}
int jj_box_w(int ride) {
  static const uint8_t w[JJ_RIDES] = {16, 44, 50, 50, 24, 34};
  return w[ride];
}

int jj_coin_at(const jj_coinset *c, int col, int row) {
  int w = jj_pat_w[c->pat];
  if (col < 0 || row < 0 || col >= w || row >= jj_pat_h[c->pat]) return 0;
  return (c->bits[row * ((w + 7) / 8) + col / 8] >> (col & 7)) & 1;
}

/* ---------------------------------------------------------------- lasers
 * A laser charges 90 frames, fires for its on interval (60 for the single shots), and fires again its off
 * interval after a shot ends, if that shot ends before the pattern does. */
void jj_beam_state(int pattern, int beam, int t, int *y, int *phase, int *k) {
  const jj_lpat_t *p = &jj_lpats[pattern];
  const jj_beam_t *b = &jj_beams[p->first + beam];
  int on = b->on > 1 ? b->on : 60;
  /* the height: from y0 to y1 over `move` frames */
  int yy = b->y0;
  if (b->move) {
    int tt = t < 0 ? 0 : t > b->move ? b->move : t;
    yy = b->y0 + (b->y1 - b->y0) * tt / b->move;
  }
  *y = JJ_MID - yy;
  *phase = 0, *k = 0;
  int fire = b->wait + CHARGE;
  while (fire + on <= p->time) {
    if (t < fire - CHARGE) return;
    if (t < fire) {
      *phase = 1, *k = t - (fire - CHARGE);
      return;
    }
    if (t < fire + on) {
      *phase = 2, *k = t - fire;
      return;
    }
    if (b->off == 0 || b->off >= 10000) return;
    int next = fire + on + b->off;
    fire = next - fire - on < CHARGE ? fire + on + CHARGE : next;
  }
}

/* ---------------------------------------------------------------- spawning */
static float place(jj_run *r, int lo, int hi, const jj_file_t *f) {
  if (hi < lo) hi = lo;
  int mid = (lo + hi) / 2, p = range(r, 0, 99);
  if (p < f->top) return (float)lo;
  p -= f->top;
  if (p < f->bottom) return (float)hi;
  p -= f->bottom;
  if (p < f->tophalf) return (float)range(r, lo, mid);
  p -= f->tophalf;
  if (p < f->bottomhalf) return (float)range(r, mid, hi);
  return (float)range(r, lo, hi);
}

static int pick(jj_run *r, const uint16_t *sched_of, int first, int count, uint32_t m) {
  int total = 0;
  for (int i = 0; i < count; i++) total += weight(sched_of[first + i], m);
  if (total <= 0) return -1;
  int x = range(r, 0, total - 1);
  for (int i = 0; i < count; i++) {
    x -= weight(sched_of[first + i], m);
    if (x < 0) return first + i;
  }
  return first + count - 1;
}

static float spawn_zapper(jj_run *r, int file, uint32_t m) {
  const jj_file_t *f = &jj_zap_files[file];
  int total = 0, i;
  for (i = 0; i < f->count; i++) total += weight(jj_zaps[f->first + i].sched, m);
  if (total <= 0) return 0;
  int x = range(r, 0, total - 1);
  for (i = 0; i < f->count - 1; i++) {
    x -= weight(jj_zaps[f->first + i].sched, m);
    if (x < 0) break;
  }
  const jj_zap_t *z = &jj_zaps[f->first + i];
  for (int k = 0; k < JJ_ZAPPERS; k++) {
    jj_zapper *s = &r->zap[k];
    if (s->on) continue;
    memset(s, 0, sizeof *s);
    s->on = 1;
    s->len = (uint16_t)range(r, z->min, z->max);
    s->ang = (uint16_t)((z->angle % 360) * 65536 / 360);
    if (z->rotate) {
      s->spin = (int16_t)(65536 / (z->rotate < 0 ? -z->rotate : z->rotate));
      if (z->rotate < 0) s->spin = (int16_t)-s->spin;
      s->ang = (uint16_t)jj_rand(&r->rng);
    }
    int ext_y = z->rotate ? s->len : (abs(jj_sin(s->ang)) * s->len) >> 10;
    int ext_x = z->rotate ? s->len : (abs(jj_cos(s->ang)) * s->len) >> 10;
    /* where it fits; one that's too long for the room left is pushed into the floor or the ceiling, so a gap of
       GAP points stays open (the files don't say, the spec's choice) */
    int half = ext_y / 2 + 14;
    int lo = JJ_CEIL + half, hi = JJ_FLOOR - half;
    if (hi - lo < 0 || (JJ_FLOOR - JJ_CEIL) - 2 * half < GAP) {
      int top = jj_rand(&r->rng) & 1;
      lo = hi = top ? JJ_FLOOR - GAP - half : JJ_CEIL + GAP + half;
    }
    s->y = place(r, lo, hi, f);
    s->x = JJ_SPAWN_X + ext_x / 2.0f;
    return (float)ext_x + 28;
  }
  return 0;
}

static float spawn_coins(jj_run *r, int file, uint32_t m) {
  const jj_file_t *f = &jj_coin_files[file];
  int p = pick(r, jj_coin_pick_sched, f->first, f->count, m);
  if (p < 0) return 0;
  int pat = jj_coin_pick[p];
  for (int k = 0; k < JJ_COINSETS; k++) {
    jj_coinset *c = &r->coin[k];
    if (c->on) continue;
    memset(c, 0, sizeof *c);
    c->on = 1, c->pat = (uint8_t)pat;
    int w = jj_pat_w[pat], h = jj_pat_h[pat], nb = (w + 7) / 8;
    memcpy(c->bits, jj_pat_bits + jj_pat_off[pat], (size_t)(nb * h));
    c->y = (int16_t)place(r, JJ_CEIL + 6, JJ_FLOOR - 10 - (h - 1) * JJ_COIN_GAP, f);
    c->x = JJ_SPAWN_X;
    return (float)(w * JJ_COIN_GAP);
  }
  return 0;
}

static float spawn_missile(jj_run *r, int file) {
  for (int k = 0; k < JJ_MISSILES; k++) {
    jj_missile *s = &r->mis[k];
    if (s->on) continue;
    memset(s, 0, sizeof *s);
    s->on = 1, s->state = JJ_WARN;
    s->chase = jj_mis_files[file].chase;
    s->y = r->y - jj_box_h(r->ride) / 2.0f;
    s->x = JJ_W - 18;
    return 0;
  }
  return 0;
}

static float spawn_token(jj_run *r, uint32_t m) {
  for (int k = 0; k < JJ_TOKENS; k++) {
    jj_token *s = &r->token[k];
    if (s->on) continue;
    s->on = 1;
    s->x = JJ_SPAWN_X;
    s->y = (float)range(r, JJ_CEIL + 70, JJ_FLOOR - 70);
    s->phase = (uint16_t)jj_rand(&r->rng);
    if (m / JJ_TOKEN_KM != r->token_km) r->token_km = m / JJ_TOKEN_KM, r->token_count = 0;
    r->token_count++;
    r->token_last = m + JJ_TOKEN_MAX_GAP;   /* the next one is due by then */
    return 40;
  }
  return 0;
}

static void spawn_scientist(jj_run *r) {
  for (int k = 0; k < JJ_SCIENTISTS; k++) {
    jj_scientist *s = &r->sci[k];
    if (s->on) continue;
    s->on = 1, s->state = 0, s->t = 0;
    s->x = JJ_SPAWN_X;
    s->dir = (int8_t)(jj_rand(&r->rng) & 1 ? 1 : -1);
    return;
  }
}

static void spawn(jj_run *r) {
  uint32_t m = jj_metres(r);
  const jj_set_t *set = &jj_sets[r->set];
  float width = 0;
  int imin, imax;
  if (r->set == JJ_SET_DEFAULT) {
    int i = 0;
    while (i + 1 < JJ_PACE_STEPS && jj_pace_from[i + 1] <= m) i++;
    imin = jj_pace_min[i], imax = jj_pace_max[i];
  } else {
    imin = set->imin, imax = set->imax;
  }
  if (range(r, 0, 99) < 40) spawn_scientist(r);   /* workerinfo.xml spawnProbability 40 */
  /* a spin token that's due */
  bool token_ok = m >= JJ_TOKEN_FIRST_MIN && (m / JJ_TOKEN_KM != r->token_km || r->token_count < JJ_TOKEN_PER_KM);
  if (token_ok && m >= r->token_last) {
    width = spawn_token(r, m);
    r->next = (float)range(r, imin, imax) + width;
    return;
  }
  if ((r->set == JJ_SET_MISSILE || r->set == JJ_SET_COIN) && r->streak) {
    if (r->set == JJ_SET_MISSILE) spawn_missile(r, set->mis);
    else width = spawn_coins(r, set->coin, m);
    r->next = (float)range(r, imin, imax) + width;
    if (!--r->streak) r->set = JJ_SET_DEFAULT;
    return;
  }
  /* weights: zapper, missile, coins, lasers, token, missile swarm, coin streak */
  int w[7] = {0};
  if (set->zap != 255) w[0] = weight(jj_zap_files[set->zap].sched, m);
  if (set->mis != 255) w[1] = weight(jj_mis_files[set->mis].sched, m);
  if (set->coin != 255) w[2] = weight(jj_coin_files[set->coin].sched, m);
  if (r->set == JJ_SET_DEFAULT) {
    w[3] = weight(JJ_LASER_SCHED, m);
    w[5] = jj_sets[JJ_SET_MISSILE].prob;
    w[6] = jj_sets[JJ_SET_COIN].prob;
  }
  if (token_ok) w[4] = JJ_TOKEN_WEIGHT;
  int total = 0;
  for (int i = 0; i < 7; i++) total += w[i];
  int x = total > 0 ? range(r, 0, total - 1) : 0, kind = 0;
  while (kind < 6 && x >= w[kind]) x -= w[kind++];
  switch (kind) {
  case 0: width = spawn_zapper(r, set->zap, m); break;
  case 1: width = spawn_missile(r, set->mis); break;
  case 2: width = spawn_coins(r, set->coin, m); break;
  case 3: {
    int total3 = 0, p;
    for (p = 0; p < JJ_LASER_PATTERNS; p++) total3 += weight(jj_lpats[p].sched, m);
    if (total3 <= 0) break;
    int y = range(r, 0, total3 - 1);
    for (p = 0; p < JJ_LASER_PATTERNS - 1; p++) {
      y -= weight(jj_lpats[p].sched, m);
      if (y < 0) break;
    }
    r->laser = (uint8_t)(p + 1), r->laser_t = 0;
    break;
  }
  case 4: width = spawn_token(r, m); break;
  case 5: r->set = JJ_SET_MISSILE, r->streak = (uint8_t)range(r, jj_sets[JJ_SET_MISSILE].minc, jj_sets[JJ_SET_MISSILE].maxc); break;
  case 6: r->set = JJ_SET_COIN, r->streak = (uint8_t)range(r, jj_sets[JJ_SET_COIN].minc, jj_sets[JJ_SET_COIN].maxc); break;
  }
  r->next = kind >= 5 ? 0 : (float)range(r, imin, imax) + width;
}

/* ---------------------------------------------------------------- the run */
void jj_start(jj_run *r, uint32_t seed, bool double_coins, bool head_start) {
  memset(r, 0, sizeof *r);
  r->rng = seed ? seed : 0x4A4A4A4Au;
  r->y = JJ_FLOOR;
  r->ground = 1;
  r->double_coins = double_coins;
  r->head_start = head_start;
  uint32_t m0 = head_start ? 500 : 0;
  r->dist = m0 * 16 * JJ_DIST_DIV;
  if (head_start) r->safe = 120;
  r->next = 300;
  r->token_last = m0 + (uint32_t)range(r, JJ_TOKEN_FIRST_MIN, JJ_TOKEN_FIRST_MAX);
  r->token_km = m0 / JJ_TOKEN_KM;
  r->vehicle_at = m0 + (uint32_t)range(r, 50, JJ_VEHICLE_RANGE);
}

bool jj_over(const jj_run *r) { return r->state == JJ_STOPPED && r->stop_t >= 45; }

static void boom(jj_run *r, float x, float y, int kind) {
  int k = 0;   /* a free one, else the oldest */
  for (int i = 0; i < JJ_FX; i++) {
    if (!r->fx[i].t) {
      k = i;
      break;
    }
    if (r->fx[i].t > r->fx[k].t) k = i;
  }
  r->fx[k] = (jj_fx){x, y, (uint8_t)kind, 1};
}

/* everything on screen switched off: a vehicle taken, a Second Chance */
static void clear_screen(jj_run *r) {
  for (int i = 0; i < JJ_ZAPPERS; i++)
    if (r->zap[i].on && r->zap[i].x < JJ_W + 60) r->zap[i].dud = 1;
  for (int i = 0; i < JJ_MISSILES; i++)
    if (r->mis[i].on) {
      if (r->mis[i].state == JJ_FLY) boom(r, r->mis[i].x, r->mis[i].y, 0);
      r->mis[i].on = 0;
    }
  r->laser = 0;
}

void jj_revive(jj_run *r) {
  clear_screen(r);
  r->state = JJ_ALIVE, r->by = 0, r->stop_t = 0;
  r->ride = JJ_FOOT, r->set = JJ_SET_DEFAULT, r->streak = 0;
  r->vy = -6, r->ground = 0, r->safe = 120, r->dvx = 0;
  r->revived++;
}

static void knock_out(jj_run *r, int by) {
  r->by = (uint8_t)by;
  r->dvx = jj_speed(r) / 60.0f;
  r->ride = JJ_FOOT;
  r->held = 0;
  if (by == JJ_BY_ZAPPER) {
    r->state = JJ_ZAPPED, r->stop_t = 0;
  } else {
    boom(r, JJ_BX, r->y - 20, 0);
    r->state = JJ_TUMBLE, r->vy = -5;
  }
}

/* hit: a vehicle breaks, else Barry's out */
static void hit(jj_run *r, int by) {
  if (r->safe || r->state != JJ_ALIVE) return;
  if (r->ride != JJ_FOOT) {
    boom(r, JJ_BX, r->y - jj_box_h(r->ride) / 2.0f, 1);
    r->ride = JJ_FOOT, r->set = JJ_SET_DEFAULT, r->streak = 0, r->flip = 0;
    r->safe = FRAMES_SAFE + 30;
    r->vy = -4, r->ground = 0;
    r->vehicle_at = jj_metres(r) + (uint32_t)range(r, JJ_VEHICLE_EXCLUSION, JJ_VEHICLE_RANGE);
    return;
  }
  knock_out(r, by);
}

static void take_vehicle(jj_run *r) {
  int total = 0;
  for (int i = 0; i < JJ_RIDES - 1; i++) total += jj_vehicles[i].weight;
  int x = range(r, 0, total - 1), v = 0;
  while (v < JJ_RIDES - 2 && x >= jj_vehicles[v].weight) x -= jj_vehicles[v++].weight;
  const jj_vehicle_t *d = &jj_vehicles[v];
  float now = jj_speed(r);
  r->ride = (uint8_t)(v + 1);
  r->ride_speed = now > d->start ? now : d->start;
  r->set = d->set, r->streak = 0;
  r->jumps = d->jumps, r->boost = 0, r->flip = 0, r->cft = 0;
  r->vy = 0;
  r->next = 260;
  clear_screen(r);
}

static void barry(jj_run *r, bool held) {
  bool press = held && !r->held;
  int h = jj_box_h(r->ride);
  if (r->ride == JJ_FOOT) {
    if (press) r->vy *= JJ_VSCALAR;   /* a boost starts from half the speed he had (vehicles: from none) */
    r->vy += JJ_GRAVITY;
    if (held) r->vy -= JJ_BOOST;
    r->vy *= JJ_FRICTION;
  } else if (r->ride == JJ_CFT) {
    r->cft = (uint16_t)(r->cft + 65536 * 8 / 360);
    if (r->boost) r->boost--;
    if (press) {
      int ang = jj_sin(r->cft) * 16384 / 1024;   /* +-90 degrees */
      float ty = r->y - h / 2.0f - 96.0f * jj_sin((uint16_t)ang) / 1024.0f;
      if (ty < JJ_CEIL + h / 2.0f) ty = JJ_CEIL + h / 2.0f;
      if (ty > JJ_FLOOR - h / 2.0f) ty = JJ_FLOOR - h / 2.0f;
      r->y = ty + h / 2.0f;
      r->boost = 12;   /* (the Teleporter's teleport frames) */
    }
    r->vy = 0;
  } else if (r->ride == JJ_GRAV) {
    const jj_vehicle_t *d = &jj_vehicles[JJ_GRAV - 1];
    if (press) r->flip ^= 1, r->ground = 0;
    r->vy += r->flip ? -d->gravity : d->gravity;
    r->vy *= d->friction;
  } else {
    const jj_vehicle_t *d = &jj_vehicles[r->ride - 1];
    if (press && r->jumps > 0 && (r->ride == JJ_BIRD || r->ground)) {
      r->vy = -d->initial;
      r->boost = d->frames;
      r->jumps--;
      r->ground = 0;
    }
    if (held && r->ride != JJ_BIRD) {
      if (r->boost) r->vy -= d->boost, r->boost--;
      else if (!r->ground) r->vy -= d->final_boost;
    }
    if (!held) r->boost = 0;
    r->vy += d->gravity;
    r->vy *= d->friction;
  }
  r->held = held;
  r->y += r->vy;
  if (r->y >= JJ_FLOOR) {
    r->y = JJ_FLOOR;
    if (r->vy > 0) r->vy = 0;
    if (!r->ground && r->ride != JJ_FOOT && r->ride != JJ_CFT) r->jumps = jj_vehicles[r->ride - 1].jumps;
    r->ground = !(r->ride == JJ_GRAV && r->flip);
  } else if (r->y - h <= JJ_CEIL) {
    r->y = JJ_CEIL + h;
    if (r->vy < 0) r->vy = 0;
    r->ground = r->ride == JJ_GRAV && r->flip;
  } else {
    r->ground = 0;
  }
}

static bool boxes(float ax, float ay, float aw, float ah, float bx, float by, float bw, float bh) {
  return ax < bx + bw && bx < ax + aw && ay < by + bh && by < ay + ah;
}

static bool zapper_hits(const jj_zapper *z, float bx, float by, float bw, float bh) {
  /* the beam, from orb to orb, 10 points either side */
  float dx = jj_cos(z->ang) * (z->len / 2.0f) / 1024.0f, dy = jj_sin(z->ang) * (z->len / 2.0f) / 1024.0f;
  int n = z->len / 8 + 2;
  for (int i = 0; i <= n; i++) {
    float t = -1.0f + 2.0f * i / n;
    float px = z->x + dx * t, py = z->y + dy * t;
    float rad = (i == 0 || i == n) ? 14.0f : 9.0f;
    if (px + rad > bx && px - rad < bx + bw && py + rad > by && py - rad < by + bh) return true;
  }
  return false;
}

void jj_step(jj_run *r, bool held) {
  r->t++;
  for (int i = 0; i < JJ_FX; i++)
    if (r->fx[i].t && ++r->fx[i].t > 32) r->fx[i].t = 0;
  float dx;
  if (r->state == JJ_ALIVE) {
    barry(r, held);
    if (r->safe) r->safe--;
    if (r->ride != JJ_FOOT) {
      const jj_vehicle_t *d = &jj_vehicles[r->ride - 1];
      r->ride_speed += d->increase;
      if (r->ride_speed > d->max) r->ride_speed = d->max;
    }
    dx = jj_speed(r) / 60.0f;
  } else {
    /* knocked out: zapped a moment, then he tumbles and slides to a stop */
    if (r->state == JJ_ZAPPED) {
      if (++r->stop_t >= 24) r->state = JJ_TUMBLE, r->stop_t = 0, r->vy = -3;
    } else {
      r->vy += JJ_DEATH_GRAVITY;
      r->y += r->vy;
      if (r->y >= JJ_FLOOR) {
        r->y = JJ_FLOOR;
        r->vy = r->vy > JJ_MIN_BOUNCE ? -r->vy * 0.5f : 0;
        if (r->vy == 0) r->dvx *= JJ_DEATH_GROUND_FRICTION;
      }
      if (r->state == JJ_TUMBLE && r->vy == 0 && r->y >= JJ_FLOOR && r->dvx < 0.3f) r->state = JJ_STOPPED, r->dvx = 0;
      if (r->state == JJ_STOPPED && r->stop_t < 60000) r->stop_t++;
    }
    dx = r->dvx;
  }
  r->dist += (uint32_t)(dx * 16.0f + 0.5f);
  uint32_t m = jj_metres(r);

  /* things move */
  for (int i = 0; i < JJ_ZAPPERS; i++) {
    jj_zapper *z = &r->zap[i];
    if (!z->on) continue;
    z->x -= dx;
    z->ang = (uint16_t)(z->ang + z->spin);
    if (z->x < -z->len / 2 - 40) {
      z->on = 0;
      if (r->state == JJ_ALIVE) r->zappers_dodged++;
    }
  }
  for (int i = 0; i < JJ_COINSETS; i++)
    if (r->coin[i].on && (r->coin[i].x -= dx) < -(float)jj_pat_w[r->coin[i].pat] * JJ_COIN_GAP - 20) r->coin[i].on = 0;
  for (int i = 0; i < JJ_TOKENS; i++) {
    jj_token *t = &r->token[i];
    if (!t->on) continue;
    t->x -= dx;
    t->phase = (uint16_t)(t->phase + 300);
    if (t->x < -30) t->on = 0;
  }
  if (r->box.on) {
    r->box.x -= dx;
    r->box.phase = (uint16_t)(r->box.phase + 600);
    if (r->box.x < -40) r->box.on = 0, r->vehicle_at = m + (uint32_t)range(r, JJ_VEHICLE_EXCLUSION, JJ_VEHICLE_RANGE);
  }
  for (int i = 0; i < JJ_FX; i++)
    if (r->fx[i].t) r->fx[i].x -= dx;
  for (int i = 0; i < JJ_SCIENTISTS; i++) {
    jj_scientist *s = &r->sci[i];
    if (!s->on) continue;
    s->t++;
    float walk = s->state == 2 ? 0 : s->state == 1 ? 3.0f : 1.0f;
    s->x += s->dir * walk - dx;
    /* alerted when Barry comes near (workerinfo.xml: 200 to 500 points), some run */
    if (s->state == 0 && s->x - JJ_BX < 300 && s->x > JJ_BX && (s->t & 31) == 0 && range(r, 0, 99) < 20) s->state = 1, s->dir = 1;
    /* the jetpack's bullets knock them down */
    if (s->state != 2 && r->state == JJ_ALIVE && r->ride == JJ_FOOT && r->held && s->x > JJ_BX - 30 && s->x < JJ_BX + 4 &&
        JJ_FLOOR - r->y < 150)
      s->state = 2, s->t = 0;
    if (s->x < -30 || s->x > JJ_SPAWN_X + 40) s->on = 0;
  }
  for (int i = 0; i < JJ_MISSILES; i++) {
    jj_missile *s = &r->mis[i];
    if (!s->on) continue;
    s->t++;
    float target = r->y - jj_box_h(r->ride) / 2.0f;
    if (s->state == JJ_WARN) {
      if (r->state == JJ_ALIVE) s->y += s->y < target - s->chase ? s->chase : s->y > target + s->chase ? -s->chase : target - s->y;
      if (s->t >= WARN_FRAMES) s->state = JJ_LOCK, s->t = 0;
    } else if (s->state == JJ_LOCK) {
      if (s->t >= LOCK_FRAMES) s->state = JJ_FLY, s->t = 0, s->x = JJ_W + 30;
    } else {
      s->x -= jj_mis_files[0].speed + dx;
      if (s->x < -40) {
        s->on = 0;
        if (r->state == JJ_ALIVE) r->missiles_dodged++;
      }
    }
  }
  if (r->laser) {
    r->laser_t++;
    if (r->laser_t > jj_lpats[r->laser - 1].time + SLIDE) r->laser = 0, r->next = 120;
  }

  if (r->state != JJ_ALIVE) {
    /* coins still count while he slides */
    goto coins;
  }

  /* what comes next */
  if (!r->laser) {
    r->next -= dx;
    if (r->next <= 0) spawn(r);
    if (r->ride == JJ_FOOT && !r->box.on && m >= r->vehicle_at && !r->laser) {
      r->box.on = 1, r->box.x = JJ_SPAWN_X;
      r->box.y = (float)range(r, JJ_CEIL + 50, JJ_MID - 10);
      r->box.phase = 0;
      r->vehicle_at = 0xffffffffu;
    }
  }

  /* collisions */
  {
    int w = jj_box_w(r->ride), h = jj_box_h(r->ride);
    float bx = JJ_BX - w / 2.0f, by = r->y - h;
    for (int i = 0; i < JJ_ZAPPERS; i++) {
      jj_zapper *z = &r->zap[i];
      if (z->on && !z->dud && z->x - z->len / 2 - 20 < bx + w && z->x + z->len / 2 + 20 > bx && zapper_hits(z, bx, by, w, h)) {
        if (!r->safe && r->ride != JJ_FOOT) z->dud = 1;
        hit(r, JJ_BY_ZAPPER);
        if (r->state != JJ_ALIVE) break;
      }
    }
    for (int i = 0; i < JJ_MISSILES && r->state == JJ_ALIVE; i++) {
      jj_missile *s = &r->mis[i];
      if (s->on && s->state == JJ_FLY && boxes(s->x - 14, s->y - 6, 28, 12, bx, by, w, h)) {
        if (!r->safe) s->on = 0, boom(r, s->x, s->y, 0);
        hit(r, JJ_BY_MISSILE);
      }
    }
    if (r->laser && r->state == JJ_ALIVE) {
      const jj_lpat_t *p = &jj_lpats[r->laser - 1];
      for (int b = 0; b < p->count; b++) {
        int y, phase, k;
        jj_beam_state(r->laser - 1, b, r->laser_t, &y, &phase, &k);
        if (phase == 2 && k >= 4 && y + 7 > by && y - 7 < by + h) {
          hit(r, JJ_BY_LASER);
          break;
        }
      }
    }
    for (int i = 0; i < JJ_TOKENS; i++) {
      jj_token *t = &r->token[i];
      if (!t->on) continue;
      float ty = t->y + 60.0f * jj_sin(t->phase) / 1024.0f;
      if (boxes(t->x - 12, ty - 12, 24, 24, bx, by, w, h)) {
        t->on = 0;
        if (r->tokens < 99) r->tokens++;
        r->tokens_taken++;
      }
    }
    if (r->box.on && r->ride == JJ_FOOT) {
      float yy = r->box.y + 6.0f * jj_sin(r->box.phase) / 1024.0f;
      if (boxes(r->box.x - 20, yy - 20, 40, 40, bx, by, w, h)) {
        r->box.on = 0;
        take_vehicle(r);
      }
    }
  }
coins: {
  int w = jj_box_w(r->ride), h = jj_box_h(r->ride);
  if (r->state != JJ_ALIVE) w = 24, h = 24;
  float bx = JJ_BX - w / 2.0f, by = r->y - h;
  for (int i = 0; i < JJ_COINSETS; i++) {
    jj_coinset *c = &r->coin[i];
    if (!c->on) continue;
    int c0 = (int)floorf((bx - 8 - c->x) / JJ_COIN_GAP + 0.5f), c1 = (int)floorf((bx + w + 8 - c->x) / JJ_COIN_GAP + 0.5f);
    int r0 = (int)floorf((by - 8 - c->y) / JJ_COIN_GAP + 0.5f), r1 = (int)floorf((by + h + 8 - c->y) / JJ_COIN_GAP + 0.5f);
    for (int row = r0; row <= r1; row++)
      for (int col = c0; col <= c1; col++)
        if (jj_coin_at(c, col, row)) {
          float cx = c->x + col * JJ_COIN_GAP, cy = c->y + row * JJ_COIN_GAP;
          if (cx + 8 > bx && cx - 8 < bx + w && cy + 8 > by && cy - 8 < by + h) {
            int pw = (jj_pat_w[c->pat] + 7) / 8;
            c->bits[row * pw + col / 8] &= (uint8_t)~(1u << (col & 7));
            uint32_t add = r->double_coins ? 2 : 1;
            if (r->coins < 99999999u) r->coins += add;
          }
        }
  }
}
}

/* ---------------------------------------------------------------- saves read back */
static bool finite_in(float v, float lo, float hi) { return v == v && v >= lo && v <= hi; }

bool jj_valid(const jj_run *r) {
  if (!finite_in(r->y, JJ_CEIL, JJ_FLOOR + 1) || !finite_in(r->vy, -60, 60) || !finite_in(r->dvx, 0, 120) ||
      !finite_in(r->ride_speed, 0, 6001) || !finite_in(r->next, -200, 2000))
    return false;
  if (r->held > 1 || r->ground > 1 || r->state > JJ_STOPPED || r->by > JJ_BY_LASER || r->ride >= JJ_RIDES || r->flip > 1 ||
      r->set > JJ_SET_CFT || r->laser > JJ_LASER_PATTERNS || r->tokens > 99 || r->double_coins > 1 || r->head_start > 1 ||
      r->jumps > 10 || r->boost > 40 || r->safe > 400)
    return false;
  if (r->laser && r->laser_t > jj_lpats[r->laser - 1].time + SLIDE + 1) return false;
  for (int i = 0; i < JJ_ZAPPERS; i++) {
    const jj_zapper *z = &r->zap[i];
    if (z->on > 1 || z->dud > 1 || (z->on && (!finite_in(z->x, -400, 900) || !finite_in(z->y, 0, JJ_H) || z->len > 300)))
      return false;
  }
  for (int i = 0; i < JJ_COINSETS; i++) {
    const jj_coinset *c = &r->coin[i];
    if (c->on > 1 || (c->on && (c->pat >= JJ_PATTERNS || !finite_in(c->x, -1000, 900) || c->y < 0 || c->y > JJ_H))) return false;
  }
  for (int i = 0; i < JJ_MISSILES; i++) {
    const jj_missile *s = &r->mis[i];
    if (s->on > 1 || (s->on && (s->state > JJ_FLY || !finite_in(s->x, -200, 900) || !finite_in(s->y, -100, 500) ||
                                !finite_in(s->chase, 0, 10))))
      return false;
  }
  for (int i = 0; i < JJ_TOKENS; i++)
    if (r->token[i].on > 1 || (r->token[i].on && (!finite_in(r->token[i].x, -100, 900) || !finite_in(r->token[i].y, 0, JJ_H))))
      return false;
  if (r->box.on > 1 || (r->box.on && (!finite_in(r->box.x, -100, 900) || !finite_in(r->box.y, 0, JJ_H)))) return false;
  for (int i = 0; i < JJ_FX; i++)
    if (r->fx[i].t > 33 || r->fx[i].kind > 1 || (r->fx[i].t && (!finite_in(r->fx[i].x, -2000, 2000) || !finite_in(r->fx[i].y, -500, 900))))
      return false;
  for (int i = 0; i < JJ_SCIENTISTS; i++)
    if (r->sci[i].on > 1 || r->sci[i].state > 2 || (r->sci[i].on && !finite_in(r->sci[i].x, -100, 1000))) return false;
  return true;
}

/* ---------------------------------------------------------------- the Final Spin */
void jj_spin_start(jj_spin *s) {
  for (int i = 0; i < JJ_PRIZES; i++) s->weight[i] = jj_prize_weight[i];
  s->won = 0;
}

int jj_spin_roll(jj_spin *s, uint32_t *rng) {
  s->won = 0;
  if (jj_rand(rng) % 4) return -1;   /* one token in four wins (the spec, [P]) */
  int total = 0;
  for (int i = 0; i < JJ_PRIZES; i++) total += s->weight[i];
  if (total <= 0) return -1;
  int x = (int)(jj_rand(rng) % (uint32_t)total), p = 0;
  while (p < JJ_PRIZES - 1 && x >= s->weight[p]) x -= s->weight[p++];
  s->weight[p] = (uint16_t)(s->weight[p] * jj_prize_after[p] / 1000);
  s->won = (uint8_t)(p + 1);
  return p;
}
