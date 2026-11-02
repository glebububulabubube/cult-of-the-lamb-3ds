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
#define MAX_BUILD 16
#define MAX_FOLLOW 24
#define ALTAR_R 14

static u32 C(float r,float g,float b){
    return C2D_Color32((int)(r*255),(int)(g*255),(int)(b*255),255);
}

static float px,py,php,pmaxhp=100.0f;
static float bx[MAX_BUILD],by[MAX_BUILD]; static int buildCount;
static float fwX[MAX_FOLLOW],fwY[MAX_FOLLOW]; static int followCount;
static char fwName[MAX_FOLLOW][24];
static float altarX,altarY;
static int score;
static float dmgFlash;
static bool wantExit=false;

enum GameState { ST_MENU, ST_PLAY };
static GameState state = ST_MENU;
static int menuSel = 0;

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
    C2D_DrawText(&txt, C2D_AlignLeft, x, y, 0.5f, scale, scale, C(r,g,b));
}

static void addFollower(float x,float y,const char* nm){
    if(followCount>=MAX_FOLLOW) return;
    fwX[followCount]=x; fwY[followCount]=y;
    snprintf(fwName[followCount],24,"%s",nm);
    followCount++;
}
static void addRandomFollower(float x,float y){
    addFollower(x,y, names[rand()%nameCount]);
}

static void startGame(void){
    score=10; buildCount=0; followCount=0;
    px=SCR_W/2+30; py=SCR_H/2; php=pmaxhp; dmgFlash=0;
    altarX=SCR_W/2; altarY=SCR_H/2;
    addRandomFollower(altarX-30, altarY+20);
    addRandomFollower(altarX+30, altarY+10);
    addRandomFollower(altarX,     altarY+28);
}

static void buildHouse(float x,float y){
    if(buildCount>=MAX_BUILD) return;
    if(score<5) return;
    bx[buildCount]=x; by[buildCount]=y; buildCount++;
    score-=5;
    addRandomFollower(x+rnd(-15,15), y+20);

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
        touchPosition tp;
        hidTouchRead(&tp);
        static float touchCd=0;
        if(touchCd>0) touchCd-=dt;
        if((tp.px!=0 || tp.py!=0) && touchCd<=0){
            float gx = (tp.px / (float)BOT_W) * SCR_W;
            float gy = (tp.py / (float)BOT_H) * SCR_H;
            buildHouse(gx, gy);
            touchCd=0.3f;
        }
        static float acc=0; acc+=dt;
        if(acc>2.0f){ acc=0; if(followCount>0) score+= followCount/2 + 1; }
        if(dmgFlash>0) dmgFlash-=dt;
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

static void drawAltar(void){
    C2D_DrawCircleSolid(altarX,altarY,0.2f,ALTAR_R, C(0.45f,0.1f,0.1f));
    C2D_DrawCircleSolid(altarX,altarY,0.21f,ALTAR_R-2, C(0.2f,0.05f,0.05f));
    for(int i=0;i<5;i++){
        float a0=i*2.0f*3.14159f/5.0f - 3.14159f/2.0f;
        float a1=(i+2)*2.0f*3.14159f/5.0f - 3.14159f/2.0f;
        C2D_DrawLine(altarX+cos(a0)*ALTAR_R, altarY+sin(a0)*ALTAR_R, C(0.9f,0.3f,0.3f),
                     altarX+cos(a1)*ALTAR_R, altarY+sin(a1)*ALTAR_R, C(0.9f,0.3f,0.3f), 1.0f, 0.5f);
    }
    C2D_DrawCircleSolid(altarX,altarY,0.3f,5, C(1.0f,0.8f,0.2f));
    int n=followCount>10?10:followCount;
    for(int i=0;i<n;i++){
        float ang=(i/(float)10.0f)*2.0f*3.14159f;
        float kx=altarX+cos(ang)*(ALTAR_R+8);
        float ky=altarY+sin(ang)*(ALTAR_R+5);
        C2D_DrawRectSolid(kx-4,ky+2,0.3f,8,9, C(0.75f,0.1f,0.1f));
        C2D_DrawCircleSolid(kx,ky,0.3f,6, C(0.35f,0.6f,0.25f));
        C2D_DrawRectSolid(kx-5,ky-2,0.32f,10,3, C(0.95f,0.95f,0.95f));
        C2D_DrawRectSolid(kx-5,ky+1,0.33f,10,2, C(0.95f,0.95f,0.95f));
        C2D_DrawTriangle(kx, ky+1, C(0.9f,0.1f,0.1f), kx-2, ky-1, C(0.9f,0.1f,0.1f), kx+2, ky-1, C(0.9f,0.1f,0.1f), 0.35f);
        drawText(kx-14, ky-12, 0.5f, fwName[i], 1,1,0.6f);
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

static void drawGame(void){
    C2D_TargetClear(top, bg);
    C2D_SceneBegin(top);
    C2D_DrawRectSolid(0,0,0,SCR_W,SCR_H, bg);
    for(int i=0;i<SCR_W;i+=40) C2D_DrawRectSolid(i,0,0.1f,1,SCR_H, C(0,0,0));
    for(int j=0;j<SCR_H;j+=40) C2D_DrawRectSolid(0,j,0.1f,SCR_W,1, C(0,0,0));
    for(int i=0;i<8;i++){ float cx=40+i*45, cy=195;
        C2D_DrawRectSolid(cx-6,cy-5,0.2f,12,10, C(0.6f,0.4f,0.18f));
        C2D_DrawRectSolid(cx-6,cy-5,0.21f,12,4, C(0.45f,0.3f,0.12f));
    }
    drawAltar();
    drawHouses();
    drawLambfull(px,py,1.0f);
    if(dmgFlash>0) C2D_DrawRectSolid(0,0,0.9f,SCR_W,SCR_H, C2D_Color32(255,0,0,50));
    C2D_DrawRectSolid(6,6,0.5f,54,10, C(0.2f,0.15f,0.1f));
    C2D_DrawRectSolid(8,8,0.5f, (score>50?50:score),8, C(1,0.85f,0.3f));
    C2D_DrawRectSolid(6,20,0.5f,54,8, C(0.2f,0.15f,0.1f));
    C2D_DrawRectSolid(8,22,0.5f, (followCount>50?50:followCount*2),6, C(0.7f,0.8f,0.9f));
    char h[24]; snprintf(h,24,"Food:%d Cult:%d",score,followCount);
    drawText(70,4,0.5f,h,1,0.85f,0.3f);
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
    drawLambfull(cx, cy-R-18, 1.4f);
    drawText(110, 12, 0.9f, "CULT OF THE LAMB", 0.9f,0.1f,0.1f);
}

static void drawBottomMenu(void){
    C2D_TargetClear(bottom, black);
    C2D_SceneBegin(bottom);
    C2D_DrawRectSolid(0,0,0,BOT_W,BOT_H, black);
    drawText(60,6,0.6f,"CULT OF THE LAMB",1,0.2f,0.2f);
    const char* items[4]={"PLAY","SETTINGS","CREDITS","EXIT"};
    for(int i=0;i<4;i++){
        int y=45+i*50;
        if(i==menuSel){
            C2D_DrawRectSolid(30,y-4,0.3f,260,30, C(0.55f,0.1f,0.15f));
        } else {
            C2D_DrawRectSolid(30,y-4,0.3f,260,30, C(0.18f,0.12f,0.1f));
        }
        u32 col = (i==menuSel)?C(1,0.85f,0.3f):C(0.9f,0.9f,0.9f);
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
    drawText(110,100,0.5f,"TAP TO BUILD",1,0.9f,0.4f);
    drawText(60,175,0.5f,"Food bar",1,0.85f,0.3f);
}

int main(void){
    srand((unsigned)time(NULL));
    gfxInitDefault();
    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
    C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
    C2D_Prepare();
    top = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
    bottom = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);
    sysFont = C2D_FontLoadSystem();
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
        if(state==ST_PLAY && (down&KEY_B)){ state=ST_MENU; }

        update(1.0f/60.0f);

        C2D_TextBufClear(tBuf);
        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
        if(state==ST_PLAY){ drawGame(); drawBottomGame(); }
        else { drawMenuTop(); drawBottomMenu(); }
        C3D_FrameEnd(0);
    }

    C2D_TextBufDelete(tBuf);
    C2D_FontUnload(sysFont);
    C2D_Fini();
    C3D_Fini();
    gfxExit();
    return 0;
}
