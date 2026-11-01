// CULT OF THE LAMB style demo for Nintendo 3DS
// Own game inspired by Cult of the Lamb.
// Uses only basic citro2d primitives. Latin comments only (encoding-safe).

#include <3ds.h>
#include <citro2d.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

#define SCR_W 400
#define SCR_H 240

#define MAX_FOOD 32
#define MAX_FOLLOWERS 16

static u32 C(float r,float g,float b){
    return C2D_Color32((int)(r*255),(int)(g*255),(int)(b*255),255);
}

// game state
static float px,py,pang,php,pmaxhp;
static float fx[MAX_FOOD],fy[MAX_FOOD]; static bool ftake[MAX_FOOD]; static int fcount;
static float ox[MAX_FOLLOWERS],oy[MAX_FOLLOWERS],ot[MAX_FOLLOWERS]; static int ocount;
static int score=0;
static float dmgFlash=0.0f;

enum GameState { ST_MENU, ST_PLAY };
static GameState state = ST_MENU;
static int menuSel = 0;

static float rnd(float a,float b){ return a + (float)rand()/(float)RAND_MAX*(b-a); }

static void startGame(void){
    score=0; fcount=0;
    px=SCR_W/2; py=SCR_H/2; php=pmaxhp; pang=0; dmgFlash=0;
    ocount=0;
    for(int i=0;i<4;i++){ ox[i]=rnd(40,SCR_W-40); oy[i]=rnd(40,SCR_H-40); ot[i]=rnd(0,9); ocount++; }
    for(int i=0;i<8;i++){ fx[i]=rnd(30,SCR_W-30); fy[i]=rnd(30,SCR_H-30); ftake[i]=false; fcount++; }
}

static void update(float dt){
    if(state!=ST_PLAY) return;
    u32 k=hidKeysHeld();
    float dx=0,dy=0;
    if(k&KEY_CPAD_UP) dy-=1;
    if(k&KEY_CPAD_DOWN) dy+=1;
    if(k&KEY_CPAD_LEFT) dx-=1;
    if(k&KEY_CPAD_RIGHT) dx+=1;
    float len=sqrtf(dx*dx+dy*dy);
    if(len>0){ dx/=len; dy/=len; pang=atan2f(dy,dx); }
    px+=dx*2.6f*60.f*dt; py+=dy*2.6f*60.f*dt;
    if(px<15){px=15;} if(px>SCR_W-15){px=SCR_W-15;}
    if(py<15){py=15;} if(py>SCR_H-15){py=SCR_H-15;}
    for(int i=0;i<fcount;i++){
        if(!ftake[i] && hypotf(fx[i]-px,fy[i]-py)<20){ ftake[i]=true; score++; }
    }
    for(int i=0;i<ocount;i++){ ot[i]+=dt*0.4f; ox[i]+=sinf(ot[i])*0.4f; oy[i]+=cosf(ot[i]*0.9f)*0.4f; }
    if(dmgFlash>0) dmgFlash-=dt;
}

static C3D_RenderTarget *top;
static u32 bg, red, cream, selc, foodC, foodLid, fol1, fol2;

// draw lamb from primitives (procedural)
static void drawLamb(float x,float y){
    // light disc base
    C2D_DrawCircleSolid(x,y+6,0.5f,16, C(1,1,1));
    C2D_DrawCircleSolid(x,y+6,0.51f,16, C(0.7f,0.1f,0.1f));
    // cloak body
    C2D_DrawRectSolid(x-10,y-4,0.6f,20,18, red);
    C2D_DrawRectSolid(x-10,y+10,0.59f,20,3, cream);
    // head
    C2D_DrawCircleSolid(x,y-2,0.5f,11, C(0.92f,0.92f,0.92f));
    // eyes
    C2D_DrawCircleSolid(x-4,y-3,0.6f,2, C(0,0,0));
    C2D_DrawCircleSolid(x+4,y-3,0.6f,2, C(0,0,0));
    // horns
    C2D_DrawRectSolid(x-11,y-12,0.5f,3,8, C(0.6f,0,0.1f));
    C2D_DrawRectSolid(x+8,y-12,0.5f,3,8, C(0.6f,0,0.1f));
    // third eye symbol
    C2D_DrawCircleSolid(x,y-9,0.5f,4, C(0.85f,0.1f,0.1f));
}

// draw menu: clean bars + highlighted selection
static void drawMenu(void){
    C2D_SceneBegin(top);
    C2D_DrawRectSolid(0,0,0,SCR_W,SCR_H, bg);
    // logo bar
    C2D_DrawRectSolid(70,20,0.2f,260,22, red);
    C2D_DrawRectSolid(72,22,0.21f,256,18, C(0.95f,0.9f,0.75f));
    // separator
    C2D_DrawRectSolid(70,58,0.2f,260,3, C(0.5f,0.4f,0.3f));
    // 4 menu rows
    int y=76;
    for(int i=0;i<4;i++){
        int cy=y + i*36;
        // highlight selection
        if(i==menuSel){
            C2D_DrawRectSolid(70,cy-8,0.3f,260,26, C(0.55f,0.1f,0.15f));
            C2D_DrawRectSolid(76,cy-3,0.31f,12,18, selc); // indicator
        }
        // text placeholder bars (cleaner now)
        u32 col = (i==menuSel) ? selc : cream;
        C2D_DrawRectSolid(100,cy,0.4f,22,8, col);
        C2D_DrawRectSolid(128,cy,0.4f,10,8, col);
        C2D_DrawRectSolid(144,cy,0.4f,30,8, col);
    }
    // bottom hint
    C2D_DrawRectSolid(70,230,0.2f,260,2, C(0.5f,0.4f,0.3f));
}

// draw gameplay screen
static void drawGame(void){
    C2D_SceneBegin(top);
    C2D_DrawRectSolid(0,0,0,SCR_W,SCR_H, bg);
    for(int i=0;i<SCR_W;i+=40) C2D_DrawRectSolid(i,0,0.1f,1,SCR_H, C(0,0,0));
    for(int j=0;j<SCR_H;j+=40) C2D_DrawRectSolid(0,j,0.1f,SCR_W,1, C(0,0,0));
    // food crates
    for(int i=0;i<fcount;i++){
        if(ftake[i]) continue;
        C2D_DrawRectSolid(fx[i]-8,fy[i]-6,0.2f,16,14, foodC);
        C2D_DrawRectSolid(fx[i]-8,fy[i]-6,0.21f,16,5,  foodLid);
    }
    // followers
    for(int i=0;i<ocount;i++){
        C2D_DrawCircleSolid(ox[i],oy[i],0.3f,8, fol1);
        C2D_DrawCircleSolid(ox[i],oy[i]-7,0.29f,4, fol2);
    }
    // player
    drawLamb(px,py);
    // score indicator top-left
    C2D_DrawRectSolid(8,8,0.5f,30,8, C(1,0.9f,0.4f));
    C2D_DrawRectSolid(8,20,0.5f,20,6, C(0.9f,0.6f,0.2f));
    if(dmgFlash>0) C2D_DrawRectSolid(0,0,0.9f,SCR_W,SCR_H, C2D_Color32(255,0,0,60));
}

int main(void){
    srand((unsigned)time(NULL));
    pmaxhp=100.0f;

    gfxInitDefault();
    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
    C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
    C2D_Prepare();
    top = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);

    bg = C(0.16f,0.11f,0.08f);
    red = C(0.82f,0.0f,0.13f);
    cream = C(0.95f,0.9f,0.8f);
    selc = C(1.0f,0.85f,0.3f);
    foodC = C(0.54f,0.36f,0.17f);
    foodLid = C(0.42f,0.25f,0.10f);
    fol1 = C(0.86f,0.86f,0.86f);
    fol2 = C(0.60f,0.60f,0.60f);

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
        } else {
            if(down&KEY_B){ state=ST_MENU; menuSel=0; }
        }

        update(1.0f/60.0f);

        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
        if(state==ST_PLAY) drawGame(); else drawMenu();
        C3D_FrameEnd(0);
    }

    C2D_Fini();
    C3D_Fini();
    gfxExit();
    return 0;
}