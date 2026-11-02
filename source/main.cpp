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
#define MAX_BUILD 32
#define MAX_FOLLOW 24
#define ALTAR_R 16
#define MAX_ENEMY 8

static u32 C(float r,float g,float b){
    return C2D_Color32((int)(r*255),(int)(g*255),(int)(b*255),255);
}

// ---- ALL forward declarations ----
static void startGame(void);
static void updateCamera(void);
static void addFollower(float x,float y,const char* nm);
static void addRandomFollower(float x,float y);
static void enterDungeon(void);
static void returnToCult(void);
static void updateDungeon(float dt);
static void drawText(float x,float y,float scale,const char* s,float r,float g,float b);
static void drawLambfull(float x,float y,float sc);
static void drawFollowerWander(float ox,float oy);
static void drawBottomGrass(void);
static void drawBuilding(float wx,float wy,int type);
static void drawHouses(float ox,float oy);
static void drawGrassBg(void);
static void drawTalkTop(void);
static void drawTempleTop(void);
static void drawTempleBottom(void);
static void doTalkAction(int action);
static void doTempleAction(int action);

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
static int talkTarget;
static int talkMenuSel;
static int talkMenuOpen;
static int buildSel;

enum GameState { ST_MENU, ST_PLAY, ST_DUNGEON, ST_TALK, ST_TEMPLE };
static GameState state = ST_MENU;
static int menuSel = 0;
static float tclock;
static bool wantExit=false;
static float templeMenuSel;
static float ritualCd;

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
    score=12; buildCount=0; hasKitchen=0; hasTemple=0;
    altarX=WORLD_W/2; altarY=WORLD_H/2;
    px=altarX+40; py=altarY; php=pmaxhp; dmgFlash=0;
    ritualCd=0;
    addRandomFollower(altarX-40, altarY+20);
    addRandomFollower(altarX+40, altarY+10);
    addRandomFollower(altarX,     altarY+40);
    camx=px-SCR_W/2; camy=py-SCR_H/2;
    talkTarget=-1; talkMenuOpen=0; buildSel=0;
}

static void tryBuild(int type,float x,float y){
    if(buildCount>=MAX_BUILD) return;
    int cost = (type==0)?5 : (type==1)?4 : (type==2)?8 : (type==3)?2 : 10;
    if(score<cost) return;
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

static float dpx,dpy;
static int dungeonBoss;
static float ex[MAX_ENEMY],ey[MAX_ENEMY],ehp[MAX_ENEMY],espeed[MAX_ENEMY]; static int ecount;
static int enemiesAlive;
static float dmgCd,enemyAtkCd,dmgRed;
static int bossDead;

static void spawnEnemy(float x,float y,float hp,float sp,bool boss){
    if(ecount>=MAX_ENEMY) return;
    ex[ecount]=x; ey[ecount]=y; ehp[ecount]=hp; espeed[ecount]=sp; ecount++;
    enemiesAlive++; if(boss) dungeonBoss=1;
}
static void enterDungeon(void){
    ecount=0; enemiesAlive=0; dungeonBoss=0;
    dpx=SCR_W/2; dpy=SCR_H/2; dmgCd=0; enemyAtkCd=0; dmgRed=0;
    bossDead=0; php=pmaxhp;
    spawnEnemy(60,60,30,1.2f,false);
    spawnEnemy(340,60,30,1.3f,false);
    spawnEnemy(60,180,30,1.2f,false);
    spawnEnemy(200,40,30,1.4f,false);
    spawnEnemy(200,200,100,1.6f,true);
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
    if(dpx<12){dpx=12;} if(dpx>SCR_W-12){dpx=SCR_W-12;}
    if(dpy<12){dpy=12;} if(dpy>SCR_H-12){dpy=SCR_H-12;}
    for(int i=0;i<ecount;i++){
        if(ehp[i]<=0) continue;
        float a=atan2f(dpy-ey[i],dpx-ex[i]);
        float sp=espeed[i]*60.f*dt;
        ex[i]+=cos(a)*sp; ey[i]+=sin(a)*sp;
    }
    if(dmgCd>0) dmgCd-=dt;
    u32 down=hidKeysDown();
    if((down&KEY_A)&&dmgCd<=0){
        dmgCd=0.25f;
        for(int i=0;i<ecount;i++){
            if(ehp[i]<=0) continue;
            if(hypotf(ex[i]-dpx,ey[i]-dpy)<26){ ehp[i]-=25; if(ehp[i]<=0) enemiesAlive--; break; }
        }
    }
    enemyAtkCd-=dt;
    if(enemyAtkCd<=0){
        enemyAtkCd=1.0f;
        for(int i=0;i<ecount;i++){
            if(ehp[i]<=0) continue;
            if(hypotf(ex[i]-dpx,ey[i]-dpy)<20){ php-=8; dmgRed=3; if(php<0)php=0; }
        }
    }
    if(dmgRed>0) dmgRed-=dt;
    if(enemiesAlive<=0 && !bossDead){
        bossDead=1;
        addRandomFollower(altarX+rnd(-30,30), altarY+rnd(0,40));
        score+=5;
    }
}
static void returnToCult(void){
    state=ST_PLAY;
    php=pmaxhp;
    talkMenuOpen=0;
}

static void drawEnemy(float x,float y,bool boss){
    float s=boss?1.6f:1.0f;
    C2D_DrawRectSolid(x-7*s,y+2*s,0.3f,14*s,16*s, C(0.15f,0.15f,0.15f));
    C2D_DrawCircleSolid(x,y,0.3f,8*s, C(0.1f,0.1f,0.1f));
    C2D_DrawRectSolid(x-1*s,y-2*s,0.4f,2*s,6*s, C(1,0,0));
    C2D_DrawRectSolid(x-9*s,y+4*s,0.4f,3*s,9*s, C(0.7f,0.7f,0.8f));
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
    if(type==0){ // house
        C2D_DrawTriangle(x,y-14, C(0.7f,0.15f,0.15f), x-14,y, C(0.7f,0.15f,0.15f), x+14,y, C(0.7f,0.15f,0.15f),0.4f);
        C2D_DrawRectSolid(x-14,y,0.39f,28,16, C(0.8f,0.7f,0.5f));
        C2D_DrawRectSolid(x-4,y+6,0.41f,8,10, C(0.4f,0.25f,0.1f));
    } else if(type==1){ // toilet
        C2D_DrawRectSolid(x-8,y,0.39f,16,14, C(0.75f,0.75f,0.8f));
        C2D_DrawCircleSolid(x,y-2,0.4f,5, C(0.9f,0.9f,0.95f));
        C2D_DrawRectSolid(x-3,y+6,0.41f,6,8, C(0.4f,0.4f,0.45f));
    } else if(type==2){ // kitchen
        C2D_DrawTriangle(x,y-12, C(0.8f,0.4f,0.2f), x-14,y, C(0.8f,0.4f,0.2f), x+14,y, C(0.8f,0.4f,0.2f),0.4f);
        C2D_DrawRectSolid(x-14,y,0.39f,28,16, C(0.6f,0.5f,0.35f));
        C2D_DrawCircleSolid(x,y-4,0.42f,6, C(0.3f,0.3f,0.3f));
        C2D_DrawRectSolid(x-2,y-8,0.43f,4,2, C(0.5f,0.5f,0.5f));
    } else if(type==3){ // decor flower
        C2D_DrawRectSolid(x-1,y,0.4f,2,8, C(0.2f,0.5f,0.2f));
        C2D_DrawCircleSolid(x,y-3,0.41f,4, C(1,0.4f,0.6f));
        C2D_DrawCircleSolid(x-3,y-1,0.41f,2, C(1,0.9f,0.3f));
    } else { // type 4 temple
        C2D_DrawTriangle(x,y-20, C(0.5f,0.1f,0.55f), x-18,y-4, C(0.5f,0.1f,0.55f), x+18,y-4, C(0.5f,0.1f,0.55f),0.4f);
        C2D_DrawRectSolid(x-18,y-4,0.39f,36,4, C(0.35f,0.1f,0.4f));
        for(int i=-1;i<=1;i++){
            C2D_DrawRectSolid(x-10+i*10,y-1,0.41f,6,14, C(0.3f,0.08f,0.35f));
            C2D_DrawTriangle(x-7+i*10,y-15, C(0.55f,0.15f,0.6f), x-4+i*10,y-8, C(0.55f,0.15f,0.6f), x-1+i*10,y-15, C(0.55f,0.15f,0.6f),0.42f);
        }
        C2D_DrawCircleSolid(x,y-8,0.44f,3, C(1,0.85f,0.2f));
    }
}

static void drawHouses(float ox,float oy){
    for(int i=0;i<buildCount;i++){
        float x=bx[i]-ox, y=by[i]-oy;
        if(x<-30||x>SCR_W+30||y<-30||y>SCR_H+30) continue;
        drawBuilding(x,y,(int)btype[i]);
    }
}

static void drawPortal(float ox,float oy){
    float gx=WORLD_W/2-ox, gy=WORLD_H-60-oy;
    C2D_DrawCircleSolid(gx,gy,0.2f,16, C(0.4f,0.05f,0.12f));
    C2D_DrawCircleSolid(gx,gy,0.21f,13, C(0.6f,0.1f,0.2f));
    for(int i=0;i<5;i++){
        float a0=i*2.0f*3.14159f/5.0f-3.14159f/2.0f;
        float a1=(i+2)*2.0f*3.14159f/5.0f-3.14159f/2.0f;
        C2D_DrawLine(gx+cos(a0)*13,gy+sin(a0)*13, C(1,0.4f,0.4f),
                     gx+cos(a1)*13,gy+sin(a1)*13, C(1,0.4f,0.4f),1.0f,0.4f);
    }
    drawText(gx-30,gy+18,0.5f,"DUNGEON",1,0.5f,0.5f);
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
    drawPortal(ox,oy);
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
            float cr=(i==talkMenuSel)?1.0f:0.9f, cg=(i==talkMenuSel)?0.85f:0.9f, cb=(i==talkMenuSel)?0.3f:0.9f;
            drawText(40,y,0.5f,acts[i],cr,cg,cb);
        }
    } else {
        const char* types[5]={"HOUSE","TOILET","KITCHEN","DECOR","TEMPLE"};
        for(int i=0;i<5;i++){
            int y=26+i*40;
            u32 col=(i==buildSel)? C(0.55f,0.1f,0.15f): C(0.15f,0.4f,0.1f);
            C2D_DrawRectSolid(20,y-3,0.3f,132,30, col);
            drawText(26,y,0.42f,types[i],0.95f,0.95f,0.95f);
        }
        C2D_DrawRectSolid(180,80,0.3f,125,90, C(0.1f,0.3f,0.08f));
        drawText(186,120,0.5f,"TAP = build",1,0.9f,0.4f);
        drawText(186,184,0.42f,"TAP temple = enter",0.9f,0.7f,0.5f);
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

static void drawDungeon(void){
    C2D_TargetClear(top, C(0.08f,0.05f,0.05f));
    C2D_SceneBegin(top);
    C2D_DrawRectSolid(0,0,0,SCR_W,SCR_H, C(0.1f,0.06f,0.06f));
    for(int i=0;i<SCR_W;i+=32) C2D_DrawRectSolid(i,0,0.1f,1,SCR_H, C(0,0,0));
    for(int j=0;j<SCR_H;j+=32) C2D_DrawRectSolid(0,j,0.1f,SCR_W,1, C(0,0,0));
    C2D_DrawRectSolid(0,0,0.2f,SCR_W,4, C(0.4f,0.2f,0.2f));
    C2D_DrawRectSolid(0,SCR_H-4,0.2f,SCR_W,4, C(0.4f,0.2f,0.2f));
    C2D_DrawRectSolid(0,0,0.2f,4,SCR_H, C(0.4f,0.2f,0.2f));
    C2D_DrawRectSolid(SCR_W-4,0,0.2f,4,SCR_H, C(0.4f,0.2f,0.2f));
    for(int i=0;i<ecount;i++){
        if(ehp[i]<=0) continue;
        bool boss=(dungeonBoss&&i==ecount-1);
        drawEnemy(ex[i],ey[i],boss);
    }
    drawLambfull(dpx,dpy,0.7f);
    float mx=SCR_W/2-80, my=12;
    C2D_DrawRectSolid(mx,my,0.5f,160,8, C(0.2f,0.1f,0.1f));
    if(bossDead) C2D_DrawRectSolid(mx+1,my+1,0.51f,158,6, C(1,0.3f,0.2f));
    if(dmgRed>0) C2D_DrawRectSolid(0,0,0.9f,SCR_W,SCR_H, C2D_Color32(255,0,0,60));
    if(enemiesAlive<=0){
        drawText(SCR_W/2-100,SCR_H/2,0.6f,"CLEAR! Reward earned",1,0.9f,0.3f);
    }
}

static void drawTempleTop(void){
    C2D_TargetClear(top, C(0.2f,0.08f,0.25f));
    C2D_SceneBegin(top);
    C2D_DrawRectSolid(0,0,0,SCR_W,SCR_H, C(0.2f,0.08f,0.25f));
    for(int i=0;i<50;i++){
        float gx=(i*41+5)% (SCR_W+8);
        float gy=(i*67+12)% SCR_H;
        C2D_DrawCircleSolid(gx,gy,0.2f,2, C(0.35f,0.15f,0.4f));
    }
    drawText(40,16,0.7f,"TEMPPLE OF THE CULT",1,0.85f,0.4f);
    if(ritualCd>0){
        char rt[40]; snprintf(rt,40,"Ritual ready in %.0f",ritualCd);
        drawText(40,40,0.5f,rt,1,0.7f,0.4f);
    } else {
        drawText(40,40,0.5f,"Ritual is READY! (feast cost 6)",1,0.9f,0.5f);
    }
    char s2[40]; snprintf(s2,40,"Followers: %d  Food: %d",followCount,score);
    drawText(40,60,0.5f,s2,1,0.9f,0.4f);
}

static void drawTempleBottom(void){
    C2D_TargetClear(bottom, C(0.15f,0.05f,0.18f));
    C2D_SceneBegin(bottom);
    C2D_DrawRectSolid(0,0,0,BOT_W,BOT_H, C(0.15f,0.05f,0.18f));
    const char* acts[3]={"PREACH","RITUAL","LEAVE"};
    for(int i=0;i<3;i++){
        int y=30+i*60;
        u32 col=(i==(int)templeMenuSel)? C(0.5f,0.1f,0.55f): C(0.2f,0.08f,0.25f);
        C2D_DrawRectSolid(40,y-4,0.3f,240,40, col);
        float cr=(i==(int)templeMenuSel)?1.0f:0.9f;
        drawText(60,y,0.55f,acts[i],cr,0.9f,0.9f);
    }
}

static void doTempleAction(int action){
    if(action==0){ // PREACH
        if(followCount>0){
            for(int i=0;i<followCount;i++){ fwM[i]+=10; if(fwM[i]>100)fwM[i]=100; }
            score+=2;
        }
        state=ST_PLAY;
    } else if(action==1){ // RITUAL feast
        if(ritualCd<=0 && score>=6){
            score-=6;
            for(int i=0;i<followCount;i++){ fwM[i]+=30; if(fwM[i]>100)fwM[i]=100; }
            ritualCd=20.0f;
            int gained = followCount>0? followCount/2 : 0;
            score+=gained;
        }
        state=ST_PLAY;
    } else { // LEAVE
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
        float cr=(i==menuSel)?1.0f:0.9f, cg=(i==menuSel)?0.85f:0.9f, cb=(i==menuSel)?0.3f:0.9f;
        drawText(60,y-2,0.6f,items[i],cr,cg,cb);
    }
}

static void drawBottomDungeon(void){
    C2D_TargetClear(bottom, C(0.08f,0.04f,0.04f));
    C2D_SceneBegin(bottom);
    C2D_DrawRectSolid(0,0,0,BOT_W,BOT_H, C(0.1f,0.05f,0.05f));
    drawText(30,20,0.6f,"A = FIGHT",1,0.6f,0.5f);
    drawText(30,60,0.6f,"B = leave",1,0.6f,0.5f);
    char h[40]; snprintf(h,40,"HP %d",(int)php);
    drawText(30,100,0.6f,h,1,0.8f,0.4f);
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
        float gx=WORLD_W/2, gy=WORLD_H-60;
        bool acted=false;
        if((down&KEY_A)&&hypotf(gx-px,gy-py)<30){ state=ST_DUNGEON; enterDungeon(); acted=true; }
        if(!acted && (down&KEY_A) && hasTemple){
            for(int i=0;i<buildCount;i++){
                if((int)btype[i]==4){
                    if(hypotf(bx[i]-px,by[i]-py)<32){ templeMenuSel=0; state=ST_TEMPLE; acted=true; break; }
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
    } else if(state==ST_DUNGEON){
        updateDungeon(dt);
        u32 down=hidKeysDown();
        if(down&KEY_B){ returnToCult(); }
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
        touchPosition tp;
        hidTouchRead(&tp);
        u32 down=hidKeysDown();
        if(down&KEY_B){ state=ST_PLAY; }
        if(down&(KEY_DUP|KEY_CPAD_UP)) templeMenuSel=fmodf(templeMenuSel+2,3);
        if(down&(KEY_DDOWN|KEY_CPAD_DOWN)) templeMenuSel=fmodf(templeMenuSel+1,3);
        static float tCd=0; if(tCd>0)tCd-=dt;
        if((tp.px!=0||tp.py!=0)&&tCd<=0){
            int row=(tp.py-30)/60;
            if(row>=0&&row<3){ templeMenuSel=row; doTempleAction(row); }
            tCd=0.3f;
        }
        if(down&KEY_A){ doTempleAction((int)templeMenuSel); }
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
        else if(state==ST_DUNGEON){ drawDungeon(); drawBottomDungeon(); }
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
