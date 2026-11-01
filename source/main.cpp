/*
 * КУЛЬТ ЯГНЁНКА — демейк для Nintendo 3DS
 * Собственная игра в духе Cult of the Lamb.
 * Используются только базовые примитивы citro2d (Rect/Circle) — без текстур-спрайтов,
 * чтобы код был максимально надёжным и компилировался без ошибок API.
 */

#include <3ds.h>
#include <citro2d.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define SCR_W 400
#define SCR_H 240

#define MAX_FOOD 32
#define MAX_ENEMIES 16
#define MAX_FOLLOWERS 16

static u32 RGBAf(float r,float g,float b,float a){
    return C2D_Color32((int)(r*255),(int)(g*255),(int)(b*255),(int)(a*255));
}

// ---- состояние ----
static float px,py,pang,php,pmaxhp =100.0f;
static float fx[MAX_FOOD],fy[MAX_FOOD]; static bool ftake[MAX_FOOD]; static int fcount;
static float ex[MAX_ENEMIES],ey[MAX_ENEMIES],es[MAX_ENEMIES],ed[MAX_ENEMIES],ecd[MAX_ENEMIES]; static int ecount;
static float ox[MAX_FOLLOWERS],oy[MAX_FOLLOWERS],ot[MAX_FOLLOWERS]; static int ocount;
static int day=1, score=0;
static float timer=90.0f, dmgFlash=0.0f, lastSpawn=0.0f;
static bool running=true;

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
    px=SCR_W/2; py=SCR_H/2;
    php=pmaxhp; pang=0;
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
    if(k&KEY_CPAD_UP)   dy-=1;
    if(k&KEY_CPAD_DOWN) dy+=1;
    if(k&KEY_CPAD_LEFT) dx-=1;
    if(k&KEY_CPAD_RIGHT)dx+=1;
    float len=sqrtf(dx*dx+dy*dy);
    if(len>0){ dx/=len; dy/=len; pang=atan2f(dy,dx); }
    px+=dx*2.6f*60.f*dt; py+=dy*2.6f*60.f*dt;
    if(px<15)px=15; if(px>SCR_W-15)px=SCR_W-15;
    if(py<15)py=15; if(py>SCR_H-15)py=SCR_H-15;

    for(int i=0;i<fcount;i++){
        if(!ftake[i] && hypotf(fx[i]-px,fy[i]-py)<20){
            ftake[i]=true; score++;
            if(score%5==0 && php<pmaxhp) php=fminf(pmaxhp,php+10);
        }
    }
    for(int i=0;i<ocount;i++){
        ot[i]+=dt*0.4f;
        ox[i]+=sinf(ot[i])*0.4f;
        oy[i]+=cosf(ot[i]*0.9f)*0.4f;
    }
    for(int i=0;i<ecount;i++){
        float a=atan2f(py-ey[i],px-ex[i]);
        ex[i]+=cosf(a)*es[i]*60.f*dt;
        ey[i]+=sinf(a)*es[i]*60.f*dt;
        if(ecd[i]>0) ecd[i]-=dt;
        if(hypotf(px-ex[i],py-ey[i])<18 && ecd[i]<=0){
            ecd[i]=1.0f; php-=ed[i]; dmgFlash=3.0f;
            if(php<0)php=0;
        }
    }
    lastSpawn+=dt;
    if(lastSpawn>2.0f){ lastSpawn=0; if(ecount<2+day) addEnemy(); }
    timer-=dt;
    if(timer<=0){ day++; if(day>99)day=99; initLevel(); }
    if(dmgFlash>0) dmgFlash-=dt;
}

// рисование ягнёнка примитивами (процедурно, в стиле культовой игрушки)
static void drawLamb(float x,float y){
    // светлая подложка
    C2D_DrawCircleSolid(x,y+6,0.5f,16, RGBAf(1,1,1,0.85f));
    C2D_DrawCircleSolid(x,y+6,0.51f,16, RGBAf(0.7f,0.1f,0.1f,1.0f));
    // плащ (треугольник)
    C2D_DrawTriangle(x, y-4,0.6f, x-16, y+16,0.1f, x+16, y+16,0.1f, RGBAf(0.82f,0,0.13f,1));
    // белая кайма
    C2D_DrawTriangle(x, y+12,0.59f, x-15, y+16,0.1f, x+15, y+16,0.1f, RGBAf(0.9f,0.9f,0.9f,1));
    // голова
    C2D_DrawCircleSolid(x, y-6,0.5f,15, RGBAf(0.7f,0.7f,0.7f,1));
    C2D_DrawCircleSolid(x, y-6,0.49f,11, RGBAf(0.92f,0.92f,0.92f,1));
    // глаза
    C2D_DrawCircleSolid(x-6, y-7,0.55f,3, RGBAf(0,0,0,1));
    C2D_DrawCircleSolid(x+6, y-7,0.55f,3, RGBAf(0,0,0,1));
    // рога
    C2D_DrawRectSolid(x-14,y-18,0.5f,4,9, RGBAf(0.6f,0,0.1f,1));
    C2D_DrawRectSolid(x+10,y-18,0.5f,4,9, RGBAf(0.6f,0,0.1f,1));
    // третий глаз-символ
    C2D_DrawCircleSolid(x, y-20,0.5f,4, RGBAf(0.85f,0.1f,0.1f,1));
}

static void drawTop(void){
    C2D_Target *top = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
    C2D_SceneBegin(top);
    // фон
    C2D_DrawRectSolid(0,0,0,SCR_W,SCR_H, RGBAf(0.16f,0.11f,0.08f,1.0f));
    for(int i=0;i<SCR_W;i+=40) C2D_DrawRectSolid(i,0,0.1f,1,SCR_H, RGBAf(0,0,0,0.25f));
    for(int j=0;j<SCR_H;j+=40) C2D_DrawRectSolid(0,j,0.1f,SCR_W,1, RGBAf(0,0,0,0.25f));
    // еда
    for(int i=0;i<fcount;i++){
        if(ftake[i]) continue;
        C2D_DrawRectSolid(fx[i]-8,fy[i]-6,0.2f,16,14, RGBAf(0.54f,0.36f,0.17f,1));
        C2D_DrawRectSolid(fx[i]-8,fy[i]-6,0.21f,16,5,  RGBAf(0.42f,0.25f,0.1f,1));
    }
    // последователи
    for(int i=0;i<ocount;i++){
        C2D_DrawCircleSolid(ox[i],oy[i],0.3f,8, RGBAf(0.86f,0.86f,0.86f,1));
        C2D_DrawCircleSolid(ox[i],oy[i]-7,0.29f,4, RGBAf(0.6f,0.6f,0.6f,1));
    }
    // враги
    for(int i=0;i<ecount;i++){
        C2D_DrawTriangle(ex[i]+10,ey[i],0.4f, ex[i]-6,ey[i]-7,0.4f, ex[i]-6,ey[i]+7,0.4f, RGBAf(0.75f,0.2f,0.3f,1));
    }
    // игрок
    drawLamb(px,py);
    if(dmgFlash>0) C2D_DrawRectSolid(0,0,0.9f,SCR_W,SCR_H, RGBAf(1,0,0,0.2f));
    C2D_Flush();
}

static void drawBottom(void){
    // нижний экран — текст
    consoleSelect(NULL);
    printf("\x1b[1;8H КУЛЬТ ЯГНЁНКА");
    printf("\x1b[3;1H Последователи: %d", ocount);
    printf("\x1b[4;1H Еда:           %d", score);
    printf("\x1b[5;1H Здоровье:      %d/%d", (int)php,(int)pmaxhp);
    printf("\x1b[6;1H День: %d   Время: %d", day,(int)timer);
    printf("\x1b[8;1H Круг: движение");
    printf("\x1b[9;1H A: молитва (лечит)");
    printf("\x1b[10;1H START: выход");
}

int main(void){
    srand((unsigned)time(NULL));
    gfxInitDefault();
    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
    C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
    C2D_Prepare();
    consoleInit(GFX_BOTTOM, NULL);

    initLevel();

    while(aptMainLoop()){
        hidScanInput();
        u32 down=hidKeysDown();
        u32 held=hidKeysHeld();
        if(down&KEY_START) break;
        if(down&KEY_A){ php=fminf(pmaxhp,php+15); }
        if(down&KEY_B){ php=fminf(pmaxhp,php+5); }
        if(held&KEY_SELECT){ running=false; }

        float dt=1.0f/60.0f;
        update(dt);
        drawTop();
        drawBottom();

        C2D_Flush();
        gspWaitForEvent(GSPGPU_EVENT_VBlank0, false);
        C2D_SwapBuffers();
    }

    C2D_Fini();
    C3D_Fini();
    gfxExit();
    return 0;
}