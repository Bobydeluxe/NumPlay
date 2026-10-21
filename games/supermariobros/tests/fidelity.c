/* Regression fingerprint captured from the full, unoptimized port.
 * Hash every serialized tick and every RGB565 test image, including restored
 * states, 32 stages, power-ups, ending, menus and the reference input replay.
 * No native pointers or compiler-dependent structure padding enter the hash. */
#include "../src/game.h"
#include <stdio.h>
#include <assert.h>
#include <inttypes.h>
static bool traced_step(sm_game *,uint8_t);
static void traced_render(const sm_game *,unsigned,unsigned,bool,sm_push);
#define sm_step traced_step
#define sm_render traced_render
#define main scenarios
#include "test.c"
#undef main
#undef sm_render
#undef sm_step
static uint64_t fingerprint=UINT64_C(14695981039346656037);
static unsigned ticks,images;
static void hash(const void *data,size_t size) {
  const uint8_t *p=data;
  while(size--) {fingerprint^=*p++;fingerprint*=UINT64_C(1099511628211);}
}
static bool traced_step(sm_game *g,uint8_t keys) {
  bool ok=sm_step(g,keys);uint8_t encoded[SM_SAVE_BYTES];
  sm_encode(g,encoded);hash(encoded,sizeof encoded);++ticks;return ok;
}
static void traced_render(const sm_game *g,unsigned v,unsigned s,bool f,sm_push p) {
  sm_render(g,v,s,f,p);
  /* Normalize pixel byte order for a portable reference. */
  for(unsigned i=0;i<320*240;++i) {
    uint8_t b[2]={(uint8_t)screen[i],(uint8_t)(screen[i]>>8)};hash(b,2);
  }
  ++images;
}
static unsigned reverse_buttons(unsigned x) {
  unsigned n=0;for(unsigned i=0;i<8;++i)n=(n<<1)|((x>>i)&1);return n;
}
int main(int argc,char **argv) {
  assert(argc==3);assert(scenarios()==0);
  FILE *f=fopen(argv[1],"r");assert(f);
  unsigned next=0,value=0,buttons=0;int has=fscanf(f,"%u:%u",&next,&value);
  sm_init(&game);
  for(unsigned i=0;i<8500;++i) {
    while(has==2&&next<=i) {buttons=reverse_buttons(value);has=fscanf(f,"%u:%u",&next,&value);}
    assert(traced_step(&game,buttons));
    if(i%30==0) {sm_invalidate();traced_render(&game,0,0,false,push);}
  }
  fclose(f);
  char result[100],expected[100];
  sprintf(result,"FIDELITY %016" PRIx64 " ticks=%u images=%u\n",fingerprint,ticks,images);
  f=fopen(argv[2],"r");assert(f);assert(fgets(expected,sizeof expected,f));fclose(f);
  fputs(result,stdout);assert(!strcmp(result,expected));
  return 0;
}
