#include <3ds.h>
#include <citro2d.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <time.h>

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

static u32 C(float r,float g,float b){
    return C2D_Color32((int)(r*255),(int)(g*255),(int)(b*255),255);
}

static void startGame(void);
static void updateCamera(void);
static void addFollower(float x,float y,const char* nm);
static void addRandomFollower(float x,float y);
static void updateDungeon(float dt);
static void drawText(float x,float y,float scale,const char* s,float r,float g,float b);
static void drawLambfull(float x,float y,float sc);
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
static void drawBishop(float x,float y);
static void drawDungeonTop(void);
static void drawDungeonBottom(void);
static void drawMenuTop(void);
static void drawBottomMenu(void);

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

// dungeon state
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

static void drawText(float x,float y,float scale,const char* s,float r,float g,float b){
    C2D_Text txt;
    C2D_TextFontParse(&txt, sysFont, tBuf, s);
    C2D_TextOptimize(&txt);
    C2D_DrawText(&txt, C2D_AlignLeft, x, y, 0.3f, scale, scale, C(r,g,b));
}

static void drawLambfull(float x,float y,float sc){
    C2D_DrawCircleSolid(x, y+22*sc, 0.6f, 9*sc, C2D_Color32(0,0,0,90));
    C2D_DrawRectSolid(x-8*sc, y+12*sc, 0.3f, 4*sc, 9*sc, grey);
    C2D_DrawRectSolid(x+4*sc, y+12*sc, 0.3f, 4*sc, 9*sc, grey);
    C2D_DrawRectSolid(x-13*sc, y-6*sc, 0.4f, 26*sc, 20*sc, red);
    for(int i=0;i<5;i++){
        float zx=x-12*sc+i*6*sc;
        C2D_DrawRectSolid(zx, y+13*sc, 0.41f, 5*sc, 3*sc, (i%2)? white : C(0.9f,0.9f,0.9f));
    }
    C2D_DrawRectSolid(x-16*sc, y-2*sc, 0.5f, 5*sc, 8*sc, grey);
    C2D_DrawRectSolid(x+11*sc, y-2*sc, 0.5f, 5*sc, 8*sc, grey);
    C2D_DrawCircleSolid(x, y+3*sc, 0.51f, 4*sc, yellow);
    C2D_DrawCircleSolid(x-1*sc, y+3*sc, 0.6f, 1*sc, black);
    C2D_DrawCircleSolid(x+1*sc, y+3*sc, 0.6f, 1*sc, black);
    C2D_DrawCircleSolid(x, y-15*sc, 0.5f, 9*sc, grey);
    C2D_DrawTriangle(x-11*sc, y-19*sc, C(0.6f,0.55f,0.55f), x-6*sc,y-14*sc, C(0.6f,0.55f,0.55f), x-3*sc,y-21*sc, C(0.6f,0.55f,0.55f), 0.55f);
    C2D_DrawTriangle(x+11*sc, y-19*sc, C(0.6f,0.55f,0.55f), x+6*sc,y-14*sc, C(0.6f,0.55f,0.55f), x+3*sc,y-21*sc, C(0.6f,0.55f,0.55f), 0.55f);
    C2D_DrawCircleSolid(x-3.5f*sc, y-16*sc, 0.55f, 2*sc, black);
    C2D_DrawCircleSolid(x+3.5f*sc, y-16*sc, 0.55f, 2*sc, black);
    C2D_DrawCircleSolid(x-3*sc, y-17*sc, 0.6f, 0.8f*sc, white);
    C2D_DrawCircleSolid(x+4*sc, y-17*sc, 0.6f, 0.8f*sc, white);
    C2D_DrawRectSolid(x-8*sc, y-30*sc, 0.5f, 16*sc, 8*sc, yellow);
    C2D_DrawRectSolid(x-8*sc, y-30*sc, 0.51f, 16*sc, 2*sc, white);
    C2D_DrawRectSolid(x-9*sc, y-33*sc, 0.52f, 3*sc, 5*sc, black);
    C2D_DrawRectSolid(x+6*sc, y-33*sc, 0.52f, 3*sc, 5*sc, black);
    C2D_DrawCircleSolid(x, y-26*sc, 0.6f, 2.5f*sc, C(1,0,0));
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
    drawLambfull(px-ox,py-oy,1.0f);
    if(dmgFlash>0) C2D_DrawRectSolid(0,0,0.9f,SCR_W,SCR_H, C2D_Color32(255,0,0,50));
    char h[32]; snprintf(h,32,"Food:%d Cult:%d",score,followCount);
    drawText(8,30,0.5f,h,1,0.85f,0.3f);
    if(ritualCd>0) drawText(8,46,0.45f,"Ritual charging...",1,0.7f,0.4f);
    drawText(240,4,0.45f,"A: talk/enter",1,0.9f,0.4f);
}

static void drawBottomGrass(void){
    C2D_TargetClear(bottom, C(0.2f,0.5f,0.15f));
    C2D_SceneBegin(bottom);
    C2D_DrawRectSolid(0,0,0,BOT_W,BOT_H, C(0.2f,0.5f,0.15f));
    for(int i=0;i<50;i++){
        float gx=(i*37+10)% (BOT_W+8);
        float gy=(i*53)% BOT_H;
        C2D_DrawRectSolid(gx,gy,0.2f,2,6, C(0.3f,0.6f,0.2f));
    }
}

static void drawBottomGame(void){
    drawBottomGrass();
    if(talkMenuOpen){
        const char* acts[4]={"WORK","SCARE","CHEER","READ MIND"};
        for(int i=0;i<4;i++){
            int y=20+i*50;
            u32 col=(i==talkMenuSel)? C(0.55f,0.1f,0.15f): C(0.2f,0.4f,0.12f);
            C2D_DrawRectSolid(20,y-4,0.3f,280,34, col);
            drawText(40,y,0.5f,acts[i],0.9f,0.9f,0.9f);
        }
    } else {
        const char* types[5]={"HOUSE","TOILET","KITCHEN","DECOR","TEMPLE"};
        const int costs[5]={3,2,6,1,8};
        for(int i=0;i<5;i++){
            int y=26+i*40;
            u32 col=(i==buildSel)? C(0.55f,0.1f,0.15f): C(0.15f,0.4f,0.1f);
            C2D_DrawRectSolid(20,y-3,0.3f,132,30, col);
            drawText(26,y,0.42f,types[i],0.95f,0.95f,0.95f);
            char cr[12]; snprintf(cr,12,"%d",costs[i]);
            drawText(128,y,0.42f,cr,1,0.9f,0.2f);
        }
        C2D_DrawRectSolid(180,80,0.3f,125,90, C(0.1f,0.3f,0.08f));
        drawText(186,120,0.5f,"TAP = build",1,0.9f,0.4f);
        if(buildFailMsg>0) drawText(186,96,0.5f,"NO FOOD!",1,0.4f,0.2f);
    }
}

static void drawTalkTop(void){
    C2D_TargetClear(top, bg);
    C2D_SceneBegin(top);
    drawGrassBg();
    C2D_DrawRectSolid(0,0,0.5f,SCR_W,SCR_H, C2D_Color32(0,0,0,160));
    char t[48];
    if(talkTarget>=0 && talkTarget<followCount){
        snprintf(t,48,"%s - Moral %d", fwName[talkTarget], (int)fwM[talkTarget]);
    } else snprintf(t,48,"No one");
    drawText(40,30,0.6f,t,1,0.9f,0.4f);
}

// ==== DUNGEON HUB (4 themed doors) ====
static void enterHub(void){
    hx=200; hy=200; php=pmaxhp;
}
static void updateHub(float dt){
    u32 k=hidKeysHeld();
    float dx=0,dy=0;
    if(k&KEY_CPAD_UP) dy-=1;
    if(k&KEY_CPAD_DOWN) dy+=1;
    if(k&KEY_CPAD_LEFT) dx-=1;
    if(k&KEY_CPAD_RIGHT) dx+=1;
    float len=sqrtf(dx*dx+dy*dy);
    if(len>0){ dx/=len; dy/=len; }
    hx+=dx*2.6f*60.f*dt; hy+=dy*2.6f*60.f*dt;
    if(hx<15){hx=15;} if(hx>SCR_W-15){hx=SCR_W-15;}
    if(hy<15){hy=15;} if(hy>SCR_H-15){hy=SCR_H-15;}
    u32 down=hidKeysDown();
    if(down&(KEY_B)){ state=ST_PLAY; }
    if(down&KEY_A){
        if(hypotf(hx-110,hy-70)<32){ enterDungeon(0); }
        else if(hypotf(hx-290,hy-70)<32){ enterDungeon(1); }
        else if(hypotf(hx-110,hy-190)<32){ enterDungeon(2); }
        else if(hypotf(hx-290,hy-190)<32){ enterDungeon(3); }
        else if(hypotf(hx-200,hy-225)<36){ state=ST_PLAY; }
    }
}
static void drawHubTop(void){
    C2D_TargetClear(top, C(0.15f,0.12f,0.2f));
    C2D_SceneBegin(top);
    C2D_DrawRectSolid(0,0,0,SCR_W,SCR_H, C(0.15f,0.12f,0.2f));
    for(int i=0;i<120;i++){
        float fx=(i*61)% SCR_W, fy=(i*83)% SCR_H;
        C2D_DrawRectSolid(fx,fy,0.1f,2,2, C(0.22f,0.18f,0.28f));
    }
    // 4 doors
    struct { float x,y; const char* t; u32 col; } dr[4] = {
        {110,70,"FOREST", C(0.2f,0.5f,0.2f)},
        {290,70,"WATER",  C(0.2f,0.4f,0.75f)},
        {110,190,"SPIDERS",C(0.35f,0.15f,0.35f)},
        {290,190,"TOADS", C(0.3f,0.55f,0.3f)}
    };
    for(int i=0;i<4;i++){
        C2D_DrawRectSolid(dr[i].x-22,dr[i].y-30,0.2f,44,60, C(0.08f,0.06f,0.1f));
        C2D_DrawRectSolid(dr[i].x-16,dr[i].y-24,0.21f,32,48, dr[i].col);
        C2D_DrawRectSolid(dr[i].x-10,dr[i].y-18,0.22f,20,36, C(0.03f,0.02f,0.04f));
        drawText(dr[i].x-26,dr[i].y+32,0.4f,dr[i].t,0.95f,0.95f,0.95f);
    }
    // exit back to cult
    C2D_DrawRectSolid(176,218,0.2f,48,20, C(0.35f,0.25f,0.2f));
    drawText(185,226,0.4f,"BACK",1,0.85f,0.7f);
    drawLambfull(hx,hy,1.0f);
    drawText(50,6,0.5f,"DUNGEON HALL",1,0.9f,0.4f);
}
static void drawHubBottom(void){
    C2D_TargetClear(bottom, C(0.15f,0.12f,0.2f));
    C2D_SceneBegin(bottom);
    C2D_DrawRectSolid(0,0,0,BOT_W,BOT_H, C(0.15f,0.12f,0.2f));
    drawText(30,30,0.6f,"Pick a dungeon door",1,0.9f,0.4f);
    drawText(30,70,0.6f,"A: enter",1,0.9f,0.4f);
    drawText(30,110,0.6f,"B: back to cult",1,0.9f,0.4f);
}

// ==== DUNGEON ROOMS (Binding of Isaac style) ====
static void loadRoom(void){
    for(int i=0;i<MAX_ENEMY;i++){ exd[i]=eyd[i]=ehpz[i]=0; }
    dEnemyCount=0; dEnemies=0;
    dpx=40; dpy=SCR_H/2;
    if(dRoom==DUNGEON_ROOMS-1){
        exd[0]=SCR_W/2+60; eyd[0]=SCR_H/2; ehpz[0]=220; espd[0]=0.8f;
        dEnemyCount=1; dEnemies=1;
    } else {
        int n=3+(dRoom%3);
        for(int i=0;i<n && dEnemyCount<MAX_ENEMY;i++){
            float xx=120+(float)((i*97)%180);
            float yy=70+(float)((i*53)%120);
            exd[dEnemyCount]=xx; eyd[dEnemyCount]=yy;
            ehpz[dEnemyCount]=35+i*6; espd[dEnemyCount]=1.1f+dTheme*0.08f;
            dEnemyCount++; dEnemies++;
        }
    }
    dAtkCd=0; dAtkEnemy=0; dRed=0;
}
static void enterDungeon(int theme){
    dTheme=theme; dRoom=0;
    state=ST_DUNGEON;
    php=pmaxhp;
    loadRoom();
}
static void updateDungeon(float dt){
    u32 k=hidKeysHeld();
    float dx=0,dy=0;
    if(k&KEY_CPAD_UP) dy-=1;
    if(k&KEY_CPAD_DOWN) dy+=1;
    if(k&KEY_CPAD_LEFT) dx-=1;
    if(k&KEY_CPAD_RIGHT) dx+=1;
    float len=sqrtf(dx*dx+dy*dy);
    if(len>0){ dx/=len; dy/=len; }
    dpx+=dx*2.6f*60.f*dt; dpy+=dy*2.6f*60.f*dt;
    if(dpx<10){dpx=10;} if(dpx>SCR_W-10){dpx=SCR_W-10;}
    if(dpy<10){dpy=10;} if(dpy>SCR_H-10){dpy=SCR_H-10;}
    for(int i=0;i<dEnemyCount;i++){
        if(ehpz[i]<=0) continue;
        float a=atan2f(dpy-eyd[i],dpx-exd[i]);
        float sp=espd[i]*60.f*dt;
        exd[i]+=cos(a)*sp; eyd[i]+=sin(a)*sp;
        if(exd[i]<14){exd[i]=14;} if(exd[i]>SCR_W-14){exd[i]=SCR_W-14;}
        if(eyd[i]<14){eyd[i]=14;} if(eyd[i]>SCR_H-14){eyd[i]=SCR_H-14;}
    }
    u32 down=hidKeysDown();
    if(dAtkCd>0) dAtkCd-=dt;
    if((down&KEY_A)&&dAtkCd<=0){
        dAtkCd=0.25f;
        for(int i=0;i<dEnemyCount;i++){
            if(ehpz[i]<=0) continue;
            if(hypotf(exd[i]-dpx,eyd[i]-dpy)<34){ ehpz[i]-=25; if(ehpz[i]<=0)dEnemies--; break; }
        }
    }
    dAtkEnemy-=dt;
    if(dAtkEnemy<=0){
        dAtkEnemy=1.0f;
        for(int i=0;i<dEnemyCount;i++){
            if(ehpz[i]<=0) continue;
            if(hypotf(exd[i]-dpx,eyd[i]-dpy)<18){ php-=8; dRed=2.0f; if(php<0)php=0; }
        }
    }
    if(dRed>0) dRed-=dt;
    // door logic
    if(dpx<8 && dEnemies<=0){ enterHub(); return; }  // back door
    if(dpx>SCR_W-8 && dEnemies<=0){
        if(dRoom<DUNGEON_ROOMS-1){ dRoom++; loadRoom(); }
        else {
            score+=12;
            addRandomFollower(altarX+rnd(-30,30), altarY+rnd(0,40));
            enterHub();
        }
        return;
    }
    if(dpx<8 && dEnemies>0){ dpx=10; }
    if(dpx>SCR_W-8 && dEnemies>0){ dpx=SCR_W-10; }
}
static void drawEnemy(float x,float y){
    u32 zc;
    if(dTheme==0) zc=C(0.2f,0.55f,0.2f);
    else if(dTheme==1) zc=C(0.2f,0.4f,0.8f);
    else if(dTheme==2) zc=C(0.35f,0.18f,0.4f);
    else zc=C(0.3f,0.55f,0.32f);
    C2D_DrawRectSolid(x-7,y+2,0.3f,14,15, zc);
    C2D_DrawCircleSolid(x,y,0.3f,8, zc);
    C2D_DrawCircleSolid(x-3,y-2,0.5f,2, red);
    C2D_DrawCircleSolid(x+3,y-2,0.5f,2, red);
    if(dTheme==2){
        for(int i=0;i<4;i++) C2D_DrawRectSolid(x-9+i*3,y+7,0.4f,2,6, C(0.2f,0.1f,0.22f));
    } else if(dTheme==3){
        C2D_DrawCircleSolid(x-2,y+4,0.5f,2, C(0.65f,0.8f,0.65f));
        C2D_DrawCircleSolid(x+3,y+2,0.5f,1.5f, C(0.65f,0.8f,0.65f));
    }
}
static void drawBishop(float x,float y){
    float s=1.5f;
    C2D_DrawRectSolid(x-16,y-4,0.3f,32,34, C(0.35f,0.1f,0.42f));
    C2D_DrawCircleSolid(x,y,0.3f,10*s, C(0.55f,0.28f,0.6f));
    C2D_DrawTriangle(x-7,y-12, C(0.9f,0.8f,0.3f), x+7,y-12, C(0.9f,0.8f,0.3f), x,y-26, C(0.9f,0.8f,0.3f),0.4f);
    C2D_DrawCircleSolid(x-3,y-3,0.5f,2, white);
    C2D_DrawCircleSolid(x+3,y-3,0.5f,2, white);
    C2D_DrawRectSolid(x+20,y-18,0.4f,2,34, C(0.4f,0.3f,0.2f));
    C2D_DrawCircleSolid(x+21,y-20,0.4f,4, C(0.9f,0.8f,0.2f));
    drawText(x-24,y+22,0.5f,"BISHOP",1,0.85f,0.3f);
}
static void drawDungeonTop(void){
    u32 rb;
    if(dTheme==0) rb=C(0.1f,0.2f,0.08f);
    else if(dTheme==1) rb=C(0.08f,0.12f,0.25f);
    else if(dTheme==2) rb=C(0.12f,0.05f,0.12f);
    else rb=C(0.12f,0.22f,0.1f);
    C2D_TargetClear(top, rb);
    C2D_SceneBegin(top);
    C2D_DrawRectSolid(0,0,0,SCR_W,SCR_H, rb);
    for(int i=0;i<140;i++){
        float fx=(i*53)% SCR_W, fy=(i*79)% SCR_H;
        C2D_DrawRectSolid(fx,fy,0.1f,2,2, C(0.3f,0.3f,0.3f));
    }
    // back door (left, always open)
    C2D_DrawRectSolid(0,100,0.2f,14,70, C(0.5f,0.35f,0.2f));
    drawText(2,120,0.4f,"<",1,0.9f,0.9f);
    // next door (right) - open when cleared
    if(dEnemies<=0){
        C2D_DrawRectSolid(SCR_W-14,100,0.2f,14,70, C(0.2f,0.7f,0.3f));
        drawText(SCR_W-12,120,0.4f,">",1,1,1);
    } else {
        C2D_DrawRectSolid(SCR_W-14,100,0.2f,14,70, C(0.4f,0.15f,0.1f));
    }
    // room number & theme label
    char rlabel[48];
    const char* tn[4]={"FOREST","WATER","SPIDERS","TOADS"};
    snprintf(rlabel,48,"%s - Room %d/%d", tn[dTheme], dRoom+1, DUNGEON_ROOMS);
    drawText(6,6,0.5f,rlabel,1,0.9f,0.4f);
    // enemies
    bool bossRoom=(dRoom==DUNGEON_ROOMS-1);
    for(int i=0;i<dEnemyCount;i++){
        if(ehpz[i]<=0) continue;
        if(bossRoom) drawBishop(exd[i],eyd[i]);
        else drawEnemy(exd[i],eyd[i]);
    }
    drawLambfull(dpx,dpy,0.8f);
    if(dRed>0) C2D_DrawRectSolid(0,0,0.9f,SCR_W,SCR_H, C2D_Color32(255,0,0,60));
    // boss HP bar
    if(bossRoom && dEnemies>0){
        float mx=SCR_W/2-80, my=22;
        C2D_DrawRectSolid(mx,my,0.5f,160,10, C(0.15f,0.05f,0.05f));
        float frac=ehpz[0]/220.0f; if(frac<0)frac=0;
        C2D_DrawRectSolid(mx+1,my+1,0.51f,158*frac,8, C(0.9f,0.2f,0.15f));
    }
    if(dEnemies<=0 && bossRoom) drawText(SCR_W/2-90,SCR_H/2+40,0.6f,"BISHOP DEFEATED!",1,0.9f,0.3f);
}
static void drawDungeonBottom(void){
    C2D_TargetClear(bottom, C(0.08f,0.04f,0.04f));
    C2D_SceneBegin(bottom);
    C2D_DrawRectSolid(0,0,0,BOT_W,BOT_H, C(0.1f,0.05f,0.05f));
    drawText(30,20,0.6f,"A = FIGHT",1,0.6f,0.5f);
    drawText(30,60,0.6f,"B = nobody once clear",1,0.6f,0.5f);
    char h[40]; snprintf(h,40,"HP %d  Enemies %d",(int)php,dEnemies);
    drawText(30,100,0.6f,h,1,0.8f,0.4f);
}

// ==== TEMPLE ====
static void enterTemple(void){
    tPx=TEMPLE_W/2; tPy=TEMPLE_H-80;
    tAltarX=TEMPLE_W/2; tAltarY=TEMPLE_H/2;
    templeSel=0; templeUiDisp=0;
}
static void updateTemple(float dt){
    u32 k=hidKeysHeld();
    float dx=0,dy=0;
    if(k&KEY_CPAD_UP) dy-=1;
    if(k&KEY_CPAD_DOWN) dy+=1;
    if(k&KEY_CPAD_LEFT) dx-=1;
    if(k&KEY_CPAD_RIGHT) dx+=1;
    float len=sqrtf(dx*dx+dy*dy);
    if(len>0){ dx/=len; dy/=len; }
    tPx+=dx*2.6f*60.f*dt; tPy+=dy*2.6f*60.f*dt;
    if(tPx<40){tPx=40;} if(tPx>TEMPLE_W-40){tPx=TEMPLE_W-40;}
    if(tPy<40){tPy=40;} if(tPy>TEMPLE_H-40){tPy=TEMPLE_H-40;}
    float da=hypotf(tAltarX-tPx,tAltarY-tPy);
    if(da<40) templeUiDisp=1; else templeUiDisp=0;
    u32 down=hidKeysDown();
    if((down&KEY_B)){ state=ST_PLAY; }
    if(templeUiDisp){
        static float tc=0; if(tc>0)tc-=dt;
        if(down&(KEY_DUP|KEY_CPAD_UP)) templeSel=(templeSel+2)%3;
        if(down&(KEY_DDOWN|KEY_CPAD_DOWN)) templeSel=(templeSel+1)%3;
        touchPosition tp; hidTouchRead(&tp);
        if((tp.px!=0||tp.py!=0)&&tc<=0){
            int row=(tp.py-30)/60;
            if(row==0) doTempleAction(0);
            else if(row==1) doTempleAction(1);
            else if(row==2) doTempleAction(2);
            tc=0.3f;
        }
        if(down&KEY_A){ doTempleAction(templeSel); }
    }
}
static void drawTempleTop(void){
    tCamx=tPx-SCR_W/2; tCamy=tPy-SCR_H/2;
    if(tCamx<0)tCamx=0; if(tCamy<0)tCamy=0;
    if(tCamx>TEMPLE_W-SCR_W)tCamx=TEMPLE_W-SCR_W;
    if(tCamy>TEMPLE_H-SCR_H)tCamy=TEMPLE_H-SCR_H;
    C2D_TargetClear(top, C(0.18f,0.07f,0.28f));
    C2D_SceneBegin(top);
    C2D_DrawRectSolid(0,0,0,SCR_W,SCR_H, C(0.4f,0.2f,0.4f));
    for(int i=0;i<200;i++){
        float fx=((i*73)% (TEMPLE_W+8))-tCamx;
        float fy=((i*131)% (TEMPLE_H+8))-tCamy;
        if(fx<-10||fx>SCR_W+10||fy<-10||fy>SCR_H+10) continue;
        C2D_DrawRectSolid(fx,fy,0.05f,2,3, C(0.5f,0.25f,0.5f));
    }
    for(int i=0;i<8;i++){
        float cx2=80+i*120, cy2=160;
        float sx=cx2-tCamx, sy=cy2-tCamy;
        if(sx<-20||sx>SCR_W+20||sy<-20||sy>SCR_H+20) continue;
        C2D_DrawRectSolid(sx-6,sy,0.3f,12,120, C(0.3f,0.25f,0.45f));
        C2D_DrawCircleSolid(sx,sy,0.4f,8, C(0.35f,0.3f,0.5f));
    }
    for(int i=0;i<4;i++){
        float bx2=140+i*200, by2=100;
        float sx=bx2-tCamx, sy=by2-tCamy;
        if(sx<-40||sx>SCR_W+40) continue;
        C2D_DrawRectSolid(sx-20,sy,0.3f,40,18, C(0.4f,0.3f,0.3f));
        C2D_DrawRectSolid(sx-20,sy+14,0.31f,40,4, C(0.3f,0.2f,0.2f));
    }
    float ax=tAltarX-tCamx, ay=tAltarY-tCamy;
    C2D_DrawCircleSolid(ax,ay,0.2f,40, C(0.55f,0.1f,0.1f));
    C2D_DrawCircleSolid(ax,ay,0.21f,32, C(0.25f,0.05f,0.05f));
    for(int i=0;i<5;i++){
        float a0=i*2.0f*3.14159f/5.0f-3.14159f/2.0f;
        float a1=(i+2)*2.0f*3.14159f/5.0f-3.14159f/2.0f;
        C2D_DrawLine(ax+cos(a0)*32,ay+sin(a0)*32, C(0.9f,0.3f,0.3f),
                     ax+cos(a1)*32,ay+sin(a1)*32, C(0.9f,0.3f,0.3f),2.0f,0.4f);
    }
    C2D_DrawCircleSolid(ax,ay,0.3f,8, C(1.0f,0.8f,0.2f));
    drawText(ax-40,ay+44,0.5f,"THE ALTAR",1,0.8f,0.4f);
    if(templeUiDisp) drawText(ax-20,ay-60,0.5f,"A to act",1,0.9f,0.4f);
    drawLambfull(tPx-tCamx,tPy-tCamy,1.0f);
    drawText(SCR_W/2-60,SCR_H-20,0.5f,"B=leave",0.9f,0.8f,0.6f);
}
static void drawTempleBottom(void){
    C2D_TargetClear(bottom, C(0.15f,0.05f,0.18f));
    C2D_SceneBegin(bottom);
    C2D_DrawRectSolid(0,0,0,BOT_W,BOT_H, C(0.15f,0.05f,0.18f));
    if(templeUiDisp){
        const char* acts[3]={"PREACH","RITUAL","LEAVE"};
        for(int i=0;i<3;i++){
            int y=30+i*60;
            u32 col=(i==templeSel)? C(0.5f,0.1f,0.55f): C(0.2f,0.08f,0.25f);
            C2D_DrawRectSolid(40,y-4,0.3f,240,40, col);
            float cr=(i==templeSel)?1.0f:0.9f;
            drawText(60,y,0.55f,acts[i],cr,0.9f,0.9f);
        }
    } else {
        drawText(20,30,0.6f,"Move: circle pad",1,0.9f,0.4f);
        drawText(20,60,0.6f,"A: action",1,0.9f,0.4f);
        drawText(20,90,0.6f,"B: leave",1,0.9f,0.4f);
    }
}
static void doTempleAction(int action){
    if(action==0){
        if(followCount>0){
            for(int i=0;i<followCount;i++){ fwM[i]+=10; if(fwM[i]>100)fwM[i]=100; }
            score+=2;
        }
    } else if(action==1){
        if(ritualCd<=0 && score>=6){
            score-=6;
            for(int i=0;i<followCount;i++){ fwM[i]+=30; if(fwM[i]>100)fwM[i]=100; }
            ritualCd=20.0f;
            int gained = followCount>0? followCount/2 : 0;
            score+=gained;
        }
    } else if(action==2){
        state=ST_PLAY;
    }
}

static void drawMenuTop(void){
    C2D_TargetClear(top, white);
    C2D_SceneBegin(top);
    C2D_DrawRectSolid(0,0,0,SCR_W,SCR_H, white);
    float cx=SCR_W/2, cy=SCR_H/2+10, R=90;
    C2D_DrawCircleSolid(cx,cy,0.2f,R, C(0.9f,0.15f,0.15f));
    C2D_DrawCircleSolid(cx,cy,0.21f,R-6, white);
    for(int i=0;i<5;i++){
        float a0=i*2.0f*3.14159f/5.0f-3.14159f/2.0f;
        float a1=(i+2)*2.0f*3.14159f/5.0f-3.14159f/2.0f;
        C2D_DrawLine(cx+cos(a0)*R,cy+sin(a0)*R, C(0.9f,0.15f,0.15f),
                     cx+cos(a1)*R,cy+sin(a1)*R, C(0.9f,0.15f,0.15f),3.0f,0.5f);
    }
    drawLambfull(cx,cy-R-18,1.4f);
    drawText(40,12,0.8f,"CULT OF THE LAMB",0.9f,0.1f,0.1f);
}
static void drawBottomMenu(void){
    C2D_TargetClear(bottom, black);
    C2D_SceneBegin(bottom);
    C2D_DrawRectSolid(0,0,0,BOT_W,BOT_H, black);
    drawText(60,6,0.55f,"CULT OF THE LAMB",1,0.2f,0.2f);
    const char* items[4]={"PLAY","SETTINGS","CREDITS","EXIT"};
    for(int i=0;i<4;i++){
        int y=45+i*50;
        u32 col=(i==menuSel)? C(0.55f,0.1f,0.15f): C(0.18f,0.12f,0.1f);
        C2D_DrawRectSolid(30,y-4,0.3f,260,30, col);
        drawText(60,y-2,0.6f,items[i],0.9f,0.9f,0.9f);
    }
}

static void doTalkAction(int action){
    if(talkTarget<0||talkTarget>=followCount){ talkMenuOpen=0; state=ST_PLAY; return; }
    int i=talkTarget;
    switch(action){
        case 0: fwWork[i]=1; score+=3; break;
        case 1: fwM[i]-=15; score+=4; break;
        case 2: fwM[i]+=20; if(score>=2)score-=2; break;
        case 3: break;
    }
    talkMenuOpen=0;
    state=ST_PLAY;
}

static void update(float dt){
    tclock+=dt;
    if(state==ST_PLAY){
        u32 k=hidKeysHeld();
        float dx=0,dy=0;
        if(k&KEY_CPAD_UP) dy-=1;
        if(k&KEY_CPAD_DOWN) dy+=1;
        if(k&KEY_CPAD_LEFT) dx-=1;
        if(k&KEY_CPAD_RIGHT) dx+=1;
        float len=sqrtf(dx*dx+dy*dy);
        if(len>0){ dx/=len; dy/=len; }
        px+=dx*2.6f*60.f*dt; py+=dy*2.6f*60.f*dt;
        if(px<20){px=20;} if(px>WORLD_W-20){px=WORLD_W-20;}
        if(py<20){py=20;} if(py>WORLD_H-20){py=WORLD_H-20;}
        for(int i=0;i<followCount;i++){
            fwT[i]+=dt*0.6f;
            fwX[i]+=cosf(fwT[i])*0.6f;
            fwY[i]+=sinf(fwT[i]*0.9f)*0.6f;
            if(fwX[i]<20){fwX[i]=20;} if(fwX[i]>WORLD_W-20){fwX[i]=WORLD_W-20;}
            if(fwY[i]<20){fwY[i]=20;} if(fwY[i]>WORLD_H-20){fwY[i]=WORLD_H-20;}
        }
        if(ritualCd>0) ritualCd-=dt;
        if(buildFailMsg>0) buildFailMsg-=dt;
        touchPosition tp;
        hidTouchRead(&tp);
        static float touchCd=0;
        if(touchCd>0) touchCd-=dt;
        if((tp.px!=0||tp.py!=0)&&touchCd<=0){
            float tpx=tp.px, tpy=tp.py;
            if(tpx<160){
                int selt=(int)(tpy/40);
                if(selt>=0&&selt<5) buildSel=selt;
            } else {
                float wx=camx+((tpx-170)/((float)BOT_W-170))*SCR_W + (tpx-170);
                float wy=camy+(tpy/(float)BOT_H)*SCR_H;
                tryBuild(buildSel,wx,wy);
            }
            touchCd=0.3f;
        }
        static float acc=0; acc+=dt;
        if(acc>2.0f){ acc=0;
            int workTotal=0; for(int i=0;i<followCount;i++) if(fwWork[i]) workTotal++;
            int kitchenBonus = hasKitchen?2:0;
            score+= workTotal + followCount/3 + kitchenBonus;
        }
        if(dmgFlash>0) dmgFlash-=dt;
        u32 down=hidKeysDown();
        bool acted=false;
        // top gate -> dungeon hall
        float gx=WORLD_W/2, gy=20;
        if((down&KEY_A)&&hypotf(gx-px,gy-py)<40){ enterHub(); state=ST_DUNGEONHUB; acted=true; }
        // temple
        if(!acted && (down&KEY_A) && hasTemple){
            for(int i=0;i<buildCount;i++){
                if((int)btype[i]==4){
                    if(hypotf(bx[i]-px,by[i]-py)<50){ enterTemple(); state=ST_TEMPLE; acted=true; break; }
                }
            }
        }
        if(!acted && (down&KEY_A)){
            for(int i=0;i<followCount;i++){
                if(hypotf(fwX[i]-px,fwY[i]-py)<28){
                    talkTarget=i; talkMenuSel=0; talkMenuOpen=1; state=ST_TALK; break;
                }
            }
        }
    } else if(state==ST_DUNGEONHUB){
        updateHub(dt);
    } else if(state==ST_DUNGEON){
        updateDungeon(dt);
    } else if(state==ST_TALK){
        touchPosition tp;
        hidTouchRead(&tp);
        u32 down=hidKeysDown();
        if(down&KEY_B){ talkMenuOpen=0; state=ST_PLAY; }
        if(down&(KEY_DUP|KEY_CPAD_UP)) talkMenuSel=(talkMenuSel+3)%4;
        if(down&(KEY_DDOWN|KEY_CPAD_DOWN)) talkMenuSel=(talkMenuSel+1)%4;
        static float tCd=0; if(tCd>0)tCd-=dt;
        if((tp.px!=0||tp.py!=0)&&tCd<=0){
            int row=(tp.py-20)/50;
            if(row>=0&&row<4){ talkMenuSel=row; doTalkAction(row); }
            tCd=0.3f;
        }
        if(down&KEY_A){ doTalkAction(talkMenuSel); }
    } else if(state==ST_TEMPLE){
        updateTemple(dt);
    } else if(state==ST_MENU){
        touchPosition tp;
        hidTouchRead(&tp);
        static float mCd=0;
        if(mCd>0)mCd-=dt;
        if((tp.px!=0||tp.py!=0)&&mCd<=0){
            int row=(tp.py-20)/50;
            if(row>=0&&row<4) menuSel=row;
            if(row==0){ state=ST_PLAY; startGame(); }
            else if(row==3) wantExit=true;
            mCd=0.3f;
        }
    }
}

int main(void){
    srand((unsigned)time(NULL));
    gfxInitDefault();
    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
    C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
    C2D_Prepare();
    top=C2D_CreateScreenTarget(GFX_TOP,GFX_LEFT);
    bottom=C2D_CreateScreenTarget(GFX_BOTTOM,GFX_LEFT);
    sysFont=C2D_FontLoadSystem(CFG_REGION_USA);
    tBuf=C2D_TextBufNew(4096);
    bg=C(0.2f,0.5f,0.15f);
    red=C(0.85f,0.05f,0.1f);
    cream=C(0.95f,0.9f,0.8f);
    black=C(0.05f,0.05f,0.05f);
    white=C(1,1,1);
    grey=C(0.45f,0.42f,0.42f);
    yellow=C(1,0.85f,0.2f);

    while(aptMainLoop()){
        hidScanInput();
        u32 down=hidKeysDown();
        if(down&KEY_START) break;
        if(wantExit) break;
        update(1.0f/60.0f);
        C2D_TextBufClear(tBuf);
        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
        if(state==ST_PLAY){ drawGame(); drawBottomGame(); }
        else if(state==ST_DUNGEONHUB){ drawHubTop(); drawHubBottom(); }
        else if(state==ST_DUNGEON){ drawDungeonTop(); drawDungeonBottom(); }
        else if(state==ST_TALK){ drawTalkTop(); drawBottomGame(); }
        else if(state==ST_TEMPLE){ drawTempleTop(); drawTempleBottom(); }
        else { drawMenuTop(); drawBottomMenu(); }
        C3D_FrameEnd(0);
    }
    C2D_TextBufDelete(tBuf);
    C2D_FontFree(sysFont);
    C2D_Fini();
    C3D_Fini();
    gfxExit();
    return 0;
}
