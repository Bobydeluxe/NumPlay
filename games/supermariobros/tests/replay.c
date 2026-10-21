#include "../src/game.h"
#include <stdio.h>
#include <assert.h>
static sm_game game;
static unsigned reverse(unsigned x) {unsigned n=0;for(int i=0;i<8;++i)n=(n<<1)|((x>>i)&1);return n;}
int main(int argc,char **argv) {
  assert(argc==2);FILE *f=fopen(argv[1],"r");assert(f);
  unsigned frame=0,next=0,buttons=0,value=0,w=99,l=99,visited=0;
  int has=fscanf(f,"%u:%u",&next,&value);sm_init(&game);
  for(frame=0;frame<8500;++frame) {
    while(has==2&&next<=frame) {buttons=reverse(value);has=fscanf(f,"%u:%u",&next,&value);}
    if(!sm_step(&game,buttons)) {printf("FAULT %u\n",frame);return 1;}
    unsigned ww=game.state.rammem[0x75f],ll=game.state.rammem[0x75c];
    if(ww==0&&ll<4&&game.state.rammem[0x770]==1)visited|=1u<<ll;
    if(w!=ww||l!=ll) {w=ww;l=ll;printf("Frame %u: %u-%u mode %u lives %u\n",frame,w+1,l+1,game.state.rammem[0x770],game.state.rammem[0x75a]);}
  }
  assert(visited==15);puts("PASS: reference inputs clear 1-1, 1-2 and 1-3 and enter 1-4");
  fclose(f);return 0;
}
