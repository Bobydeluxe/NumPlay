/* SPDX-License-Identifier: GPL-3.0-only */
#include "game.h"
#include "palette.h"
#include <string.h>
#define RGB(r,g,b) (((r)>>3)<<11|((g)>>2)<<5|((b)>>3))
#define WHITE RGB(255,254,255)
#define NAVY RGB(13,19,31)
#define GOLD RGB(247,186,63)
#define GREY RGB(180,192,207)
static uint16_t strip[320*8],palette[32];
static int top;
static bool invalid=true;
void sm_invalidate(void) {invalid=true;}
static void pixel(int x,int y,uint16_t c) {
  if(x>=0&&x<320&&y>=top&&y<top+8)strip[(y-top)*320+x]=c;
}
static void rect(int x,int y,int w,int h,uint16_t c) {
  int l=x<0?0:x,r=x+w>320?320:x+w,t=y<top?top:y,b=y+h>top+8?top+8:y+h;
  for(int yy=t;yy<b;++yy)for(int xx=l;xx<r;++xx)strip[(yy-top)*320+xx]=c;
}
static void tile(unsigned idx,int x,int y,unsigned pal,bool flipx,bool flipy,uint16_t override) {
  if(y>=top+8||y+8<=top||x>=288||x+8<=32)return;
  const uint8_t *p=smb_patterns+(idx&511)*16;
  int start=top>y?top-y:0,end=top+8-y;if(end>8)end=8;
  for(int yy=start;yy<end;++yy) {
    int row=flipy?7-yy:yy;unsigned lo=p[row],hi=p[row+8];
    for(int xx=0;xx<8;++xx) {
      unsigned bit=flipx?xx:7-xx,v=((lo>>bit)&1)|(((hi>>bit)&1)<<1);
      if(v&&x+xx>=32&&x+xx<288)pixel(x+xx,y+yy,override?override:palette[(pal&7)*4+v]);
    }
  }
}
static void text(int x,int y,const char *s,uint16_t c) {
  for(;*s;++s,x+=8) {
    unsigned ch=(unsigned char)*s;
    if(ch>='a'&&ch<='z')ch-=32;
    unsigned idx=ch>='0'&&ch<='9'?ch-'0':ch>='A'&&ch<='Z'?ch-'A'+10:36;
    if(ch=='-')idx=40;
    if(ch=='!')idx=43;
    if(ch=='.')idx=175;
    tile(256+idx,x,y,0,false,false,c);
  }
}
static void centered(int y,const char *s,uint16_t c) {text(160-(int)strlen(s)*4,y,s,c);}
static void sprites(const sm_game *g,bool behind) {
  for(int i=63;i>=0;--i) {
    const uint8_t *p=g->sprites+i*4;unsigned a=p[2];
    if(((a&32)!=0)!=behind)continue;
    tile(p[1],32+p[3],1+p[0],4+(a&3),(a&64)!=0,(a&128)!=0,0);
  }
}
static void background(const sm_game *g) {
  for(int yy=top;yy<top+8;yy+=8) {
    unsigned ytile=(unsigned)yy/8;
    unsigned scroll=yy<32?0:g->state.ppu.scroll_x;
    unsigned start=scroll&255,bank=(scroll>>8)&1;
    int sx=32-(int)(start&7);
    for(unsigned col=start/8;sx<288;++col,sx+=8) {
      unsigned nt=bank^((col/32)&1),x=col&31;
      const uint8_t *p=g->state.ppuram+nt*1024;
      unsigned a=p[960+(ytile/4)*8+x/4];a>>=((ytile/2)&1)*4+((x/2)&1)*2;
      tile(256+p[ytile*32+x],sx,yy,a&3,false,false,0);
    }
  }
}
static void choice(int y,const char *label,unsigned id,unsigned selected) {
  if(id==selected)rect(54,y-4,212,16,RGB(54,68,91)),text(60,y,">",GOLD);
  text(76,y,label,id==selected?WHITE:GREY);
}
static void overlay(unsigned view,unsigned selected,bool failed) {
  rect(47,50,226,149,NAVY);rect(47,50,226,3,GOLD);
  if(view==SM_VIEW_PAUSE) {
    centered(64,"PAUSED",WHITE);
    choice(90,"RESUME",0,selected);choice(112,"NEW GAME",1,selected);
    choice(134,"CONTROLS",2,selected);choice(156,"SAVE AND EXIT",3,selected);
    centered(182,failed?"SAVE FAILED":"BY BOBYDELUXE",failed?GOLD:GREY);
  } else if(view==SM_VIEW_GUIDE) {
    centered(64,"CONTROLS",WHITE);
    text(58,86,"ARROWS     MOVE / DUCK",GREY);
    text(58,104,"OK / 5     JUMP / SWIM",WHITE);
    text(58,122,"SHIFT      RUN / FIRE",WHITE);
    text(58,140,"EXE        START",GREY);
    text(58,158,"ALPHA      1 / 2 PLAYERS",GREY);
    centered(182,"OK / BACK  RETURN",GOLD);
  } else if(view==SM_VIEW_RESTART) {
    centered(64,"NEW GAME",WHITE);
    centered(94,"REPLACE THIS ADVENTURE",GREY);
    centered(111,"AND START FROM WORLD 1",GREY);
    choice(143,"KEEP PLAYING",0,selected);choice(171,"START NEW",1,selected);
  } else if(view==SM_VIEW_FAULT) {
    centered(69,"GAME INTERRUPTED",GOLD);centered(102,"PROGRESS IS RETAINED",GREY);
    centered(136,"OK  RETURN TO TITLE",WHITE);centered(177,"HOME  EXIT",GREY);
  } else {
    centered(65,"SAVE FAILED",GOLD);centered(94,"FREE STORAGE MAY BE FULL",GREY);
    centered(119,"OK  RETRY SAVE AND EXIT",WHITE);
    centered(143,"1   RETURN TO THE GAME",GREY);
    centered(171,"BACK  EXIT WITHOUT SAVE",GOLD);
  }
}
static unsigned old_view=99,old_selected;
static bool old_failed;
static uint32_t old_ticks=UINT32_MAX;
void sm_render(const sm_game *g,unsigned view,unsigned selected,bool failed,sm_push push) {
  if(!invalid&&old_view==view&&old_selected==selected&&old_failed==failed&&old_ticks==g->ticks)return;
  for(unsigned i=0;i<32;++i) {
    unsigned c=g->state.ppuram[0x800+i]&63;
    palette[i]=(uint16_t)RGB(PALETTE_BUILTIN[c][0],PALETTE_BUILTIN[c][1],PALETTE_BUILTIN[c][2]);
  }
  for(top=0;top<240;top+=8) {
    rect(0,top,320,8,NAVY);rect(32,top,256,8,palette[16]);
    if(g->state.ppu.screen_on) {sprites(g,true);background(g);sprites(g,false);}
    /* A small adaptation credit outside the original scene on the title. */
    if(g->state.rammem[0x770]==0&&g->state.rammem[0x772]>=3) {
      rect(104,196,112,11,palette[16]);centered(197,"BY BOBYDELUXE",WHITE);
    }
    if(view!=SM_VIEW_GAME)overlay(view,selected,failed);
    push(0,top,320,8,strip);
  }
  old_view=view;old_selected=selected;old_failed=failed;old_ticks=g->ticks;invalid=false;
}
