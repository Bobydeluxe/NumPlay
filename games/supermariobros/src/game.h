/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef SM_GAME_H
#define SM_GAME_H
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include "../vendor/smbcore/mario.h"
#include "../vendor/smbcore/base.h"
#include "../vendor/smbcore/interface.h"
#define SM_SAVE_BYTES 4404
enum { SM_RIGHT=1,SM_LEFT=2,SM_DOWN=4,SM_UP=8,SM_START=16,SM_SELECT=32,SM_RUN=64,SM_JUMP=128 };
typedef struct {
  struct SMB_state state;
  uint32_t ticks;
  uint8_t sprites[256];
  uint8_t keys;
  bool fault;
} sm_game;
extern const uint8_t smb_patterns[8192];
void sm_init(sm_game *g);
bool sm_step(sm_game *g,unsigned keys);
void sm_level(sm_game *g,unsigned world,unsigned level);
void sm_encode(const sm_game *g,uint8_t out[SM_SAVE_BYTES]);
bool sm_decode(sm_game *g,const uint8_t *data,size_t n);
typedef void (*sm_push)(int x,int y,int w,int h,const uint16_t *pixels);
enum { SM_VIEW_GAME,SM_VIEW_PAUSE,SM_VIEW_GUIDE,SM_VIEW_RESTART,SM_VIEW_SAVE_ERROR,SM_VIEW_FAULT };
void sm_render(const sm_game *g,unsigned view,unsigned selected,bool failed,sm_push push);
void sm_invalidate(void);
#endif
