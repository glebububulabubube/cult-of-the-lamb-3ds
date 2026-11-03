#include <3ds.h>
#include <citro2d.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <time.h>

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
#define ALTAR_R 16
#define MAX_ENEMY 6
#define DUNGEON_ROOMS 5
#define SAVE_PATH "sdmc:/cultlamb_save.bin"

static u32 C(float r,float g,float b){
    return C2D_Color32((int)(r*255),(int)(g*255),(int)(b*255),255);
}

static void startGame(void);
static void updateCamera(void);
static void addFollower(float x,float y,const char* nm);
static void addRandomFollower(float x,float y);
static void updateDungeon(float dt);
static void drawText(float x,float y,float scale,const char* s,float r,float g,float b);
static void drawLambfull(float x,float y,float sc,int sword);
static void drawFollowerWander(float ox,float oy);
static void drawBottomGrass(void);
static void drawBuilding(float wx,float wy,int type);
static void drawHouses(float ox,float oy);
static void drawGrassBg(void);
static void drawTopGate(float ox,float oy);
static void drawTalkTop(void);
static void doTalkAction(int action);
static void doTempleAction(int action);
static void updateTemple(float dt);
static void drawTempleTop(void);
static void drawTempleBottom(void);
static void enterTemple(void);
static void enterHub(void);
static void updateHub(float dt);
static void drawHubTop(void);
static void drawHubBottom(void);
static void loadRoom(void);
static void enterDungeon(int theme);
static void drawEnemy(float x,float y);
static void drawBoss(float x,float y);
static void drawDungeonTop(void);
static void drawDungeonBottom(void);
static void drawMenuTop(void);
static void drawBottomMenu(void);
static void saveGame(void);
static int loadGame(void);
static void newGame(void);
static void demolishAt(float wx,float wy);

static float px,py,php,pmaxhp=100.0f;
static float camx,camy;
static float bx[MAX_BUILD],by[MAX_BUILD],btype[MAX_BUILD]; static int buildCount;
static int hasKitchen; static int hasTemple;
static float fwX[MAX_FOLLOW],fwY[MAX_FOLLOW],fwT[MAX_FOLLOW],fwM[MAX_FOLLOW]; static int followCount;
static int fwWork[MAX_FOLLOW];
static char fwName[MAX_FOLLOW][24];
static float altarX,altarY;
static int score;
static float dmgFlash;
static int talkTarget,talkMenuSel,talkMenuOpen,buildSel,buildFailMsg;
static float tPx,tPy,tCamx,tCamy,tAltarX,tAltarY;
static int templeSel,templeUiDisp;
static float ritualCd;

enum GameState { ST_MENU, ST_DUNGEONHUB, ST_PLAY, ST_DUNGEON, ST_TALK, ST_TEMPLE };
static GameState state = ST_MENU;
static int menuSel = 0;
static float tclock;
static bool wantExit=false;

static float dpx,dpy;
static int dTheme,dRoom;
static float exd[MAX_ENEMY],eyd[MAX_ENEMY],ehpz[MAX_ENEMY],espd[MAX_ENEMY];
static int dEnemyCount,dEnemies;
static float dAtkCd,dAtkEnemy,dRed;
static float hx,hy;

static C3D_RenderTarget *top, *bottom;
static u32 bg, red, cream, black, white, grey, yellow;
static C2D_Font sysFont;
static C2D_TextBuf tBuf;

static const char* names[] = {
    "ZEB","MORK","LUG","ROK","SYN","BAR","NAT","VEX",
    "KRO","MAL","ZOR","GNA","THU","FEN","ORY","SAL","VUL","IX"
};
static int nameCount = 18;
static float rnd(float a,float b){ return a + (float)rand()/(float)RAND_MAX*(b-a); }

static const char* bossName(void){
    if(dTheme==0) return "LESHY";
    if(dTheme==1) return "KALLAMAR";
    if(dTheme==2) return "SHAMURA";
    return "HEKAT";
}

static void saveGame(void){
    FILE* f=fopen(SAVE_PATH,"wb");
    if(!f) return;
    int n=followCount;
    fwrite(&score,sizeof(int),1,f);
    fwrite(&hasKitchen,sizeof(int),1,f);
    fwrite(&hasTemple,sizeof(int),1,f);
    fwrite(&n,sizeof(int),1,f);
    fwrite(&buildCount,sizeof(int),1,f);
    for(int i=0;i<buildCount;i++){
        fwrite(&bx[i],sizeof(float),1,f);
        fwrite(&by[i],sizeof(float),1,f);
        fwrite(&btype[i],sizeof(float),1,f);
    }
    for(int i=0;i<n;i++){
        fwrite(fwName[i],24,1,f);
        fwrite(&fwM[i],sizeof(float),1,f);
        fwrite(&fwWork[i],sizeof(int),1,f);
        fwrite(&fwX[i],sizeof(float),1,f);
        fwrite(&fwY[i],sizeof(float),1,f);
    }
    fclose(f);
}
static int loadGame(void){
    FILE* f=fopen(SAVE_PATH,"rb");
    if(!f) return 0;
    int n=0;
    fread(&score,sizeof(int),1,f);
    fread(&hasKitchen,sizeof(int),1,f);
    fread(&hasTemple,sizeof(int),1,f);
    fread(&n,sizeof(int),1,f);
    fread(&buildCount,sizeof(int),1,f);
    if(n<0||n>MAX_FOLLOW) n=0;
    if(buildCount<0||buildCount>MAX_BUILD) buildCount=0;
    for(int i=0;i<buildCount;i++){
        fread(&bx[i],sizeof(float),1,f);
        fread(&by[i],sizeof(float),1,f);
        fread(&btype[i],sizeof(float),1,f);
    }
    followCount=n;
    for(int i=0;i<n;i++){
        fread(fwName[i],24,1,f);
        fread(&fwM[i],sizeof(float),1,f);
        fread(&fwWork[i],sizeof(int),1,f);
        fread(&fwX[i],sizeof(float),1,f);
        fread(&fwY[i],sizeof(float),1,f);
        fwT[i]=rnd(0,6);
    }
    fclose(f);
    return 1;
}
static void newGame(void){
    remove(SAVE_PATH);
    startGame();
    state=ST_PLAY;
}

static void demolishAt(float wx,float wy){
    int best=-1; float bestd=46*46;
    for(int i=0;i<buildCount;i++){
        int t=(int)btype[i];
        if(t==2||t==4) continue;
        float dx=bx[i]-wx, dy=by[i]-wy;
        float d=dx*dx+dy*dy;
        if(d<bestd){ bestd=d; best=i; }
    }
    if(best<0) return;
    for(int i=best;i<buildCount-1;i++){
        bx[i]=bx[i+1]; by[i]=by[i+1]; btype[i]=btype[i+1];
    }
    buildCount--;
}

static void drawText(float x,float y,float scale,const char* s,float r,float g,float b){
    C2D_Text txt;
    C2D_TextFontParse(&txt, sysFont, tBuf, s);
    C2D_TextOptimize(&txt);
    C2D_DrawText(&txt, C2D_AlignLeft, x, y, 0.3f, scale, scale, C(r,g,b));
}

static void drawLambfull(float x,float y,float sc,int sword){
    C2D_PlainImageTint(x, y-LAMB_TEX_H*sc*0.5f, LAMB_TEX_W*sc, LAMB_TEX_H*sc, lamb_tex, 0.0f, 0xFFFFFFFF);
    C2D_DrawCircleSolid(x, y+24*sc, 0.3f, 10*sc, C2D_Color32(0,0,0,80));
    if(sword){
        C2D_DrawRectSolid(x+18*sc, y-8*sc, 0.55f, 3*sc, 13*sc, C(0.85f,0.88f,0.95f));
        C2D_DrawTriangle(x+19.5f*sc, y-22*sc, C(0.92f,0.94f,0.98f), x+17*sc,y-8*sc, C(0.92f,0.94f,0.98f), x+22*sc,y-8*sc, C(0.92f,0.94f,0.98f),0.56f);
        C2D_DrawRectSolid(x+16*sc, y-9*sc, 0.57f, 9*sc, 3*sc, C(0.65f,0.5f,0.2f));
    }
}

static void addFollower(float x,float y,const char* nm){
    if(followCount>=MAX_FOLLOW) return;
    fwX[followCount]=x; fwY[followCount]=y;
    fwT[followCount]=rnd(0,6.28f);
    fwM[followCount]=50;
    fwWork[followCount]=0;
    snprintf(fwName[followCount],24,"%s",nm);
    followCount++;
}
static void addRandomFollower(float x,float y){
    addFollower(x,y, names[rand()%nameCount]);
}

static void startGame(void){
    score=20; buildCount=0; hasKitchen=0; hasTemple=0;
    altarX=WORLD_W/2; altarY=WORLD_H/2;
    px=altarX+40; py=altarY; php=pmaxhp; dmgFlash=0;
    ritualCd=0; buildFailMsg=0;
    addRandomFollower(altarX-40, altarY+20);
    addRandomFollower(altarX+40, altarY+10);
    addRandomFollower(altarX,     altarY+40);
    camx=px-SCR_W/2; camy=py-SCR_H/2;
    talkTarget=-1; talkMenuOpen=0; buildSel=0;
}

static void tryBuild(int type,float x,float y){
    if(buildCount>=MAX_BUILD) return;
    int cost = (type==0)?3 : (type==1)?2 : (type==2)?6 : (type==3)?1 : 8;
    if(score<cost){ buildFailMsg=90; return; }
    if(type==2 && hasKitchen) return;
    if(type==4 && hasTemple) return;
    bx[buildCount]=x; by[buildCount]=y; btype[buildCount]=type; buildCount++;
    if(type==2) hasKitchen=1;
    if(type==4) hasTemple=1;
    score-=cost;
    if(type==0) addRandomFollower(x+rnd(-20,20), y+20);
}

static void updateCamera(void){
    camx=px-SCR_W/2; camy=py-SCR_H/2;
    if(camx<0){camx=0;} if(camy<0){camy=0;}
    if(camx>WORLD_W-SCR_W){camx=WORLD_W-SCR_W;}
    if(camy>WORLD_H-SCR_H){camy=WORLD_H-SCR_H;}
}

static void drawAltar(float ox,float oy){
    float x=altarX-ox, y=altarY-oy;
    C2D_DrawCircleSolid(x,y,0.2f,ALTAR_R, C(0.45f,0.1f,0.1f));
    C2D_DrawCircleSolid(x,y,0.21f,ALTAR_R-2, C(0.2f,0.05f,0.05f));
    for(int i=0;i<5;i++){
        float a0=i*2.0f*3.14159f/5.0f-3.14159f/2.0f;
        float a1=(i+2)*2.0f*3.14159f/5.0f-3.14159f/2.0f;
        C2D_DrawLine(x+cos(a0)*ALTAR_R,y+sin(a0)*ALTAR_R, C(0.9f,0.3f,0.3f),
                     x+cos(a1)*ALTAR_R,y+sin(a1)*ALTAR_R, C(0.9f,0.3f,0.3f),1.0f,0.5f);
    }
    C2D_DrawCircleSolid(x,y,0.3f,5, C(1.0f,0.8f,0.2f));
}

static void drawBuilding(float wx,float wy,int type){
    float x=wx, y=wy;
    if(type==0){
        C2D_DrawTriangle(x,y-14, C(0.7f,0.15f,0.15f), x-14,y, C(0.7f,0.15f,0.15f), x+14,y, C(0.7f,0.15f,0.15f),0.4f);
        C2D_DrawRectSolid(x-14,y,0.39f,28,16, C(0.8f,0.7f,0.5f));
        C2D_DrawRectSolid(x-4,y+6,0.41f,8,10, C(0.4f,0.25f,0.1f));
    } else if(type==1){
        C2D_DrawRectSolid(x-8,y,0.39f,16,14, C(0.75f,0.75f,0.8f));
        C2D_DrawCircleSolid(x,y-2,0.4f,5, C(0.9f,0.9f,0.95f));
        C2D_DrawRectSolid(x-3,y+6,0.41f,6,8, C(0.4f,0.4f,0.45f));
    } else if(type==2){
        C2D_DrawTriangle(x,y-12, C(0.8f,0.4f,0.2f), x-14,y, C(0.8f,0.4f,0.2f), x+14,y, C(0.8f,0.4f,0.2f),0.4f);
        C2D_DrawRectSolid(x-14,y,0.39f,28,16, C(0.6f,0.5f,0.35f));
        C2D_DrawCircleSolid(x,y-4,0.42f,6, C(0.3f,0.3f,0.3f));
        C2D_DrawRectSolid(x-2,y-8,0.43f,4,2, C(0.5f,0.5f,0.5f));
    } else if(type==3){
        C2D_DrawRectSolid(x-1,y,0.4f,2,8, C(0.2f,0.5f,0.2f));
        C2D_DrawCircleSolid(x,y-3,0.41f,4, C(1,0.4f,0.6f));
        C2D_DrawCircleSolid(x-3,y-1,0.41f,2, C(1,0.9f,0.3f));
    } else {
        float s=2.6f;
        C2D_DrawTriangle(x,y-34*s, C(0.5f,0.1f,0.55f), x-26*s,y-16, C(0.5f,0.1f,0.55f), x+26*s,y-16, C(0.5f,0.1f,0.55f),0.4f);
        C2D_DrawRectSolid(x-26*s,y-16,0.39f,52*s,6, C(0.35f,0.1f,0.4f));
        C2D_DrawRectSolid(x-26*s,y-10,0.39f,52*s,34, C(0.3f,0.08f,0.35f));
        C2D_DrawRectSolid(x-8,y-2,0.41f,16,26, C(0.15f,0.04f,0.2f));
        C2D_DrawCircleSolid(x,y-22,0.44f,4, C(1,0.85f,0.2f));
        C2D_DrawRectSolid(x-6,y-42,0.43f,12,3, C(0.25f,0.06f,0.4f));
    }
}

static void drawHouses(float ox,float oy){
    for(int i=0;i<buildCount;i++){
        float x=bx[i]-ox, y=by[i]-oy;
        if(x<-60||x>SCR_W+60||y<-60||y>SCR_H+60) continue;
        drawBuilding(x,y,(int)btype[i]);
    }
}

static void drawTopGate(float ox,float oy){
    float gx=WORLD_W/2-ox, gy=20-oy;
    if(gx<-40||gx>SCR_W+40) return;
    C2D_DrawRectSolid(gx-20,gy-22,0.2f,40,24, C(0.2f,0.1f,0.1f));
    C2D_DrawRectSolid(gx-13,gy-16,0.3f,26,16, C(0.05f,0.02f,0.05f));
    C2D_DrawCircleSolid(gx,gy-18,0.4f,8, C(0.7f,0.15f,0.35f));
    drawText(gx-30,gy+4,0.5f,"ENTER DUNGEONS",0.9f,0.7f,0.6f);
}

static void drawGrassBg(void){
    C2D_DrawRectSolid(0,0,0,SCR_W,SCR_H, C(0.2f,0.5f,0.15f));
    for(int i=0;i<60;i++){
        float gx=(i*37 + ((int)tclock*10)) % (SCR_W+8);
        float gy=(i*53) % SCR_H;
        C2D_DrawRectSolid(gx,gy,0.2f,2,6, C(0.3f,0.6f,0.2f));
    }
    for(int i=0;i<40;i++){
        float gx=(i*61+20) % (SCR_W+8);
        float gy=(i*71+30) % SCR_H;
        C2D_DrawCircleSolid(gx,gy,0.2f,2, C(0.5f,0.7f,0.3f));
    }
}

static void drawFollowerWander(float ox,float oy){
    int n=followCount;
    for(int i=0;i<n;i++){
        float wx=fwX[i]-ox, wy=fwY[i]-oy;
        if(wx<-30||wx>SCR_W+30||wy<-30||wy>SCR_H+30) continue;
        if(fwWork[i]) C2D_DrawRectSolid(wx-5,wy-9,0.35f,10,4, C(1,0.85f,0.2f));
        C2D_DrawRectSolid(wx-4,wy+2,0.3f,8,9, C(0.75f,0.1f,0.1f));
        C2D_DrawCircleSolid(wx,wy,0.3f,6, C(0.35f,0.6f,0.25f));
        C2D_DrawRectSolid(wx-5,wy-2,0.32f,10,3, C(0.95f,0.95f,0.95f));
        C2D_DrawRectSolid(wx-5,wy+1,0.33f,10,2, C(0.95f,0.95f,0.95f));
        C2D_DrawTriangle(wx,wy+1, C(0.9f,0.1f,0.1f), wx-2,wy-1, C(0.9f,0.1f,0.1f), wx+2,wy-1, C(0.9f,0.1f,0.1f),0.35f);
        drawText(wx-14,wy-16,0.4f, fwName[i],1,1,0.6f);
    }
}

static void drawGame(void){
    updateCamera();
    float ox=camx, oy=camy;
    C2D_TargetClear(top, bg);
    C2D_SceneBegin(top);
    drawGrassBg();
    drawAltar(ox,oy);
    drawTopGate(ox,oy);
    drawHouses(ox,oy);
    drawFollowerWander(ox,oy);
    drawLambfull(px-ox,py-oy,1.0f,0);
    if(dmgFlash>0) C2D_DrawRectSolid(0,0,0.9f,SCR_W,SCR_H, C2D_Color32(255,0,0,50));
    char h[32]; snprintf(h,32,"Food:%d Cult:%d",score,followCount);
    drawText(8,30,0.5f,h,1,0.85f,0.3f);
    if(ritualCd>0) drawText(8,46,0.45f,"Ritual charging...",1,0.7f,0.4f);
    drawText(240,4,0.45f,"A: talk/enter",1,0.9f,0.4f);
}
