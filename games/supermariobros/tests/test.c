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
int smoke(void) {
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
  shot("running.ppm");puts("PASS: original engine boot, start, gameplay and exact save continuation");return 0;
}

static void fixture(const char *name) {
  sm_encode(&game,save);FILE *f=fopen(name,"wb");assert(f);assert(fwrite(save,1,sizeof save,f)==sizeof save);fclose(f);
}
static void launch(unsigned world,unsigned level) {
  sm_level(&game,world,level);
  for(int i=0;i<90;++i)assert(sm_step(&game,0));
  assert(sm_step(&game,SM_START));
  for(int i=0;i<240;++i)assert(sm_step(&game,0));
  /* Underground/water levels begin with the original automatic pipe walk. */
  for(unsigned i=0;i<1200&&game.state.rammem[0x750]==0x29;++i)assert(sm_step(&game,0));
  for(unsigned i=0;i<180&&game.state.rammem[0x770]==1&&game.state.rammem[0x772]!=3;++i)assert(sm_step(&game,0));
  assert(game.state.rammem[0x75f]==world-1&&game.state.rammem[0x75c]==level-1);
}
void smb1_HandlePowerUpCollision(uint8_t objoff);
static void features(void) {
  launch(1,1);
  game.state.rammem[0x39]=0;smb1_HandlePowerUpCollision(5);
  assert(game.state.rammem[0x756]==1);
  for(unsigned i=0;i<100;++i)assert(sm_step(&game,0));
  assert(game.state.rammem[0x754]==0);
  game.state.rammem[0x39]=1;smb1_HandlePowerUpCollision(5);
  assert(game.state.rammem[0x756]==2);
  for(unsigned i=0;i<100;++i)assert(sm_step(&game,0));
  game.state.rammem[0x39]=2;smb1_HandlePowerUpCollision(5);
  assert(game.state.rammem[0x79f]==0x23);
  puts("PASS: mushroom growth, fire flower and star invincibility");
  /* Drive the real victory-message/end-world state machine. This is a unit
   * scenario, not a claim that a controller completed the 8-4 maze. */
  launch(8,4);game.state.rammem[0x770]=2;game.state.rammem[0x772]=3;
  game.state.rammem[0x719]=0;game.state.rammem[0x749]=0;
  memset(game.state.rammem+0xf,0,7);
  for(unsigned i=0;i<800;++i)assert(sm_step(&game,0));
  assert(game.state.rammem[0x772]==4&&game.state.rammem[0x75f]==7);
  fixture("ending-save.bin");
  for(unsigned i=0;i<240;++i)assert(sm_step(&game,SM_RUN));
  assert(game.state.rammem[0x7fc]);
  puts("PASS: final victory messages, end-world wait and second-quest unlock");
  launch(1,1);game.state.rammem[0x770]=1;game.state.rammem[0x772]=255;
  assert(!sm_step(&game,0)&&game.fault);
  puts("PASS: invalid engine dispatch recovers to the application safely");
}
int main(void) {
  smoke();unsigned terrain=0;
  for(unsigned w=1;w<=8;++w)for(unsigned l=1;l<=4;++l) {
    launch(w,l);terrain|=1u<<game.state.rammem[0x74e];
    char name[40];sprintf(name,"level-%u-%u.ppm",w,l);shot(name);
    sprintf(name,"level-%u-%u.bin",w,l);fixture(name);
    assert(sm_decode(&copy,save,sizeof save));
    for(unsigned frame=0;frame<300;++frame) {
      unsigned keys=SM_RIGHT|SM_RUN|(frame%53<22?SM_JUMP:0);
      assert(sm_step(&game,keys)&&sm_step(&copy,keys));
      assert(!memcmp(game.state.rammem,copy.state.rammem,2048));
      assert(!memcmp(game.state.ppuram,copy.state.ppuram,sizeof game.state.ppuram));
      assert(!memcmp(game.sprites,copy.sprites,256));
    }
  }
  printf("PASS: all 32 level starts; terrain mask %u; 9600 exact-restoration continuation ticks\n",terrain);
  assert(terrain==15);
  launch(1,1);fixture("start-save.bin");
  for(unsigned i=0;i<SM_SAVE_BYTES;++i) {
    save[i]^=1;assert(!sm_decode(&copy,save,sizeof save));save[i]^=1;
  }
  puts("PASS: every save-byte corruption rejected");
  for(unsigned v=0;v<=SM_VIEW_FAULT;++v) {
    sm_invalidate();sm_render(&game,v,0,false,push);
    unsigned before=game.ticks;sm_render(&game,v,1,false,push);assert(game.ticks==before);
  }
  puts("PASS: all interface screens render within strip bounds");
  features();
  return 0;
}
