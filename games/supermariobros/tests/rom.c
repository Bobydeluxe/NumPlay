/* Verify the packed flash address map and import an unoptimized SMN1 save. */
#include "../src/game.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static sm_game game;
static uint8_t original[32768],legacy[SM_SAVE_BYTES],saved[SM_SAVE_BYTES],bad[SM_SAVE_BYTES];
static void read_file(const char *name,uint8_t *p,size_t n) {
  FILE *f=fopen(name,"rb");assert(f);assert(fread(p,1,n,f)==n);assert(fgetc(f)==EOF);fclose(f);
}
int main(int argc,char **argv) {
  assert(argc==3);read_file(argv[1],original,sizeof original);read_file(argv[2],legacy,sizeof legacy);
  unsigned retained=0;
  for(unsigned addr=0;addr<65536;++addr) {
    const uint8_t *p=smb_rom_address((uint16_t)addr);
    if(!p)continue;
    assert(addr>=0x8000);assert(p>=smb_rom&&p<smb_rom+sizeof smb_rom);
    assert(*p==original[addr-0x8000]);++retained;
  }
  assert(retained==sizeof smb_rom);
  assert(sm_decode(&game,legacy,sizeof legacy));sm_encode(&game,saved);
  assert(!memcmp(saved,legacy,sizeof saved));
  /* A correctly checksummed snapshot with a bad native pointer must be
   * rejected before state mutation, including before the first engine tick. */
  const unsigned pointers[]={0xe7,0xe9,0xf5};
  for(unsigned i=0;i<3;++i) {
    memcpy(bad,legacy,sizeof bad);bad[16+pointers[i]]=0;bad[17+pointers[i]]=0x50;
    uint32_t h=2166136261u;for(unsigned j=0;j<4400;++j)h=(h^bad[j])*16777619u;
    for(unsigned j=0;j<4;++j)bad[4400+j]=(uint8_t)(h>>(8*j));
    assert(!sm_decode(&game,bad,sizeof bad));sm_encode(&game,saved);
    assert(!memcmp(saved,legacy,sizeof saved));
  }
  for(unsigned i=0;i<300;++i)assert(sm_step(&game,SM_RIGHT|SM_RUN|(i%53<22?SM_JUMP:0)));
  printf("PASS: %u original flash bytes, ROM gaps, legacy save import and safe pointer rejection\n",retained);
  return 0;
}
