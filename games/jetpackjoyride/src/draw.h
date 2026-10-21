/* What the screens show, besides the run itself. */
#ifndef JJ_DRAW_H
#define JJ_DRAW_H
#include "jj.h"

enum { JJ_SCREEN_TITLE, JJ_SCREEN_GUIDE, JJ_SCREEN_PLAY, JJ_SCREEN_PAUSE, JJ_SCREEN_CONFIRM, JJ_SCREEN_SPIN, JJ_SCREEN_RESULTS };

typedef struct {
  uint8_t screen, sel;
  uint32_t clock;                /* the title's lab scrolls with it */
  uint32_t best, coins;          /* the records */
  uint8_t head_start, double_coins;
  /* the Final Spin */
  uint8_t tokens, spinning;
  int8_t spin_msg;               /* the prize won, -2 no luck, -1 nothing yet */
  uint16_t reel[3];
  /* the results */
  uint32_t run_m, run_coins;
  uint8_t new_best;
} jj_view;

void jj_draw(const jj_run *r, const jj_view *v);
#endif
