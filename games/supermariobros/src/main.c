/* SPDX-License-Identifier: GPL-3.0-only */
#include <eadk.h>
#include "game.h"
#include "../../common/epsilon_app.h"
#include "../../common/epsilon_files.h"
#ifdef __ELF__
const char eadk_app_name[] __attribute__((section(".rodata.eadk_app_name")))="Super Mario Bros.";
const uint32_t eadk_api_level __attribute__((section(".rodata.eadk_api_level")))=0;
#endif
#define KEY(k) (UINT64_C(1)<<(k))
static sm_game game;
static uint8_t save_data[SM_SAVE_BYTES];
static bool have_save,save_failed;
static uint32_t last_hash;
uint32_t perf_frames,perf_max,perf_slow,step_max;
static void push(int x,int y,int w,int h,const uint16_t *p) {
  eadk_display_push_rect((eadk_rect_t){x,y,w,h},p);
}
static bool save(void) {
  if(game.fault)return false;
  sm_encode(&game,save_data);unsigned n=SM_SAVE_BYTES-4;
  uint32_t h=save_data[n]|(uint32_t)save_data[n+1]<<8|(uint32_t)save_data[n+2]<<16|(uint32_t)save_data[n+3]<<24;
  if(have_save&&!save_failed&&h==last_hash)return true;
  save_failed=!ef_write("supermario.sav",save_data,SM_SAVE_BYTES);
  if(!save_failed)have_save=true,last_hash=h;
  return !save_failed;
}
static unsigned buttons(uint64_t k) {
  unsigned b=0;
  if(k&(KEY(eadk_key_right)|KEY(eadk_key_six)))b|=SM_RIGHT;
  if(k&(KEY(eadk_key_left)|KEY(eadk_key_four)))b|=SM_LEFT;
  if(k&(KEY(eadk_key_up)|KEY(eadk_key_eight)))b|=SM_UP;
  if(k&(KEY(eadk_key_down)|KEY(eadk_key_two)))b|=SM_DOWN;
  if(k&(KEY(eadk_key_ok)|KEY(eadk_key_five)))b|=SM_JUMP;
  if(k&(KEY(eadk_key_shift)|KEY(eadk_key_zero)))b|=SM_RUN;
  if(k&KEY(eadk_key_exe))b|=SM_START;
  if(k&KEY(eadk_key_alpha))b|=SM_SELECT;
  return b;
}
int main(void) {
  np_app_begin();
  uint32_t length=0;const uint8_t *p=ef_read("supermario.sav",&length);
  bool loaded=sm_decode(&game,p,length);
  if(!loaded)sm_init(&game);
  /* A saved adventure opens frozen, so loading cannot cause an accidental death. */
  unsigned view=loaded?SM_VIEW_PAUSE:SM_VIEW_GAME,selected=0,return_view=SM_VIEW_PAUSE;
  unsigned error_return=view,error_selection=0;
  uint64_t previous=0,stale=eadk_keyboard_scan();
  uint32_t now=(uint32_t)eadk_timing_millis(),last=now,draw_at=now,save_at=now,accum=0;
  sm_invalidate();
  for(;;) {
    now=(uint32_t)eadk_timing_millis();uint64_t keys=eadk_keyboard_scan();stale&=keys;
    uint64_t active=keys&~stale,hit=active&~previous;previous=active;
    bool ok=(hit&(KEY(eadk_key_ok)|KEY(eadk_key_exe)))!=0,back=(hit&KEY(eadk_key_back))!=0;
    bool next=(hit&(KEY(eadk_key_down)|KEY(eadk_key_right)))!=0;
    bool prev=(hit&(KEY(eadk_key_up)|KEY(eadk_key_left)))!=0;
    if(view!=SM_VIEW_SAVE_ERROR&&(keys&(KEY(eadk_key_home)|KEY(eadk_key_on_off)))) {
      if(game.fault||save())break;
      error_return=view;error_selection=selected;view=SM_VIEW_SAVE_ERROR;stale=keys;
      ok=back=next=prev=false;
    }
    if(view==SM_VIEW_GAME) {
      if(back) {view=SM_VIEW_PAUSE;selected=0;save();}
      else if(hit&KEY(eadk_key_toolbox)) {view=SM_VIEW_GUIDE;return_view=SM_VIEW_GAME;}
      else {
        unsigned dt=now-last;if(dt>66)dt=66;accum+=dt*60;
        while(accum>=1000) {
          uint32_t start=(uint32_t)eadk_timing_millis();bool good=sm_step(&game,buttons(active));
          uint32_t spent=(uint32_t)eadk_timing_millis()-start;if(spent>step_max)step_max=spent;
          accum-=1000;
          if(!good){view=SM_VIEW_FAULT;break;}
        }
        if(now-save_at>=10000)save(),save_at=now;
      }
    } else {
      accum=0;
      if(view==SM_VIEW_PAUSE) {
        if(next)selected=(selected+1)%4;
        if(prev)selected=(selected+3)%4;
        if(back||(ok&&selected==0))view=SM_VIEW_GAME,stale=keys;
        else if(ok&&selected==1)view=SM_VIEW_RESTART,selected=0;
        else if(ok&&selected==2)view=SM_VIEW_GUIDE,return_view=SM_VIEW_PAUSE;
        else if(ok&&selected==3) {
          if(save())break;
          error_return=SM_VIEW_PAUSE;error_selection=3;view=SM_VIEW_SAVE_ERROR;stale=keys;
        }
      } else if(view==SM_VIEW_GUIDE) {
        if(ok||back)view=return_view,stale=keys;
      } else if(view==SM_VIEW_RESTART) {
        if(next||prev)selected^=1;
        if(back||(ok&&!selected))view=SM_VIEW_PAUSE,selected=1;
        else if(ok){sm_init(&game);view=SM_VIEW_GAME;stale=keys;save_at=now;}
      } else if(view==SM_VIEW_FAULT) {
        if(ok){sm_init(&game);view=SM_VIEW_GAME;stale=keys;}
      } else if(view==SM_VIEW_SAVE_ERROR) {
        if(back)break;
        if(ok&&save())break;
        if(hit&KEY(eadk_key_one))view=error_return,selected=error_selection,stale=keys;
      }
    }
    last=now;
    if(now-draw_at>=33||hit||!perf_frames) {
      uint32_t start=(uint32_t)eadk_timing_millis();
      sm_render(&game,view,selected,save_failed,push);
      uint32_t spent=(uint32_t)eadk_timing_millis()-start;
      if(spent>perf_max)perf_max=spent;
      if(spent>33)++perf_slow;
      ++perf_frames;draw_at=now;
    }
    eadk_timing_msleep(2);
  }
  return np_app_end();
}
