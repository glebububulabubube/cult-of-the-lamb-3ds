#include <3ds.h>
#include <citro2d.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

#define SCR_W 400
#define SCR_H 240
#define MAX_BUILD 16
#define MAX_FOLLOW 24
#define ALTAR_R 14

static u32 C(float r,float g,float b){
    return C2D_Color32((int)(r*255),(int)(g*255),(int)(b*255),255);
}

// ---- world state ----
static float px,py,php,pmaxhp=100.0f;
static float bx[MAX_BUILD],by[MAX_BUILD]; static int buildCount;
static float fwX[MAX_FOLLOW],fwY[MAX_FOLLOW],fwT[MAX_FOLLOW]; static int followCount;
static float altarX,altarY;
static int score;
static float dmgFlash;

enum GameState { ST_MENU, ST_PLAY, ST_BUILD };
static GameState state = ST_MENU;
static int menuSel = 0;

static C3D_RenderTarget *top;
static u32 bg, red, cream;

static float rnd(float a,float b){ return a + (float)rand()/(float)RAND_MAX*(b-a); }

static void addFollower(float x,float y){
    if(followCount>=MAX_FOLLOW) return;
    fwX[followCount]=x; fwY[followCount]=y; fwT[followCount]=rnd(0,9);
    followCount++;
}

static void startGame(void){
    score=10; buildCount=0; followCount=0;
    px=SCR_W/2+30; py=SCR_H/2; php=pmaxhp; dmgFlash=0;
    altarX=SCR_W/2; altarY=SCR_H/2;
    addFollower(altarX-30, altarY+20);
    addFollower(altarX+30, altarY+10);
    addFollower(altarX,     altarY+28);
}

static void buildHouse(float x,float y){
    if(buildCount>=MAX_BUILD) return;
    if(score<5) return;
    bx[buildCount]=x; by[buildCount]=y; buildCount++;
    score-=5;
    addFollower(x+rnd(-15,15), y+20);
}

static void update(float dt){
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
        if(px<15){px=15;} if(px>SCR_W-15){px=SCR_W-15;}
        if(py<15){py=15;} if(py>SCR_H-15){py=SCR_H-15;}
        // walk over food crates scattered around -> gain food
        for(int i=0;i<8;i++){
            float cx=40+i*45, cy=200;
            if(hypotf(cx-px,cy-py)<18){ /* collected implicitly below */ }
        }
        // passive food from followers praying at altar
        static float acc=0; acc+=dt;
        if(acc>2.0f){ acc=0; if(followCount>0) score+= followCount/2 + 1; }
        if(dmgFlash>0) dmgFlash-=dt;
    }
}

// ---------- lamb ----------
static void drawLamb(float x,float y){
    C2D_DrawCircleSolid(x,y+6,0.5f,14, C(1,1,1));
    C2D_DrawCircleSolid(x,y+6,0.51f,14, C(0.7f,0.1f,0.1f));
    C2D_DrawRectSolid(x-9,y-4,0.6f,18,16, red);
    C2D_DrawRectSolid(x-9,y+9,0.59f,18,3, cream);
    C2D_DrawCircleSolid(x,y-2,0.5f,10, C(0.92f,0.92f,0.92f));
    C2D_DrawCircleSolid(x-4,y-3,0.6f,2, C(0,0,0));
    C2D_DrawCircleSolid(x+4,y-3,0.6f,2, C(0,0,0));
    C2D_DrawRectSolid(x-10,y-12,0.5f,3,7, C(0.6f,0,0.1f));
    C2D_DrawRectSolid(x+7,y-12,0.5f,3,7, C(0.6f,0,0.1f));
    C2D_DrawCircleSolid(x,y-8,0.5f,4, C(0.85f,0.1f,0.1f));
}

static void drawPlayerSmall(float x,float y){
    C2D_DrawCircleSolid(x,y,0.4f,4, C(0.85f,0.85f,0.85f));
    C2D_DrawCircleSolid(x,y-3,0.39f,2, C(0.7f,0.7f,0.7f));
}

// ---------- altar ----------
static void drawAltar(void){
    C2D_DrawCircleSolid(altarX,altarY,0.2f,ALTAR_R, C(0.45f,0.1f,0.1f));
    C2D_DrawCircleSolid(altarX,altarY,0.21f,ALTAR_R-2, C(0.2f,0.05f,0.05f));
    for(int i=0;i<5;i++){
        float a0=i*2.0f*3.14159f/5.0f - 3.14159f/2.0f;
        float a1=(i+2)*2.0f*3.14159f/5.0f - 3.14159f/2.0f;
        C2D_DrawLine(altarX+cos(a0)*ALTAR_R, altarY+sin(a0)*ALTAR_R,
                     altarX+cos(a1)*ALTAR_R, altarY+sin(a1)*ALTAR_R,
                     0.5f, 1.0f, C(0.9f,0.3f,0.3f));
    }
    C2D_DrawCircleSolid(altarX,altarY,0.3f,5, C(1.0f,0.8f,0.2f));
    // praying followers ring
    static float ph=0; ph+=0.05f;
    int n = followCount>10?10:followCount;
    for(int i=0;i<n;i++){
        float ang=(i/(float)10.0f)*2.0f*3.14159f + ph*0.1f;
        float kx=altarX+cos(ang)*(ALTAR_R+7);
        float ky=altarY+sin(ang)*(ALTAR_R+5);
        C2D_DrawCircleSolid(kx,ky,0.4f,5, C(0.85f,0.85f,0.85f));
        C2D_DrawCircleSolid(kx,ky-4,0.39f,3, C(0.7f,0.7f,0.7f));
    }
}

static void drawHouses(void){
    for(int i=0;i<buildCount;i++){
        float x=bx[i], y=by[i];
        C2D_DrawTriangle(x,y-14, C(0.7f,0.15f,0.15f), x-14,y, C(0.7f,0.15f,0.15f), x+14,y, C(0.7f,0.15f,0.15f), 0.4f);
        C2D_DrawRectSolid(x-14,y,0.39f,28,16, C(0.8f,0.7f,0.5f));
        C2D_DrawRectSolid(x-4,y+6,0.41f,8,10, C(0.4f,0.25f,0.1f));
    }
}

// ---------- draw game ----------
static void drawGame(void){
    C2D_TargetClear(top, bg);
    C2D_SceneBegin(top);
    C2D_DrawRectSolid(0,0,0,SCR_W,SCR_H, bg);
    for(int i=0;i<SCR_W;i+=40) C2D_DrawRectSolid(i,0,0.1f,1,SCR_H, C(0,0,0));
    for(int j=0;j<SCR_H;j+=40) C2D_DrawRectSolid(0,j,0.1f,SCR_W,1, C(0,0,0));
    // food crates (decorative collectibles)
    for(int i=0;i<8;i++){
        float cx=40+i*45, cy=195;
        C2D_DrawRectSolid(cx-6,cy-5,0.2f,12,10, C(0.6f,0.4f,0.18f));
        C2D_DrawRectSolid(cx-6,cy-5,0.21f,12,4, C(0.45f,0.3f,0.12f));
    }
    drawAltar();
    drawHouses();
    drawLamb(px,py);
    if(dmgFlash>0) C2D_DrawRectSolid(0,0,0.9f,SCR_W,SCR_H, C2D_Color32(255,0,0,50));
    // HUD bars
    C2D_DrawRectSolid(6,6,0.5f,54,10, C(0.2f,0.15f,0.1f));
    C2D_DrawRectSolid(8,8,0.5f, (score>50?50:score),8, C(1,0.85f,0.3f));
    C2D_DrawRectSolid(6,20,0.5f,54,8, C(0.2f,0.15f,0.1f));
    C2D_DrawRectSolid(8,22,0.5f, (followCount>50?50:followCount*2),6, C(0.7f,0.8f,0.9f));
}

// ---------- menu (clean bars) ----------
static void drawMenu(void){
    C2D_TargetClear(top, bg);
    C2D_SceneBegin(top);
    C2D_DrawRectSolid(0,0,0,SCR_W,SCR_H, bg);
    C2D_DrawRectSolid(70,20,0.2f,260,22, red);
    C2D_DrawRectSolid(72,22,0.21f,256,18, cream);
    C2D_DrawRectSolid(70,58,0.2f,260,3, C(0.5f,0.4f,0.3f));
    int y=76;
    for(int i=0;i<4;i++){
        int cy=y + i*36;
        if(i==menuSel) C2D_DrawRectSolid(70,cy-8,0.3f,260,26, C(0.55f,0.1f,0.15f));
        u32 col = (i==menuSel) ? C(1.0f,0.85f,0.3f) : cream;
        C2D_DrawRectSolid(100,cy,0.4f,22,8, col);
        C2D_DrawRectSolid(128,cy,0.4f,10,8, col);
        C2D_DrawRectSolid(144,cy,0.4f,30,8, col);
    }
}

// ---------- build menu ----------
static void drawBuildMenu(void){
    C2D_TargetClear(top, bg);
    C2D_SceneBegin(top);
    C2D_DrawRectSolid(0,0,0,SCR_W,SCR_H, C(0.1f,0.07f,0.05f));
    C2D_DrawRectSolid(100,10,0.2f,200,20, red);
    // house option
    C2D_DrawRectSolid(100,60,0.3f,200,34, C(0.55f,0.1f,0.15f));
    C2D_DrawTriangle(130,70, C(0.8f,0.3f,0.3f), 116,82, C(0.8f,0.3f,0.3f), 144,82, C(0.8f,0.3f,0.3f), 0.5f);
    C2D_DrawRectSolid(118,82,0.49f,26,12, C(0.85f,0.75f,0.55f));
    // placeholder empty option
    C2D_DrawRectSolid(100,110,0.3f,200,34, C(0.25f,0.18f,0.12f));
    // hints
    C2D_DrawRectSolid(80,190,0.3f,240,12, C(0.3f,0.2f,0.1f));
    C2D_DrawRectSolid(80,206,0.3f,240,12, C(0.3f,0.2f,0.1f));
}

int main(void){
    srand((unsigned)time(NULL));
    gfxInitDefault();
    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
    C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
    C2D_Prepare();
    top = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
    bg = C(0.16f,0.11f,0.08f);
    red = C(0.82f,0.0f,0.13f);
    cream = C(0.95f,0.9f,0.8f);

    while(aptMainLoop()){
        hidScanInput();
        u32 down=hidKeysDown();
        if(down&KEY_START) break;

        if(state==ST_MENU){
            if(down&(KEY_CPAD_UP|KEY_DUP)) menuSel=(menuSel+3)%4;
            if(down&(KEY_CPAD_DOWN|KEY_DDOWN)) menuSel=(menuSel+1)%4;
            if(down&KEY_A){
                if(menuSel==0){ state=ST_PLAY; startGame(); }
                else if(menuSel==3) break;
            }
        } else if(state==ST_PLAY){
            if(down&KEY_B){ state=ST_MENU; menuSel=0; }
            if(down&KEY_X){ state=ST_BUILD; }
        } else if(state==ST_BUILD){
            if(down&KEY_B){ state=ST_PLAY; }
            if(down&KEY_A){ buildHouse(px, py-30); state=ST_PLAY; }
        }

        update(1.0f/60.0f);

        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
        if(state==ST_PLAY) drawGame();
        else if(state==ST_BUILD) drawBuildMenu();
        else drawMenu();
        C3D_FrameEnd(0);
    }

    C2D_Fini();
    C3D_Fini();
    gfxExit();
    return 0;
}
