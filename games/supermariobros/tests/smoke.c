#include "../src/game.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static sm_game game,copy;
static uint16_t screen[320*240];
static uint8_t save[SM_SAVE_BYTES];
static void push(int x,int y,int w,int h,const uint16_t *p) {
  assert(x>=0&&y>=0&&x+w<=320&&y+h<=240&&w*h<=2560);
  for(int j=0;j<h;++j)memcpy(screen+(y+j)*320+x,p+j*w,w*2);
}
static void shot(const char *name) {
  sm_invalidate();sm_render(&game,0,0,false,push);FILE *f=fopen(name,"wb");assert(f);
  fprintf(f,"P6\n320 240\n255\n");
  for(unsigned i=0;i<320*240;++i) {
    unsigned c=screen[i];fputc((c>>11)*255/31,f);fputc(((c>>5)&63)*255/63,f);fputc((c&31)*255/31,f);
  }
  fclose(f);
}
int main(void) {
  sm_init(&game);
  for(int i=0;i<90;++i)assert(sm_step(&game,0));
  printf("Title mode %u task %u bytes %u\n",game.state.rammem[0x770],game.state.rammem[0x772],(unsigned)sizeof game);shot("title.ppm");
  assert(sm_step(&game,SM_START));
  for(int i=0;i<200;++i)assert(sm_step(&game,0));
  printf("Game mode %u task %u world %u level %u\n",game.state.rammem[0x770],game.state.rammem[0x772],game.state.rammem[0x75f],game.state.rammem[0x75c]);shot("gameplay.ppm");
  sm_encode(&game,save);assert(sm_decode(&copy,save,sizeof save));
  for(unsigned i=0;i<200;++i) {
    unsigned keys=SM_RIGHT|SM_RUN|(i%70<35?SM_JUMP:0);
    assert(sm_step(&game,keys)&&sm_step(&copy,keys));
    assert(!memcmp(game.state.rammem,copy.state.rammem,2048));
    assert(!memcmp(game.state.ppuram,copy.state.ppuram,sizeof game.state.ppuram));
  }
  shot("running.ppm");puts("PASS: original engine boot, start, gameplay and exact save continuation");
}
