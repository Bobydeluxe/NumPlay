/* SPDX-License-Identifier: GPL-3.0-only
 * NumWorks platform adapter by Bobydeluxe. Core: nukep/SMB-Vanilla.
 */
#include "game.h"
#include <string.h>
#include <setjmp.h>
void smb1_Reset(void);
void smb1_NMI(void);
void smb1_sync_data(void);
void smb1_set_world_and_level(uint8_t world,uint8_t level);
struct SMB_state *SMB_STATE;
static sm_game *current;
#if PLATFORM_DEVICE
/* GCC's embedded context transfer avoids pulling in libc's OS/unwinder exit
 * dependencies. It is confined to this one-thread game-loop recovery point. */
static intptr_t checkpoint[5];
#define SM_SETJMP() __builtin_setjmp(checkpoint)
#define SM_LONGJMP() __builtin_longjmp(checkpoint,1)
#else
static jmp_buf checkpoint;
#define SM_SETJMP() setjmp(checkpoint)
#define SM_LONGJMP() longjmp(checkpoint,1)
#endif
NOINLINE void mario_core_abort(void) {current->fault=true;SM_LONGJMP();}
static void joy(void *unused,struct SMB_buttons *b) {
  (void)unused;unsigned k=current->keys;
  *b=(struct SMB_buttons){.r=(k&SM_RIGHT)!=0,.l=(k&SM_LEFT)!=0,.d=(k&SM_DOWN)!=0,.u=(k&SM_UP)!=0,
    .a=(k&SM_JUMP)!=0,.b=(k&SM_RUN)!=0,.select=(k&SM_SELECT)!=0,.start=(k&SM_START)!=0};
}
void sm_init(sm_game *g) {
  memset(g,0,sizeof *g);
  g->state.chrrom=smb_patterns;
  g->state.callbacks.joy1=joy;g->state.callbacks.joy2=joy;
  g->state.start_on_world=1;g->state.start_on_level=1;
  SMB_STATE=&g->state;current=g;
}
bool sm_step(sm_game *g,unsigned keys) {
  if(g->fault)return false;
  current=g;SMB_STATE=&g->state;g->keys=(uint8_t)keys;
  if(SM_SETJMP())return false;
  if(!g->state.reset_occurred)smb1_Reset();
  memcpy(g->sprites,g->state.rammem+0x200,256);
  smb1_NMI();
  if(!g->state.reset_occurred) {
    smb1_set_world_and_level(g->state.start_on_world-1,g->state.start_on_level-1);
    g->state.reset_occurred=true;
  }
  ++g->ticks;return true;
}
void sm_level(sm_game *g,unsigned world,unsigned level) {
  sm_init(g);
  if(world>=1&&world<=8&&level>=1&&level<=4) {
    g->state.start_on_world=(uint8_t)world;g->state.start_on_level=(uint8_t)level;
  }
}
static uint32_t hash(const uint8_t *p,size_t n) {
  uint32_t h=2166136261u;for(size_t i=0;i<n;++i)h=(h^p[i])*16777619u;return h;
}
static void put32(uint8_t *p,uint32_t n) {for(unsigned i=0;i<4;++i)p[i]=(uint8_t)(n>>(8*i));}
static uint32_t get32(const uint8_t *p) {return p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
void sm_encode(const sm_game *g,uint8_t out[SM_SAVE_BYTES]) {
  memcpy(out,"SMN1",4);out[4]=g->state.reset_occurred;out[5]=g->state.ppu.screen_on;
  out[6]=(uint8_t)g->state.ppu.scroll_x;out[7]=(uint8_t)(g->state.ppu.scroll_x>>8);
  out[8]=(uint8_t)g->state.ppu.scroll_y;out[9]=(uint8_t)(g->state.ppu.scroll_y>>8);
  out[10]=g->state.start_on_world;out[11]=g->state.start_on_level;put32(out+12,g->ticks);
  memcpy(out+16,g->state.rammem,2048);memcpy(out+2064,g->state.ppuram,2080);
  memcpy(out+4144,g->sprites,256);
  put32(out+4400,hash(out,4400));
}
bool sm_decode(sm_game *g,const uint8_t *p,size_t n) {
  if(!p||n!=SM_SAVE_BYTES||memcmp(p,"SMN1",4)||get32(p+4400)!=hash(p,4400)||p[4]>1||p[5]>1||
     p[7]>1||p[9]>1||p[10]<1||p[10]>8||p[11]<1||p[11]>4)return false;
  /* Rebasing runs outside the simulation recovery checkpoint. Reject a
   * malformed pointer before changing the live game or calling the core. */
  const unsigned pointers[]={0xe7,0xe9,0xf5};
  for(unsigned i=0;i<3;++i) {
    unsigned at=16+pointers[i];uint16_t addr=p[at]|(uint16_t)p[at+1]<<8;
    if(addr&&!smb_rom_address(addr))return false;
  }
  sm_init(g);
  g->state.reset_occurred=p[4];g->state.ppu.screen_on=p[5];
  g->state.ppu.scroll_x=p[6]|(uint16_t)p[7]<<8;g->state.ppu.scroll_y=p[8]|(uint16_t)p[9]<<8;
  g->state.start_on_world=p[10];g->state.start_on_level=p[11];g->ticks=get32(p+12);
  memcpy(g->state.rammem,p+16,2048);memcpy(g->state.ppuram,p+2064,2080);
  memcpy(g->sprites,p+4144,256);
  smb1_sync_data();return true;
}
