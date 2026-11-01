#include <3ds.h>
#include <citro2d.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

#define SCR_W 400
#define SCR_H 240
#define MAX_FOOD 32
#define MAX_ENEMIES 16
#define MAX_FOLLOWERS 16

static u32 C(float r,float g,float b){
    return C2D_Color32((int)(r*255),(int)(g*255),(int)(b*255),255);
}

static float px,py,pang,php,pmaxhp;
static float fx[MAX_FOOD],fy[MAX_FOOD]; static bool ftake[MAX_FOOD]; static int fcount;
static float ex[MAX_ENEMIES],ey[MAX_ENEMIES],es[MAX_ENEMIES],ed[MAX_ENEMIES],ecd[MAX_ENEMIES]; static int ecount;
static float ox[MAX_FOLLOWERS],oy[MAX_FOLLOWERS],ot[MAX_FOLLOWERS]; static int ocount;
static int day=1, score=0;
static float timer=90.0f, dmgFlash=0.0f, lastSpawn=0.0f;

static float rnd(float a,float b){ return a + (float)rand()/(float)RAND_MAX*(b-a); }

static void addEnemy(void){
    if(ecount>=MAX_ENEMIES) return;
    int side=rand()%4;
    float x,y;
    if(side==0){x=20; y=rnd(20,SCR_H-20);}
    else if(side==1){x=SCR_W-20; y=rnd(20,SCR_H-20);}
    else if(side==2){x=rnd(20,SCR_W-20); y=20;}
    else{x=rnd(20,SCR_W-20); y=SCR_H-20;}
    ex[ecount]=x; ey[ecount]=y;
    es[ecount]=1.2f+day*0.15f;
    ed[ecount]=6.0f+day*2.0f;
    ecd[ecount]=0;
    ecount++;
}

static void initLevel(void){
    px=SCR_W/2; py=SCR_H/2; php=pmaxhp; pang=0;
    timer=90.0f; score=0;
    ecount=0; ocount=0; fcount=0;
    int n=3+day; if(n>MAX_FOLLOWERS)n=MAX_FOLLOWERS;
    for(int i=0;i<n;i++){ ox[i]=rnd(40,SCR_W-40); oy[i]=rnd(40,SCR_H-40); ot[i]=rnd(0,9); ocount++; }
    int g=5+day*2; if(g>MAX_FOOD)g=MAX_FOOD;
    for(int i=0;i<g;i++){ fx[i]=rnd(30,SCR_W-30); fy[i]=rnd(30,SCR_H-30); ftake[i]=false; fcount++; }
    addEnemy();
}

static void update(float dt){
    u32 k=hidKeysHeld();
    float dx=0,dy=0;
    if(k&KEY_CPAD_UP) dy-=1;
    if(k&KEY_CPAD_DOWN) dy+=1;
    if(k&KEY_CPAD_LEFT) dx-=1;
    if(k&KEY_CPAD_RIGHT) dx+=1;
    float len=sqrtf(dx*dx+dy*dy);
    if(len>0){ dx/=len; dy/=len; pang=atan2f(dy,dx); }
    px+=dx*2.6f*60.f*dt; py+=dy*2.6f*60.f*dt;
    if(px<15)px=15; if(px>SCR_W-15)px=SCR_W-15;
    if(py<15)py=15; if(py>SCR_H-15)py=SCR_H-15;
    for(int i=0;i<fcount;i++){
        if(!ftake[i] && hypotf(fx[i]-px,fy[i]-py)<20){ ftake[i]=true; score++; }
    }
    for(int i=0;i<ocount;i++){ ot[i]+=dt*0.4f; ox[i]+=sinf(ot[i])*0.4f; oy[i]+=cosf(ot[i]*0.9f)*0.4f; }
    for(int i=0;i<ecount;i++){
        float a=atan2f(py-ey[i],px-ex[i]);
        ex[i]+=cosf(a)*es[i]*60.f*dt;
        ey[i]+=sinf(a)*es[i]*60.f*dt;
        if(ecd[i]>0) ecd[i]-=dt;
        if(hypotf(px-ex[i],py-ey[i])<18 && ecd[i]<=0){ ecd[i]=1.0f; php-=ed[i]; dmgFlash=3.0f; if(php<0)php=0; }
    }
    lastSpawn+=dt;
    if(lastSpawn>2.0f){ lastSpawn=0; if(ecount<2+day) addEnemy(); }
    timer-=dt;
    if(timer<=0){ day++; if(day>99)day=99; initLevel(); }
    if(dmgFlash>0) dmgFlash-=dt;
}

static C3D_RenderTarget *top;
static u32 bg, foodC, foodLid, fol1, fol2, ene, lambBase, lambBody, lambHead, lambEye, lambHorn, txt;

static void drawTop(void){
    C2D_SceneBegin(top);
    C2D_DrawRectSolid(0,0,0,SCR_W,SCR_H,bg);
    for(int i=0;i<fcount;i++){
        if(ftake[i]) continue;
        C2D_DrawRectSolid(fx[i]-8,fy[i]-6,0.2f,16,14,foodC);
        C2D_DrawRectSolid(fx[i]-8,fy[i]-6,0.21f,16,5,foodLid);
    }
    for(int i=0;i<ocount;i++){
        C2D_DrawCircleSolid(ox[i],oy[i],0.3f,8,fol1);
        C2D_DrawCircleSolid(ox[i],oy[i]-7,0.29f,4,fol2);
    }
    for(int i=0;i<ecount;i++){
        C2D_DrawRectSolid(ex[i]-8,ey[i]-6,0.4f,16,12,ene);
    }
    C2D_DrawCircleSolid(px,py+6,0.5f,16,lambBase);
    C2D_DrawRectSolid(px-8,py-8,0.4f,16,14,lambBody);
    C2D_DrawCircleSolid(px,py-2,0.5f,10,lambHead);
    C2D_DrawCircleSolid(px-4,py-3,0.6f,2,lambEye);
    C2D_DrawCircleSolid(px+4,py-3,0.6f,2,lambEye);
    C2D_DrawRectSolid(px-10,py-12,0.5f,3,6,lambHorn);
    C2D_DrawRectSolid(px+7,py-12,0.5f,3,6,lambHorn);
    if(dmgFlash>0) C2D_DrawRectSolid(0,0,0.9f,SCR_W,SCR_H,C2D_Color32(255,0,0,60));
    // верхняя строка-счёт рисуем квадратами-индикаторами
    C2D_DrawRectSolid(8,8,0.5f,10,10,C(1,0.9f,0.4f));
    C2D_DrawRectSolid(8,22,0.5f,10,10,C(0.2f,0.9f,0.2f));
    C2D_Flush();
}

static void drawBottom(void){
    // Ничего не рисуем для надёжности (без console/printf)
}

int main(void){
    srand((unsigned)time(NULL));
    pmaxhp=100.0f;

    gfxInitDefault();
    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
    C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
    C2D_Prepare();
    top = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);

    bg     = C(0.16f,0.11f,0.08f);
    foodC  = C(0.54f,0.36f,0.17f);
    foodLid= C(0.42f,0.25f,0.10f);
    fol1   = C(0.86f,0.86f,0.86f);
    fol2   = C(0.60f,0.60f,0.60f);
    ene    = C(0.75f,0.20f,0.30f);
    lambBase= C(1.0f,1.0f,1.0f);
    lambBody= C(0.82f,0.0f,0.13f);
    lambHead= C(0.92f,0.92f,0.92f);
    lambEye = C(0.0f,0.0f,0.0f);
    lambHorn= C(0.60f,0.0f,0.10f);
    txt     = C(1,1,1);

    initLevel();

    while(aptMainLoop()){
        hidScanInput();
        u32 down=hidKeysDown();
        if(down&KEY_START) break;
        if(down&KEY_A){ php=fminf(pmaxhp,php+15); }

        update(1.0f/60.0f);

        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
        drawTop();
        C3D_FrameEnd(0);
        drawBottom();
    }

    C2D_Fini();
    C3D_Fini();
    gfxExit();
    return 0;
}
