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
#define MAX_BUILD 16
#define MAX_FOLLOW 24
#define ALTAR_R 16
#define MAX_ENEMY 8

#define CULT_MELODY_LEN 12
static const float cultMelody[CULT_MELODY_LEN] = {
    220, 220, 262, 294, 262, 220, 196, 196, 220, 262, 220, 196
};
#define DUNGEON_MELODY_LEN 12
static const float dungeonMelody[DUNGEON_MELODY_LEN] = {
    440, 440, 392, 440, 523, 440, 392, 392, 440, 523, 440, 392
};
static float musicTimer = 0;
static int musicIdx = 0;

static void playFrequency(float freq){
    u32 encodedFreq = (u32)(freq * 512);
    csndPlaySound(8, SOUND_FORMAT_PSG | SOUND_ENABLE | encodedFreq, 32767);
}

static void updateMusic(bool dungeon){
    musicTimer += 1.0f/60.0f;
    if(musicTimer >= 0.35f){
        musicTimer = 0;
        float freq = dungeon ? dungeonMelody[musicIdx % DUNGEON_MELODY_LEN]
                             : cultMelody[musicIdx % CULT_MELODY_LEN];
        musicIdx++;
        playFrequency(freq);
    }
}

static u32 C(float r,float g,float b){
    return C2D_Color32((int)(r*255),(int)(g*255),(int)(b*255),255);
}

static float px,py,php,pmaxhp=100.0f;
static float camx,camy;
static float bx[MAX_BUILD],by[MAX_BUILD]; static int buildCount;
static float fwX[MAX_FOLLOW],fwY[MAX_FOLLOW],fwT[MAX_FOLLOW]; static int followCount;
static char fwName[MAX_FOLLOW][24];
static float altarX,altarY;
static int score;
static float dmgFlash;

#define DUN_W 400
#define DUN_H 240
static float dpx,dpy;
static int dungeonActive;
static int dungeonBoss;
static float ex[MAX_ENEMY],ey[MAX_ENEMY],ehp[MAX_ENEMY],espeed[MAX_ENEMY]; static int ecount;
static int enemiesAlive;
static float dmgCd;
static float enemyAtkCd;
static float dmgRed;
static int bossDead;

enum GameState { ST_MENU, ST_PLAY, ST_DUNGEON };
static GameState state = ST_MENU;
static int menuSel = 0;
static float tclock;
static bool wantExit=false;

static C3D_RenderTarget *top, *bottom;
static u32 bg, red, cream, black, white;
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

static void addFollower(float x,float y,const char* nm){
    if(followCount>=MAX_FOLLOW) return;
    fwX[followCount]=x; fwY[followCount]=y;
    fwT[followCount]=rnd(0,6.28f);
    snprintf(fwName[followCount],24,"%s",nm);
    followCount++;
}
static void addRandomFollower(float x,float y){
    addFollower(x,y, names[rand()%nameCount]);
}

static void startGame(void){
    score=10; buildCount=0; followCount=0;
    altarX=WORLD_W/2; altarY=WORLD_H/2;
    px=altarX+40; py=altarY; php=pmaxhp; dmgFlash=0;
    addRandomFollower(altarX-40, altarY+20);
    addRandomFollower(altarX+40, altarY+10);
    addRandomFollower(altarX,     altarY+40);
    camx=px-SCR_W/2; camy=py-SCR_H/2;
}

static void buildHouse(float x,float y){
    if(buildCount>=MAX_BUILD) return;
    if(score<5) return;
    bx[buildCount]=x; by[buildCount]=y; buildCount++;
    score-=5;
    addRandomFollower(x+rnd(-20,20), y+20);
}

static void updateCamera(void){
    camx=px-SCR_W/2; camy=py-SCR_H/2;
    if(camx<0){camx=0;} if(camy<0){camy=0;}
    if(camx>WORLD_W-SCR_W){camx=WORLD_W-SCR_W;}
    if(camy>WORLD_H-SCR_H){camy=WORLD_H-SCR_H;}
}

static void spawnEnemy(float x,float y,float hp,float sp,bool boss){
    if(ecount>=MAX_ENEMY) return;
    ex[ecount]=x; ey[ecount]=y; ehp[ecount]=hp; espeed[ecount]=sp;
    ecount++;
    enemiesAlive++;
    if(boss) dungeonBoss=1;
}

static void enterDungeon(void){
    dungeonActive=1; ecount=0; enemiesAlive=0; dungeonBoss=0;
    dpx=SCR_W/2; dpy=SCR_H/2; dmgCd=0; enemyAtkCd=0; dmgRed=0;
    bossDead=0; php=pmaxhp; musicIdx=0; musicTimer=0;
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
    if(dpx<12){dpx=12;} if(dpx>DUN_W-12){dpx=DUN_W-12;}
    if(dpy<12){dpy=12;} if(dpy>DUN_H-12){dpy=DUN_H-12;}

    for(int i=0;i<ecount;i++){
        if(ehp[i]<=0) continue;
        float a=atan2f(dpy-ey[i], dpx-ex[i]);
        float sp=espeed[i]*60.f*dt;
        ex[i]+=cos(a)*sp; ey[i]+=sin(a)*sp;
    }

    if(dmgCd>0) dmgCd-=dt;
    u32 down=hidKeysDown();
    if((down&KEY_A) && dmgCd<=0){
        dmgCd=0.25f;
        for(int i=0;i<ecount;i++){
            if(ehp[i]<=0) continue;
            if(hypotf(ex[i]-dpx,ey[i]-dpy)<26){
                ehp[i]-=25;
                if(ehp[i]<=0){ enemiesAlive--; }
                break;
            }
        }
    }

    enemyAtkCd-=dt;
    if(enemyAtkCd<=0){
        enemyAtkCd=1.0f;
        for(int i=0;i<ecount;i++){
            if(ehp[i]<=0) continue;
            if(hypotf(ex[i]-dpx,ey[i]-dpy)<20){
                php-=8; dmgRed=3.0f;
                if(php<0) php=0;
            }
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
    dungeonActive=0;
    php=pmaxhp;
    musicIdx=0; musicTimer=0;
}

static void drawLambfull(float x,float y,float sc){
    C2D_DrawCircleSolid(x,y+6*sc,0.5f,14*sc, C(0.9f,0.1f,0.12f));
    C2D_DrawRectSolid(x-11*sc,y-2*sc,0.6f,22*sc,18*sc, C(0.85f,0.05f,0.1f));
    C2D_DrawCircleSolid(x,y-4*sc,0.5f,13*sc, C(1,1,1));
    C2D_DrawCircleSolid(x,y-4*sc,0.49f,9*sc, C(0.05f,0.05f,0.05f));
    C2D_DrawCircleSolid(x-4*sc,y-5*sc,0.55f,2.5f*sc, C(1,0,0));
    C2D_DrawCircleSolid(x+4*sc,y-5*sc,0.55f,2.5f*sc, C(1,0,0));
    C2D_DrawRectSolid(x-5*sc,y-2*sc,0.56f,1.5f*sc,4*sc, C(1,0,0));
    C2D_DrawRectSolid(x+3.5f*sc,y-2*sc,0.56f,1.5f*sc,4*sc, C(1,0,0));
    C2D_DrawRectSolid(x-13*sc,y-9*sc,0.5f,4*sc,8*sc, black);
    C2D_DrawRectSolid(x+9*sc,y-9*sc,0.5f,4*sc,8*sc, black);
    C2D_DrawCircleSolid(x,y+6*sc,0.55f,3.5f*sc, C(1,0.85f,0.2f));
}

static void drawEnemy(float x,float y,bool boss){
    float s = boss?1.6f:1.0f;
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
        float a0=i*2.0f*3.14159f/5.0f - 3.14159f/2.0f;
        float a1=(i+2)*2.0f*3.14159f/5.0f - 3.14159f/2.0f;
        C2D_DrawLine(x+cos(a0)*ALTAR_R, y+sin(a0)*ALTAR_R, C(0.9f,0.3f,0.3f),
                     x+cos(a1)*ALTAR_R, y+sin(a1)*ALTAR_R, C(0.9f,0.3f,0.3f), 1.0f, 0.5f);
    }
    C2D_DrawCircleSolid(x,y,0.3f,5, C(1.0f,0.8f,0.2f));
}

static void drawFollowerWander(float ox,float oy){
    int n=followCount>10?10:followCount;
    for(int i=0;i<n;i++){
        float wx=fwX[i]-ox, wy=fwY[i]-oy;
        C2D_DrawRectSolid(wx-4,wy+2,0.3f,8,9, C(0.75f,0.1f,0.1f));
        C2D_DrawCircleSolid(wx,wy,0.3f,6, C(0.35f,0.6f,0.25f));
        C2D_DrawRectSolid(wx-5,wy-2,0.32f,10,3, C(0.95f,0.95f,0.95f));
        C2D_DrawRectSolid(wx-5,wy+1,0.33f,10,2, C(0.95f,0.95f,0.95f));
        C2D_DrawTriangle(wx, wy+1, C(0.9f,0.1f,0.1f), wx-2, wy-1, C(0.9f,0.1f,0.1f), wx+2, wy-1, C(0.9f,0.1f,0.1f), 0.35f);
        drawText(wx-14, wy-14, 0.4f, fwName[i], 1,1,0.6f);
    }
}

static void drawHouses(float ox,float oy){
    for(int i=0;i<buildCount;i++){
        float x=bx[i]-ox, y=by[i]-oy;
        C2D_DrawTriangle(x,y-14, C(0.7f,0.15f,0.15f), x-14,y, C(0.7f,0.15f,0.15f), x+14,y, C(0.7f,0.15f,0.15f), 0.4f);
        C2D_DrawRectSolid(x-14,y,0.39f,28,16, C(0.8f,0.7f,0.5f));
        C2D_DrawRectSolid(x-4,y+6,0.41f,8,10, C(0.4f,0.25f,0.1f));
    }
}

static void drawPortal(float ox,float oy){
    float gx=WORLD_W/2 - ox, gy=WORLD_H-60 - oy;
    C2D_DrawCircleSolid(gx,gy,0.2f,16, C(0.4f,0.05f,0.12f));
    C2D_DrawCircleSolid(gx,gy,0.21f,13, C(0.6f,0.1f,0.2f));
    for(int i=0;i<5;i++){
        float a0=i*2.0f*3.14159f/5.0f - 3.14159f/2.0f;
        float a1=(i+2)*2.0f*3.14159f/5.0f - 3.14159f/2.0f;
        C2D_DrawLine(gx+cos(a0)*13, gy+sin(a0)*13, C(1,0.4f,0.4f),
                     gx+cos(a1)*13, gy+sin(a1)*13, C(1,0.4f,0.4f), 1.0f, 0.4f);
    }
    drawText(gx-30, gy+18, 0.5f, "DUNGEON", 1,0.5f,0.5f);
}

static void drawGame(void){
    updateCamera();
    float ox=camx, oy=camy;
    C2D_TargetClear(top, bg);
    C2D_SceneBegin(top);
    C2D_DrawRectSolid(0,0,0,SCR_W,SCR_H, bg);
    for(int wx=(int)ox/40*40;wx<ox+SCR_W;wx+=40)
        C2D_DrawRectSolid(wx-ox,0,0.1f,1,SCR_H, C(0,0,0));
    for(int wy=(int)oy/40*40;wy<oy+SCR_H;wy+=40)
        C2D_DrawRectSolid(0,wy-oy,0.1f,SCR_W,1, C(0,0,0));
    C2D_DrawRectSolid(-ox,-oy,0.2f,WORLD_W,4, C(0.4f,0.3f,0.2f));
    C2D_DrawRectSolid(-ox,WORLD_H-4-oy,0.2f,WORLD_W,4, C(0.4f,0.3f,0.2f));
    C2D_DrawRectSolid(-ox,-oy,0.2f,4,WORLD_H, C(0.4f,0.3f,0.2f));
    C2D_DrawRectSolid(WORLD_W-4-ox,-oy,0.2f,4,WORLD_H, C(0.4f,0.3f,0.2f));
    drawAltar(ox,oy);
    drawPortal(ox,oy);
    drawHouses(ox,oy);
    drawFollowerWander(ox,oy);
    drawLambfull(px-ox,py-oy,1.0f);
    if(dmgFlash>0) C2D_DrawRectSolid(0,0,0.9f,SCR_W,SCR_H, C2D_Color32(255,0,0,50));
    char h[32]; snprintf(h,32,"Food:%d Cult:%d",score,followCount);
    drawText(8,30,0.5f,h,1,0.85f,0.3f);
}

static void drawDungeon(void){
    C2D_TargetClear(top, C(0.08f,0.05f,0.05f));
    C2D_SceneBegin(top);
    C2D_DrawRectSolid(0,0,0,DUN_W,DUN_H, C(0.1f,0.06f,0.06f));
    for(int i=0;i<DUN_W;i+=32) C2D_DrawRectSolid(i,0,0.1f,1,DUN_H, C(0,0,0));
    for(int j=0;j<DUN_H;j+=32) C2D_DrawRectSolid(0,j,0.1f,DUN_W,1, C(0,0,0));
    C2D_DrawRectSolid(0,0,0.2f,DUN_W,4, C(0.4f,0.2f,0.2f));
    C2D_DrawRectSolid(0,DUN_H-4,0.2f,DUN_W,4, C(0.4f,0.2f,0.2f));
    C2D_DrawRectSolid(0,0,0.2f,4,DUN_H, C(0.4f,0.2f,0.2f));
    C2D_DrawRectSolid(DUN_W-4,0,0.2f,4,DUN_H, C(0.4f,0.2f,0.2f));
    for(int i=0;i<ecount;i++){
        if(ehp[i]<=0) continue;
        bool boss = (dungeonBoss && i==ecount-1);
        drawEnemy(ex[i],ey[i],boss);
    }
    drawLambfull(dpx,dpy,1.0f);
    float mx=DUN_W/2-80, my=12;
    C2D_DrawRectSolid(mx,my,0.5f,160,8, C(0.2f,0.1f,0.1f));
    if(bossDead) C2D_DrawRectSolid(mx+1,my+1,0.51f,158,6, C(1,0.3f,0.2f));
    if(dmgRed>0) C2D_DrawRectSolid(0,0,0.9f,DUN_W,DUN_H, C2D_Color32(255,0,0,60));
    if(enemiesAlive<=0){
        char w[40]; snprintf(w,40,"CLEAR! Reward earned");
        drawText(DUN_W/2-110, DUN_H/2, 0.6f, w, 1,0.9f,0.3f);
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
        float a0=i*2.0f*3.14159f/5.0f - 3.14159f/2.0f;
        float a1=(i+2)*2.0f*3.14159f/5.0f - 3.14159f/2.0f;
        C2D_DrawLine(cx+cos(a0)*R, cy+sin(a0)*R, C(0.9f,0.15f,0.15f),
                     cx+cos(a1)*R, cy+sin(a1)*R, C(0.9f,0.15f,0.15f), 3.0f, 0.5f);
    }
    drawLambfull(cx, cy-R-18, 1.3f);
    drawText(40, 12, 0.8f, "CULT OF THE LAMB", 0.9f,0.1f,0.1f);
}

static void drawBottomMenu(void){
    C2D_TargetClear(bottom, black);
    C2D_SceneBegin(bottom);
    C2D_DrawRectSolid(0,0,0,BOT_W,BOT_H, black);
    drawText(60,6,0.55f,"CULT OF THE LAMB",1,0.2f,0.2f);
    const char* items[4]={"PLAY","SETTINGS","CREDITS","EXIT"};
    for(int i=0;i<4;i++){
        int y=45+i*50;
        if(i==menuSel){
            C2D_DrawRectSolid(30,y-4,0.3f,260,30, C(0.55f,0.1f,0.15f));
        } else {
            C2D_DrawRectSolid(30,y-4,0.3f,260,30, C(0.18f,0.12f,0.1f));
        }
        float cr=(i==menuSel)?1.0f:0.9f, cg=(i==menuSel)?0.85f:0.9f, cb=(i==menuSel)?0.3f:0.9f;
        drawText(60, y-2, 0.6f, items[i], cr,cg,cb);
    }
}

static void drawBottomGame(void){
    C2D_TargetClear(bottom, C(0.1f,0.07f,0.05f));
    C2D_SceneBegin(bottom);
    C2D_DrawRectSolid(0,0,0,BOT_W,BOT_H, C(0.12f,0.09f,0.06f));
    for(int i=0;i<BOT_W;i+=32) C2D_DrawRectSolid(i,0,0.1f,1,BOT_H, C(0,0,0));
    for(int j=0;j<BOT_H;j+=32) C2D_DrawRectSolid(0,j,0.1f,BOT_W,1, C(0,0,0));
    C2D_DrawTriangle(160,100, C(0.8f,0.3f,0.3f), 146,112, C(0.8f,0.3f,0.3f), 174,112, C(0.8f,0.3f,0.3f), 0.6f);
    C2D_DrawRectSolid(148,112,0.59f,26,12, C(0.85f,0.75f,0.55f));
    drawText(120,100,0.5f,"TAP TO BUILD",1,0.9f,0.4f);
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

static void update(float dt){
    tclock+=dt;
    if(state==ST_PLAY){
        updateMusic(false);
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
        touchPosition tp;
        hidTouchRead(&tp);
        static float touchCd=0;
        if(touchCd>0) touchCd-=dt;
        if((tp.px!=0 || tp.py!=0) && touchCd<=0){
            float wx = camx + (tp.px / (float)BOT_W) * SCR_W;
            float wy = camy + (tp.py / (float)BOT_H) * SCR_H;
            buildHouse(wx, wy);
            touchCd=0.3f;
        }
        static float acc=0; acc+=dt;
        if(acc>2.0f){ acc=0; if(followCount>0) score+= followCount/2 + 1; }
        if(dmgFlash>0) dmgFlash-=dt;
        u32 down=hidKeysDown();
        float gx=WORLD_W/2, gy=WORLD_H-60;
        if((down&KEY_A) && hypotf(gx-px,gy-py)<30){ state=ST_DUNGEON; enterDungeon(); }
    } else if(state==ST_DUNGEON){
        updateMusic(true);
        updateDungeon(dt);
        u32 down=hidKeysDown();
        if(down&KEY_B){ returnToCult(); }
    } else if(state==ST_MENU){
        touchPosition tp;
        hidTouchRead(&tp);
        static float mCd=0;
        if(mCd>0) mCd-=dt;
        if((tp.px!=0 || tp.py!=0) && mCd<=0){
            int row = (tp.py - 20) / 50;
            if(row>=0 && row<4) menuSel = row;
            if(row==0){ state=ST_PLAY; startGame(); }
            else if(row==3){ wantExit=true; }
            mCd=0.3f;
        }
    }
}

int main(void){
    srand((unsigned)time(NULL));
    gfxInitDefault();
    csndInit();
    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
    C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
    C2D_Prepare();
    top = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
    bottom = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);
    sysFont = C2D_FontLoadSystem(CFG_REGION_USA);
    tBuf = C2D_TextBufNew(4096);
    bg = C(0.16f,0.11f,0.08f);
    red = C(0.85f,0.05f,0.1f);
    cream = C(0.95f,0.9f,0.8f);
    black = C(0.05f,0.05f,0.05f);
    white = C(1,1,1);

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
        else { drawMenuTop(); drawBottomMenu(); }
        C3D_FrameEnd(0);
    }

    csndExit();
    C2D_TextBufDelete(tBuf);
    C2D_FontFree(sysFont);
    C2D_Fini();
    C3D_Fini();
    gfxExit();
    return 0;
}
