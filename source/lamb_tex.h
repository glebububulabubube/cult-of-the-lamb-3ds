#include <3ds.h>
#include <citro2d.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <time.h>
#include <tex3ds.h>
#include "lamb_tex.h"
#define SCR_W 400
#define SCR_H 240
#define BOT_W 320
#define BOT_H 240
#define WORLD_W 900
#define WORLD_H 520
#define TEMPLE_W 1000
#define TEMPLE_H 600
#define MAX_BUILD 32
#define MAX_FOLLOW 24
#define MAX_ENEMY 6
#define DUNGEON_ROOMS 5
#define SV "sdmc:/cultlamb_save.bin"
static u32 C(float r,float g,float b){ return C2D_Color32((int)(r*255),(int)(g*255),(int)(b*255),255); }
enum GS{ST_MENU,ST_HUB,ST_PLAY,ST_DUN,ST_TALK,ST_TMP};
static GS state=ST_MENU;
static int menuSel=0;
static float tclock;
static bool wantExit=false;
static C3D_RenderTarget *top,*bottom;
static C2D_Font fnt;
static C2D_TextBuf tbuf;
static u32 bg,red,cream,black,white,grey,yellow;
static float px,py,php,pmaxhp=100.0f,camx,camy;
static float bx[MAX_BUILD],by[MAX_BUILD],btype[MAX_BUILD];
static int buildCount,hasKitchen,hasTemple;
static float fwX[MAX_FOLLOW],fwY[MAX_FOLLOW],fwT[MAX_FOLLOW],fwM[MAX_FOLLOW];
static int fwWork[MAX_FOLLOW],followCount;
static char fwName[MAX_FOLLOW][16];
static float altarX,altarY;
static int score;
static float dmgFlash;
static int talkTarget,talkSel,talkOpen,buildSel,buildFail;
static float tPx,tPy,tAltarX,tAltarY,tCamx,tCamy;
static int tmpSel,tmpUi;
static float ritualCd;
static float dpx,dpy;
static int dTh,dRoom;
static float exd[MAX_ENEMY],eyd[MAX_ENEMY],ehpz[MAX_ENEMY],espd[MAX_ENEMY];
static int dCnt,dEnem;
static float dAtkC,dAtkE,dRed;
static float hx,hy;
static C2D_Image g_lamb;
static bool g_lambReady=false;
static void loadLambTex(void){
    if(!C3D_TexInit(&g_lamb.tex,128,128,GPU_RGBA8))return;
    C3D_TexSetFilter(&g_lamb.tex,GPU_LINEAR,GPU_LINEAR);
    C3D_TexSetWrap(&g_lamb.tex,GPU_CLAMP_TO_EDGE,GPU_CLAMP_TO_EDGE);
    u8* dst=(u8*)g_lamb.tex.data;
    const u32 W=128,H=128,TILE=8,TW=W/TILE;
    for(u32 y=0;y<H;y++){
        for(u32 x=0;x<W;x++){
            u32 tx=x/TILE,ty=y/TILE,ox=x%TILE,oy=y%TILE;
            u32 m=0;
            for(int b=0;b<3;b++){ m|=((ox>>b)&1)<<(2*b); m|=((oy>>b)&1)<<(2*b+1); }
            u32 dstIdx=(ty*TW+tx)*(TILE*TILE)+m;
            u32 px=lamb_tex[y*W+x];
            dst[dstIdx*4+0]=(px>>24)&0xFF;
            dst[dstIdx*4+1]=(px>>16)&0xFF;
            dst[dstIdx*4+2]=(px>>8)&0xFF;
            dst[dstIdx*4+3]=(px)&0xFF;
        }
    }
    C3D_TexFlush(&g_lamb.tex);
    g_lamb.subtex=C2D_CreateSubtexture(&g_lamb.tex,0,0,128,128);
    g_lambReady=true;
}
static const char* nmAll[]={"ZEB","MORK","LUG","ROK","SYN","BAR","NAT","VEX","KRO","MAL","ZOR","GNA","THU","FEN","ORY","SAL","VUL","IX"};
static int nmC=18;
static float rnd(float a,float b){ return a+(float)rand()/(float)RAND_MAX*(b-a); }
static void loadD(int th);
static void tmpAct(int a);
static void text(float x,float y,float s,const char* t,float r,float g,float b){
    C2D_Text t1; C2D_TextFontParse(&t1,fnt,tbuf,t); C2D_TextOptimize(&t1);
    C2D_DrawText(&t1,C2D_AlignLeft,x,y,0.3f,s,s,C(r,g,b));
}
static void lamb(float x,float y,float sc,int sword){
    if(g_lambReady && g_lamb.subtex){
        C2D_ImageTint tint;
        C2D_PlainImageTint(&tint,0xFFFFFFFF,0.0f);
        C2D_DrawImageAt(g_lamb,x-64*sc,y-64*sc,0.5f,&tint,sc,sc);
        C2D_DrawCircleSolid(x,y+24*sc,0.3f,10*sc,C2D_Color32(0,0,0,80));
        if(sword){
            C2D_DrawRectSolid(x+18*sc,y-8*sc,0.55f,3*sc,13*sc,C(0.85f,0.88f,0.95f));
            C2D_DrawTriangle(x+19.5f*sc,y-22*sc,C(0.92f,0.94f,0.98f),x+17*sc,y-8*sc,C(0.92f,0.94f,0.98f),x+22*sc,y-8*sc,C(0.92f,0.94f,0.98f),0.56f);
            C2D_DrawRectSolid(x+16*sc,y-9*sc,0.57f,9*sc,3*sc,C(0.65f,0.5f,0.2f));
        }
        return;
    }
    C2D_DrawCircleSolid(x,y+22*sc,0.6f,9*sc,C2D_Color32(0,0,0,90));
    C2D_DrawRectSolid(x-8*sc,y+12*sc,0.3f,4*sc,9*sc,grey);
    C2D_DrawRectSolid(x+4*sc,y+12*sc,0.3f,4*sc,9*sc,grey);
    C2D_DrawRectSolid(x-13*sc,y-6*sc,0.4f,26*sc,20*sc,red);
    for(int i=0;i<5;i++){ float z=x-12*sc+i*6*sc; C2D_DrawRectSolid(z,y+13*sc,0.41f,5*sc,3*sc,(i%2)?white:C(0.9f,0.9f,0.9f)); }
    C2D_DrawRectSolid(x-16*sc,y-2*sc,0.5f,5*sc,8*sc,grey);
    C2D_DrawRectSolid(x+11*sc,y-2*sc,0.5f,5*sc,8*sc,grey);
    C2D_DrawCircleSolid(x,y+3*sc,0.51f,4*sc,yellow);
    C2D_DrawCircleSolid(x-1*sc,y+3*sc,0.6f,1*sc,black);
    C2D_DrawCircleSolid(x+1*sc,y+3*sc,0.6f,1*sc,black);
    C2D_DrawCircleSolid(x,y-15*sc,0.5f,9*sc,grey);
    C2D_DrawTriangle(x-11*sc,y-19*sc,C(0.6f,0.55f,0.55f),x-6*sc,y-14*sc,C(0.6f,0.55f,0.55f),x-3*sc,y-21*sc,C(0.6f,0.55f,0.55f),0.55f);
    C2D_DrawTriangle(x+11*sc,y-19*sc,C(0.6f,0.55f,0.55f),x+6*sc,y-14*sc,C(0.6f,0.55f,0.55f),x+3*sc,y-21*sc,C(0.6f,0.55f,0.55f),0.55f);
    C2D_DrawCircleSolid(x-3.5f*sc,y-16*sc,0.55f,2*sc,black);
    C2D_DrawCircleSolid(x+3.5f*sc,y-16*sc,0.55f,2*sc,black);
    C2D_DrawCircleSolid(x-3*sc,y-17*sc,0.6f,0.8f*sc,white);
    C2D_DrawCircleSolid(x+4*sc,y-17*sc,0.6f,0.8f*sc,white);
    C2D_DrawRectSolid(x-8*sc,y-30*sc,0.5f,16*sc,8*sc,yellow);
    C2D_DrawRectSolid(x-8*sc,y-30*sc,0.51f,16*sc,2*sc,white);
    C2D_DrawRectSolid(x-9*sc,y-33*sc,0.52f,3*sc,5*sc,black);
    C2D_DrawRectSolid(x+6*sc,y-33*sc,0.52f,3*sc,5*sc,black);
    C2D_DrawCircleSolid(x,y-26*sc,0.6f,2.5f*sc,C(1,0,0));
    if(sword){
        C2D_DrawRectSolid(x+8*sc,y-8*sc,0.55f,3*sc,12*sc,C(0.85f,0.88f,0.95f));
        C2D_DrawTriangle(x+9.5f*sc,y-22*sc,C(0.92f,0.94f,0.98f),x+7*sc,y-8*sc,C(0.92f,0.94f,0.98f),x+12*sc,y-8*sc,C(0.92f,0.94f,0.98f),0.56f);
        C2D_DrawRectSolid(x+6*sc,y-9*sc,0.57f,9*sc,3*sc,C(0.65f,0.5f,0.2f));
    }
}
