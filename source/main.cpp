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
#define MAX_ENEMY 6
#define DUNGEON_ROOMS 5
#define MAX_RES 24
#define SV "sdmc:/cultlamb_save.bin"
static u32 C(float r,float g,float b){ return C2D_Color32((int)(r*255),(int)(g*255),(int)(b*255),255); }
enum GS{ST_MENU,ST_HUB,ST_PLAY,ST_DUN,ST_TALK,ST_TMP,ST_OPT};
static GS state=ST_MENU;
static int menuSel=0;
static float tclock;
static bool wantExit=false;
static C3D_RenderTarget *top,*bottom;
static C2D_Font fnt;
static C2D_TextBuf tbuf;
static u32 bg,red,cream,black,white,grey,yellow;
static float px,py,php,pmaxhp=100.0f,camx,camy;
static float p2x,p2y; static bool p2Active=false;
static int coopOn=0;
static int n3dsOn=0;
static int optSel=0;
static u32 lastKeys=0;
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
// resources: type 0=tree(sapling->grows),1=rock ; x,y world ; growth 0..1
static float rx[MAX_RES],ry[MAX_RES],rg[MAX_RES];static int rt[MAX_RES],rCnt=0;
static int rWood=0,rStone=0;
static int craftOpen=0,craftSel=0;
// craftable: type in build-term, cost wood, cost stone
static const int craftType[5]={0,1,2,3,4};
static const int craftWood[5]={20,12,30,5,50};
static const int craftStone[5]={0,8,0,5,20};
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
static void addF(float x,float y,const char* n){
    if(followCount>=MAX_FOLLOW)return;
    fwX[followCount]=x;fwY[followCount]=y;fwT[followCount]=rnd(0,6.28f);fwM[followCount]=50;fwWork[followCount]=0;
    snprintf(fwName[followCount],16,"%s",n);followCount++;
}
static void addR(float x,float y){ addF(x,y,nmAll[rand()%nmC]); }
static void spawnResources(void){
    rCnt=0;rWood=0;rStone=0;
    // trees scattered around world (more on N3DS mode)
    int nTree = n3dsOn?16:10;
    int nRock = n3dsOn?14:8;
    for(int i=0;i<nTree&&rCnt<MAX_RES;i++){
        float x=80+(float)(rand()%(WORLD_W-160));
        float y=80+(float)(rand()%(WORLD_H-160));
        // keep away from altar center
        if(hypotf(x-altarX,y-altarY)<90)continue;
        rx[rCnt]=x;ry[rCnt]=y;rg[rCnt]=rnd(0.3f,1.0f);rt[rCnt]=0;rCnt++;
    }
    // rocks
    for(int i=0;i<nRock&&rCnt<MAX_RES;i++){
        float x=80+(float)(rand()%(WORLD_W-160));
        float y=80+(float)(rand()%(WORLD_H-160));
        if(hypotf(x-altarX,y-altarY)<90)continue;
        rx[rCnt]=x;ry[rCnt]=y;rg[rCnt]=1.0f;rt[rCnt]=1;rCnt++;
    }
}
static void updateRes(float dt){
    for(int i=0;i<rCnt;i++){
        if(rt[i]==0 && rg[i]<1.0f) rg[i]+=dt*0.02f; // trees grow slowly
    }
    // rocks slowly respawn over time
    static float rockTimer=0;
    rockTimer+=dt;
    if(rockTimer>15.0f && rCnt<MAX_RES){
        rockTimer=0;
        // find a clear random spot
        for(int tries=0;tries<10;tries++){
            float x=80+(float)(rand()%(WORLD_W-160));
            float y=80+(float)(rand()%(WORLD_H-160));
            if(hypotf(x-altarX,y-altarY)<90)continue;
            if(hypotf(x-px,y-py)<60)continue;
            rx[rCnt]=x;ry[rCnt]=y;rg[rCnt]=1.0f;rt[rCnt]=1;rCnt++;
            break;
        }
    }
}
static void chopNear(float x,float y){
    for(int i=0;i<rCnt;i++){
        if(rt[i]==0 && hypotf(rx[i]-x,ry[i]-y)<26){
            if(rg[i]>0.5f){ rWood++; } // full tree gives wood
            rg[i]=0.05f;               // reset to sapling, it regrows
            return;
        }
    }
}
static void chipRock(float x,float y){
    for(int i=0;i<rCnt;i++){
        if(rt[i]==1 && hypotf(rx[i]-x,ry[i]-y)<26){
            rStone++;
            // remove this resource entirely
            for(int j=i;j<rCnt-1;j++){ rx[j]=rx[j+1];ry[j]=ry[j+1];rg[j]=rg[j+1];rt[j]=rt[j+1]; }
            rCnt--;
            return;
        }
    }
}
static void drawRes(float ox,float oy){
    for(int i=0;i<rCnt;i++){
        float x=rx[i]-ox,y=ry[i]-oy;if(x<-40||x>SCR_W+40||y<-40||y>SCR_H+40)continue;
        if(rt[i]==0){
            // tree in platformer style: trunk + big leafy crown, 3 growth stages
            float tr=rnd(10,14)*rg[i];          // trunk height grows
            if(tr<3)tr=3;
            // trunk
            C2D_DrawRectSolid(x-3,y-tr,0.4f,6,tr,C(0.45f,0.3f,0.12f));
            // crown
            float cr=12.0f*rg[i]+2.0f;
            if(rg[i]>0.35f){ // young tree small crown
                C2D_DrawCircleSolid(x,y-tr-6,0.41f,cr,C(0.15f,0.5f,0.15f));
                C2D_DrawCircleSolid(x-4,y-tr-3,0.42f,cr*0.6f,C(0.2f,0.6f,0.2f));
                C2D_DrawCircleSolid(x+4,y-tr-3,0.42f,cr*0.6f,C(0.2f,0.6f,0.2f));
            }
            if(rg[i]>0.7f){ // mature: bigger crown, extra top
                cr=15.0f*rg[i]+2.0f;
                C2D_DrawCircleSolid(x,y-tr-8,0.41f,cr,C(0.12f,0.55f,0.12f));
                C2D_DrawCircleSolid(x-6,y-tr-4,0.42f,cr*0.6f,C(0.2f,0.65f,0.2f));
                C2D_DrawCircleSolid(x+6,y-tr-4,0.42f,cr*0.6f,C(0.2f,0.65f,0.2f));
                C2D_DrawCircleSolid(x,y-tr-14,0.43f,cr*0.45f,C(0.15f,0.6f,0.15f));
            }
        } else {
            // rock: layered stone with facets
            C2D_DrawCircleSolid(x,y+3,0.4f,11,C(0.5f,0.5f,0.53f));
            C2D_DrawCircleSolid(x,y,0.41f,9,C(0.6f,0.6f,0.63f));
            C2D_DrawCircleSolid(x-2,y-3,0.42f,6,C(0.72f,0.72f,0.75f));
            C2D_DrawCircleSolid(x+2,y-1,0.43f,4,C(0.8f,0.8f,0.82f));
            // cracks
            C2D_DrawLine(x,y,0.5f,x-4,y+3,C(0.35f,0.35f,0.38f),1.0f,0.45f);
            C2D_DrawLine(x,y,0.5f,x+3,y-2,C(0.35f,0.35f,0.38f),1.0f,0.45f);
            // moss speck
            C2D_DrawCircleSolid(x+4,y+5,0.5f,2,C(0.25f,0.5f,0.25f));
        }
    }
}
static void startGame(void){
    score=20;buildCount=0;hasKitchen=0;hasTemple=0;altarX=WORLD_W/2;altarY=WORLD_H/2;
    px=altarX+40;py=altarY;php=pmaxhp;dmgFlash=0;ritualCd=0;buildFail=0;followCount=0;
    p2Active = (coopOn!=0);
    if(p2Active){ p2x=px-40; p2y=py+20; }
    addR(altarX-40,altarY+20);addR(altarX+40,altarY+10);addR(altarX,altarY+40);
    camx=px-SCR_W/2;camy=py-SCR_H/2;talkTarget=-1;talkOpen=0;buildSel=0;
    spawnResources();
}
static void tryBuild(int ty,float x,float y){
    if(buildCount>=MAX_BUILD)return;
    int cost=(ty==0)?3:(ty==1)?2:(ty==2)?6:(ty==3)?1:8;
    if(score<cost){buildFail=90;return;}
    if(ty==2&&hasKitchen)return;
    if(ty==4&&hasTemple)return;
    bx[buildCount]=x;by[buildCount]=y;btype[buildCount]=ty;buildCount++;
    if(ty==2)hasKitchen=1;
    if(ty==4)hasTemple=1;
    score-=cost;
    if(ty==0)addR(x+rnd(-20,20),y+20);
}
static void doCraft(int idx){
    if(idx<0||idx>=5)return;
    int ty=craftType[idx],w=craftWood[idx],s=craftStone[idx];
    if(rWood<w||rStone<s){buildFail=90;return;}
    if(ty==2&&hasKitchen)return;
    if(ty==4&&hasTemple)return;
    // place near player
    float bx2=px+rnd(-20,20),by2=py+rnd(-10,30);
    if(bx2<40)bx2=40;if(bx2>WORLD_W-40)bx2=WORLD_W-40;
    if(by2<40)by2=40;if(by2>WORLD_H-40)by2=WORLD_H-40;
    bx[buildCount]=bx2;by[buildCount]=by2;btype[buildCount]=ty;buildCount++;
    if(ty==2)hasKitchen=1;
    if(ty==4)hasTemple=1;
    rWood-=w;rStone-=s;
    if(ty==0)addR(bx2+rnd(-20,20),by2+20);
}
static void demolish(float wx,float wy){
    int best=-1;float bd=46*46;
    for(int i=0;i<buildCount;i++){int t=(int)btype[i];if(t==2||t==4)continue;float dx=bx[i]-wx,dy=by[i]-wy,d=dx*dx+dy*dy;if(d<bd){bd=d;best=i;}}
    if(best<0)return;
    for(int i=best;i<buildCount-1;i++){bx[i]=bx[i+1];by[i]=by[i+1];btype[i]=btype[i+1];}
    buildCount--;
}
static void updateCam(void){
    float cx=px,cy=py;
    if(p2Active){ cx=(px+p2x)*0.5f; cy=(py+p2y)*0.5f; }
    camx=cx-SCR_W/2;camy=cy-SCR_H/2;
    if(camx<0)camx=0;if(camy<0)camy=0;
    if(camx>WORLD_W-SCR_W)camx=WORLD_W-SCR_W;if(camy>WORLD_H-SCR_H)camy=WORLD_H-SCR_H;
}
static void drawAlt(float ox,float oy){
    float x=altarX-ox,y=altarY-oy;
    C2D_DrawCircleSolid(x,y,0.2f,16,C(0.45f,0.1f,0.1f));
    C2D_DrawCircleSolid(x,y,0.21f,14,C(0.2f,0.05f,0.05f));
    for(int i=0;i<5;i++){float a0=i*1.2566f-1.5708f,a1=(i+2)*1.2566f-1.5708f;C2D_DrawLine(x+cosf(a0)*16,y+sinf(a0)*16,C(0.9f,0.3f,0.3f),x+cosf(a1)*16,y+sinf(a1)*16,C(0.9f,0.3f,0.3f),1.0f,0.5f);}
    C2D_DrawCircleSolid(x,y,0.3f,5,C(1.0f,0.8f,0.2f));
}
static void drawBuild(float wx,float wy,int ty){
    float x=wx,y=wy;
    if(ty==0){C2D_DrawTriangle(x,y-14,C(0.7f,0.15f,0.15f),x-14,y,C(0.7f,0.15f,0.15f),x+14,y,C(0.7f,0.15f,0.15f),0.4f);C2D_DrawRectSolid(x-14,y,0.39f,28,16,C(0.8f,0.7f,0.5f));C2D_DrawRectSolid(x-4,y+6,0.41f,8,10,C(0.4f,0.25f,0.1f));}
    else if(ty==1){C2D_DrawRectSolid(x-8,y,0.39f,16,14,C(0.75f,0.75f,0.8f));C2D_DrawCircleSolid(x,y-2,0.4f,5,C(0.9f,0.9f,0.95f));C2D_DrawRectSolid(x-3,y+6,0.41f,6,8,C(0.4f,0.4f,0.45f));}
    else if(ty==2){C2D_DrawTriangle(x,y-12,C(0.8f,0.4f,0.2f),x-14,y,C(0.8f,0.4f,0.2f),x+14,y,C(0.8f,0.4f,0.2f),0.4f);C2D_DrawRectSolid(x-14,y,0.39f,28,16,C(0.6f,0.5f,0.35f));C2D_DrawCircleSolid(x,y-4,0.42f,6,C(0.3f,0.3f,0.3f));}
    else if(ty==3){C2D_DrawRectSolid(x-1,y,0.4f,2,8,C(0.2f,0.5f,0.2f));C2D_DrawCircleSolid(x,y-3,0.41f,4,C(1,0.4f,0.6f));C2D_DrawCircleSolid(x-3,y-1,0.41f,2,C(1,0.9f,0.3f));}
    else {float s=2.6f;C2D_DrawTriangle(x,y-34*s,C(0.5f,0.1f,0.55f),x-26*s,y-16,C(0.5f,0.1f,0.55f),x+26*s,y-16,C(0.5f,0.1f,0.55f),0.4f);C2D_DrawRectSolid(x-26*s,y-16,0.39f,52*s,6,C(0.35f,0.1f,0.4f));C2D_DrawRectSolid(x-26*s,y-10,0.39f,52*s,34,C(0.3f,0.08f,0.35f));C2D_DrawRectSolid(x-8,y-2,0.41f,16,26,C(0.15f,0.04f,0.2f));C2D_DrawCircleSolid(x,y-22,0.44f,4,C(1,0.85f,0.2f));}
}
static void gate(float ox,float oy){
    float gx=WORLD_W/2-ox,gy=20-oy;if(gx<-40||gx>SCR_W+40)return;
    C2D_DrawRectSolid(gx-20,gy-22,0.2f,40,24,C(0.2f,0.1f,0.1f));
    C2D_DrawRectSolid(gx-13,gy-16,0.3f,26,16,C(0.05f,0.02f,0.05f));
    C2D_DrawCircleSolid(gx,gy-18,0.4f,8,C(0.7f,0.15f,0.35f));
    text(gx-30,gy+4,0.5f,"ENTER DUNGEONS",0.9f,0.7f,0.6f);
}
static void grass(void){
    C2D_DrawRectSolid(0,0,0,SCR_W,SCR_H,C(0.2f,0.5f,0.15f));
    for(int i=0;i<60;i++){float gx=(i*37+((int)tclock*10))%(SCR_W+8),gy=(i*53)%SCR_H;C2D_DrawRectSolid(gx,gy,0.2f,2,6,C(0.3f,0.6f,0.2f));}
    for(int i=0;i<40;i++){float gx=(i*61+20)%(SCR_W+8),gy=(i*71+30)%SCR_H;C2D_DrawCircleSolid(gx,gy,0.2f,2,C(0.5f,0.7f,0.3f));}
}
static void followers(float ox,float oy){
    for(int i=0;i<followCount;i++){
        float wx=fwX[i]-ox,wy=fwY[i]-oy;if(wx<-30||wx>SCR_W+30||wy<-30||wy>SCR_H+30)continue;
        if(fwWork[i])C2D_DrawRectSolid(wx-5,wy-9,0.35f,10,4,C(1,0.85f,0.2f));
        C2D_DrawRectSolid(wx-4,wy+2,0.3f,8,9,C(0.75f,0.1f,0.1f));C2D_DrawCircleSolid(wx,wy,0.3f,6,C(0.35f,0.6f,0.25f));
        C2D_DrawRectSolid(wx-5,wy-2,0.32f,10,3,C(0.95f,0.95f,0.95f));C2D_DrawRectSolid(wx-5,wy+1,0.33f,10,2,C(0.95f,0.95f,0.95f));
        text(wx-14,wy-16,0.4f,fwName[i],1,1,0.6f);
    }
}
static void drawGame(void){
    updateCam();float ox=camx,oy=camy;
    C2D_TargetClear(top,bg);C2D_SceneBegin(top);
    grass();drawAlt(ox,oy);gate(ox,oy);drawRes(ox,oy);
    for(int i=0;i<buildCount;i++){float x=bx[i]-ox,y=by[i]-oy;if(x<-60||x>SCR_W+60||y<-60||y>SCR_H+60)continue;drawBuild(x,y,(int)btype[i]);}
    followers(ox,oy);
    lamb(px-ox,py-oy,1.0f,0);
    if(p2Active){
        // second player: slightly different colour for clarity (orange-ish body)
        float wx=p2x-ox,wy=p2y-oy;
        C2D_DrawCircleSolid(wx,wy+22*0.9f,0.6f,9*0.9f,C2D_Color32(0,0,0,90));
        C2D_DrawRectSolid(wx-8*0.9f,wy+12*0.9f,0.3f,4*0.9f,9*0.9f,grey);
        C2D_DrawRectSolid(wx+4*0.9f,wy+12*0.9f,0.3f,4*0.9f,9*0.9f,grey);
        C2D_DrawRectSolid(wx-13*0.9f,wy-6*0.9f,0.4f,26*0.9f,20*0.9f,C(0.9f,0.4f,0.1f));
        for(int i=0;i<5;i++){ float z=wx-12*0.9f+i*6*0.9f; C2D_DrawRectSolid(z,wy+13*0.9f,0.41f,5*0.9f,3*0.9f,(i%2)?white:C(0.9f,0.9f,0.9f)); }
        C2D_DrawRectSolid(wx-16*0.9f,wy-2*0.9f,0.5f,5*0.9f,8*0.9f,grey);
        C2D_DrawRectSolid(wx+11*0.9f,wy-2*0.9f,0.5f,5*0.9f,8*0.9f,grey);
        C2D_DrawCircleSolid(wx,wy+3*0.9f,0.51f,4*0.9f,yellow);
        C2D_DrawCircleSolid(wx,wy-15*0.9f,0.5f,9*0.9f,grey);
        C2D_DrawRectSolid(wx-8*0.9f,wy-30*0.9f,0.5f,16*0.9f,8*0.9f,yellow);
        C2D_DrawRectSolid(wx-8*0.9f,wy-30*0.9f,0.51f,16*0.9f,2*0.9f,white);
        C2D_DrawRectSolid(wx-9*0.9f,wy-33*0.9f,0.52f,3*0.9f,5*0.9f,black);
        C2D_DrawRectSolid(wx+6*0.9f,wy-33*0.9f,0.52f,3*0.9f,5*0.9f,black);
        C2D_DrawCircleSolid(wx,wy-26*0.9f,0.6f,2.5f*0.9f,C(1,0,0));
        text(wx-8,wy+26,0.4f,"P2",0.4f,0.9f,1.0f);
    }
    if(dmgFlash>0)C2D_DrawRectSolid(0,0,0.9f,SCR_W,SCR_H,C2D_Color32(255,0,0,50));
    char h[32];snprintf(h,32,"Food:%d Cult:%d",score,followCount);text(8,30,0.5f,h,1,0.85f,0.3f);
    char rh[40];snprintf(rh,40,"Wood:%d Stone:%d",rWood,rStone);text(8,46,0.45f,rh,0.9f,0.8f,0.4f);
    if(ritualCd>0)text(8,46,0.45f,"Ritual charging...",1,0.7f,0.4f);
    text(240,4,0.45f,"A: talk/enter",1,0.9f,0.4f);
}
static void bottomGrass(void){
    C2D_TargetClear(bottom,C(0.2f,0.5f,0.15f));C2D_SceneBegin(bottom);
    C2D_DrawRectSolid(0,0,0,BOT_W,BOT_H,C(0.2f,0.5f,0.15f));
    for(int i=0;i<50;i++){float gx=(i*37+10)%(BOT_W+8),gy=(i*53)%BOT_H;C2D_DrawRectSolid(gx,gy,0.2f,2,6,C(0.3f,0.6f,0.2f));}
}
static void drawBottom(void){
    bottomGrass();
    if(craftOpen){
        const char* cn[5]={"HOUSE","TOILET","KITCHEN","DECOR","TEMPLE"};
        for(int i=0;i<5;i++){
            int y=14+i*44;
            u32 c=(i==craftSel)?C(0.5f,0.2f,0.1f):C(0.2f,0.3f,0.15f);
            C2D_DrawRectSolid(20,y,0.3f,280,38,c);
            char b[40];snprintf(b,40,"%s  W%d S%d",cn[i],craftWood[i],craftStone[i]);
            text(26,y+6,0.45f,b,0.95f,0.95f,0.95f);
        }
        char rm[40];snprintf(rm,40,"Wood:%d Stone:%d",rWood,rStone);text(190,0,0.45f,rm,1,0.8f,0.3f);
        text(90,226,0.4f,"A=craft B=back",0.8f,0.9f,0.8f);
    } else if(talkOpen){
        const char* a[4]={"WORK","SCARE","CHEER","READ"};
        for(int i=0;i<4;i++){int y=20+i*50;u32 c=(i==talkSel)?C(0.55f,0.1f,0.15f):C(0.2f,0.4f,0.12f);C2D_DrawRectSolid(20,y-4,0.3f,280,34,c);text(40,y,0.5f,a[i],0.9f,0.9f,0.9f);}
    }else{
        const char* ty[6]={"HOUSE","TOILET","KITCHEN","DECOR","TEMPLE","DESTROY"};
        for(int i=0;i<6;i++){int y=16+i*34;u32 c=(i==buildSel)?C(0.6f,0.15f,0.15f):C(0.15f,0.4f,0.1f);if(i==5)c=(i==buildSel)?C(0.7f,0.15f,0.08f):C(0.35f,0.18f,0.06f);C2D_DrawRectSolid(20,y,0.3f,120,28,c);text(26,y+2,0.4f,ty[i],0.95f,0.95f,0.95f);}
        C2D_DrawRectSolid(165,60,0.3f,140,120,C(0.1f,0.3f,0.08f));text(172,110,0.5f,"TAP=bldg",1,0.9f,0.4f);
        if(buildSel==5)text(172,150,0.4f,"del",1,0.5f,0.3f);
        if(buildFail>0)text(172,190,0.5f,"NO FOOD!",1,0.4f,0.2f);
    }
}
static void talkTop(void){
    C2D_TargetClear(top,bg);C2D_SceneBegin(top);grass();C2D_DrawRectSolid(0,0,0.5f,SCR_W,SCR_H,C2D_Color32(0,0,0,160));
    char t[48];if(talkTarget>=0&&talkTarget<followCount)snprintf(t,48,"%s - M%d",fwName[talkTarget],(int)fwM[talkTarget]);else snprintf(t,48,"No one");
    text(40,30,0.6f,t,1,0.9f,0.4f);
}
static void doTalk(int a){
    if(talkTarget<0||talkTarget>=followCount){talkOpen=0;state=ST_PLAY;return;}int i=talkTarget;
    switch(a){case 0:fwWork[i]=1;score+=3;break;case 1:fwM[i]-=15;score+=4;break;case 2:fwM[i]+=20;if(score>=2)score-=2;break;default:break;}
    talkOpen=0;state=ST_PLAY;
}
static const char* bName(void){if(dTh==0)return "LESHY";if(dTh==1)return "KALLAMAR";if(dTh==2)return "SHAMURA";return "HEKAT";}
static void hubIn(void){hx=200;hy=200;php=pmaxhp;}
static void hubUp(float dt){
    u32 k=hidKeysHeld();float dx=0,dy=0;if(k&KEY_CPAD_UP)dy-=1;if(k&KEY_CPAD_DOWN)dy+=1;if(k&KEY_CPAD_LEFT)dx-=1;if(k&KEY_CPAD_RIGHT)dx+=1;
    float l=sqrtf(dx*dx+dy*dy);if(l>0){dx/=l;dy/=l;}hx+=dx*2.6f*60.f*dt;hy+=dy*2.6f*60.f*dt;
    if(hx<15)hx=15;if(hx>SCR_W-15)hx=SCR_W-15;if(hy<15)hy=15;if(hy>SCR_H-15)hy=SCR_H-15;
    u32 down=hidKeysDown();if(down&KEY_B){state=ST_PLAY;}
    if(down&KEY_A){if(hypotf(hx-110,hy-70)<32)loadD(0);else if(hypotf(hx-290,hy-70)<32)loadD(1);else if(hypotf(hx-110,hy-190)<32)loadD(2);else if(hypotf(hx-290,hy-190)<32)loadD(3);else if(hypotf(hx-200,hy-225)<36)state=ST_PLAY;}
}
static void hubDraw(void){
    C2D_TargetClear(top,C(0.15f,0.12f,0.2f));C2D_SceneBegin(top);C2D_DrawRectSolid(0,0,0,SCR_W,SCR_H,C(0.15f,0.12f,0.2f));
    struct{float x,y;const char* t;u32 c;}dr[4]={{110,70,"FOREST",C(0.2f,0.5f,0.2f)},{290,70,"WATER",C(0.2f,0.4f,0.75f)},{110,190,"SPIDERS",C(0.35f,0.15f,0.35f)},{290,190,"TOADS",C(0.3f,0.55f,0.3f)}};
    for(int i=0;i<4;i++){C2D_DrawRectSolid(dr[i].x-22,dr[i].y-30,0.2f,44,60,C(0.08f,0.06f,0.1f));C2D_DrawRectSolid(dr[i].x-16,dr[i].y-24,0.21f,32,48,dr[i].c);C2D_DrawRectSolid(dr[i].x-10,dr[i].y-18,0.22f,20,36,C(0.03f,0.02f,0.04f));text(dr[i].x-26,dr[i].y+32,0.4f,dr[i].t,0.95f,0.95f,0.95f);}
    C2D_DrawRectSolid(176,218,0.2f,48,20,C(0.35f,0.25f,0.2f));text(185,226,0.4f,"BACK",1,0.85f,0.7f);
    lamb(hx,hy,1.0f,1);text(50,6,0.5f,"DUNGEON HALL",1,0.9f,0.4f);
    C2D_TargetClear(bottom,C(0.15f,0.12f,0.2f));C2D_SceneBegin(bottom);C2D_DrawRectSolid(0,0,0,BOT_W,BOT_H,C(0.15f,0.12f,0.2f));
    text(30,30,0.6f,"Pick a door",1,0.9f,0.4f);text(30,70,0.6f,"A: enter",1,0.9f,0.4f);text(30,110,0.6f,"B: back",1,0.9f,0.4f);
}
static void loadRoom(void){
    for(int i=0;i<MAX_ENEMY;i++){exd[i]=eyd[i]=ehpz[i]=0;}dCnt=0;dEnem=0;dpx=40;dpy=SCR_H/2;
    if(dRoom==DUNGEON_ROOMS-1){exd[0]=SCR_W/2+60;eyd[0]=SCR_H/2;ehpz[0]=220;espd[0]=0.8f;dCnt=1;dEnem=1;}
    else{int n=3+(dRoom%3);for(int i=0;i<n&&dCnt<MAX_ENEMY;i++){float xx=120+(float)((i*97)%180),yy=70+(float)((i*53)%120);exd[dCnt]=xx;eyd[dCnt]=yy;ehpz[dCnt]=35+i*6;espd[dCnt]=1.1f+dTh*0.08f;dCnt++;dEnem++;}}
    dAtkC=0;dAtkE=0;dRed=0;
}
static void loadD(int th){dTh=th;dRoom=0;state=ST_DUN;php=pmaxhp;loadRoom();}
static void dUp(float dt){
    u32 k=hidKeysHeld();float dx=0,dy=0;if(k&KEY_CPAD_UP)dy-=1;if(k&KEY_CPAD_DOWN)dy+=1;if(k&KEY_CPAD_LEFT)dx-=1;if(k&KEY_CPAD_RIGHT)dx+=1;
    float l=sqrtf(dx*dx+dy*dy);if(l>0){dx/=l;dy/=l;}dpx+=dx*2.6f*60.f*dt;dpy+=dy*2.6f*60.f*dt;
    if(dEnem>0){if(dpx<26)dpx=26;if(dpx>SCR_W-26)dpx=SCR_W-26;}else{if(dpx<0)dpx=0;if(dpx>SCR_W)dpx=SCR_W;}
    if(dpy<10)dpy=10;if(dpy>SCR_H-10)dpy=SCR_H-10;
    for(int i=0;i<dCnt;i++){if(ehpz[i]<=0)continue;float a=atan2f(dpy-eyd[i],dpx-exd[i]),sp=espd[i]*60.f*dt;exd[i]+=cosf(a)*sp;eyd[i]+=sinf(a)*sp;if(exd[i]<14)exd[i]=14;if(exd[i]>SCR_W-14)exd[i]=SCR_W-14;if(eyd[i]<14)eyd[i]=14;if(eyd[i]>SCR_H-14)eyd[i]=SCR_H-14;}
    u32 down=hidKeysDown();if(dAtkC>0)dAtkC-=dt;
    if((down&KEY_A)&&dAtkC<=0){dAtkC=0.25f;for(int i=0;i<dCnt;i++){if(ehpz[i]<=0)continue;if(hypotf(exd[i]-dpx,eyd[i]-dpy)<34){ehpz[i]-=25;if(ehpz[i]<=0)dEnem--;break;}}}
    dAtkE-=dt;if(dAtkE<=0){dAtkE=1.0f;for(int i=0;i<dCnt;i++){if(ehpz[i]<=0)continue;if(hypotf(exd[i]-dpx,eyd[i]-dpy)<18){php-=8;dRed=2.0f;if(php<0)php=0;}}}
    if(dRed>0)dRed-=dt;
    if(dEnem<=0){if(dpx<20){hubIn();state=ST_HUB;return;}if(dpx>SCR_W-20){if(dRoom<DUNGEON_ROOMS-1){dRoom++;loadRoom();}else{score+=12;addR(altarX+rnd(-30,30),altarY+rnd(0,40));hubIn();state=ST_HUB;}return;}}
}
static void enemyD(float x,float y){
    C2D_DrawRectSolid(x-9,y-2,0.3f,18,20,C(0.14f,0.14f,0.15f));
    C2D_DrawTriangle(x-8,y+16,C(0.1f,0.1f,0.11f),x-13,y+22,C(0.08f,0.08f,0.09f),x-3,y+21,C(0.1f,0.1f,0.11f),0.35f);
    C2D_DrawTriangle(x+8,y+16,C(0.1f,0.1f,0.11f),x+13,y+22,C(0.08f,0.08f,0.09f),x+3,y+21,C(0.1f,0.1f,0.11f),0.35f);
    C2D_DrawCircleSolid(x,y-5,0.31f,9,C(0.1f,0.1f,0.12f));
    C2D_DrawRectSolid(x-2,y-9,0.5f,4,11,C(0.9f,0.1f,0.1f));
    C2D_DrawRectSolid(x-1,y-5,0.51f,2,6,C(0.9f,0.1f,0.1f));
    C2D_DrawRectSolid(x+6,y-5,0.5f,2,8,C(0.75f,0.78f,0.85f));
    C2D_DrawTriangle(x+6,y+6,C(0.75f,0.78f,0.85f),x+3,y+1,C(0.75f,0.78f,0.85f),x+8,y+1,C(0.75f,0.78f,0.85f),0.5f);
}
static void bossD(float x,float y){
    if(dTh==0){
        C2D_DrawRectSolid(x-20,y,0.3f,40,30,C(0.32f,0.22f,0.1f));
        C2D_DrawCircleSolid(x,y+6,0.4f,16,C(0.1f,0.05f,0.03f));
        for(int i=0;i<5;i++)C2D_DrawTriangle(x-12+i*6,y+4,C(1,1,1),x-8+i*6,y+4,C(1,1,1),x-10+i*6,y+12,C(1,1,1),0.45f);
        C2D_DrawCircleSolid(x-6,y-6,0.34f,4,red);C2D_DrawCircleSolid(x+6,y-6,0.34f,4,red);
        C2D_DrawTriangle(x-14,y-6,C(0.2f,0.5f,0.2f),x-20,y-16,C(0.15f,0.45f,0.15f),x-6,y-10,C(0.2f,0.5f,0.2f),0.4f);
        C2D_DrawTriangle(x+14,y-6,C(0.2f,0.5f,0.2f),x+20,y-16,C(0.15f,0.45f,0.15f),x+6,y-10,C(0.2f,0.5f,0.2f),0.4f);
    }else if(dTh==1){
        C2D_DrawCircleSolid(x,y,0.3f,14,C(0.2f,0.4f,0.8f));C2D_DrawCircleSolid(x,y,0.31f,10,C(0.35f,0.6f,0.9f));C2D_DrawCircleSolid(x,y-6,0.34f,3,white);C2D_DrawRectSolid(x-3,y,0.4f,6,16,C(0.15f,0.35f,0.75f));
    }else if(dTh==2){
        C2D_DrawCircleSolid(x,y,0.3f,14,C(0.35f,0.18f,0.4f));C2D_DrawCircleSolid(x,y,0.31f,9,C(0.45f,0.25f,0.5f));C2D_DrawCircleSolid(x-3,y-4,0.34f,2,red);C2D_DrawCircleSolid(x+3,y-4,0.34f,2,red);
        for(int i=0;i<4;i++)C2D_DrawRectSolid(x-16*cosf(i*1.2f-2.4f),y+(i%2?4:-4),0.4f,14*cosf(i*1.2f-2.4f),3,C(0.2f,0.1f,0.25f));
    }else{
        C2D_DrawCircleSolid(x,y+4,0.3f,16,C(0.3f,0.55f,0.32f));C2D_DrawCircleSolid(x,y-4,0.31f,12,C(0.4f,0.65f,0.4f));C2D_DrawCircleSolid(x-6,y-6,0.34f,3,C(0.2f,0.45f,0.2f));C2D_DrawCircleSolid(x+6,y-6,0.34f,3,C(0.2f,0.45f,0.2f));C2D_DrawRectSolid(x-8,y+8,0.35f,16,5,black);
    }
    text(x-30,y+26,0.5f,bName(),1,0.85f,0.3f);
}
static void dDraw(void){
    u32 rb=(dTh==0)?C(0.1f,0.2f,0.08f):(dTh==1)?C(0.08f,0.12f,0.25f):(dTh==2)?C(0.12f,0.05f,0.12f):C(0.12f,0.22f,0.1f);
    C2D_TargetClear(top,rb);C2D_SceneBegin(top);C2D_DrawRectSolid(0,0,0,SCR_W,SCR_H,rb);
    C2D_DrawRectSolid(0,100,0.2f,14,70,C(0.5f,0.35f,0.2f));text(2,120,0.4f,"<",1,0.9f,0.9f);
    if(dEnem<=0){C2D_DrawRectSolid(SCR_W-14,100,0.2f,14,70,C(0.2f,0.7f,0.3f));text(SCR_W-12,120,0.4f,">",1,1,1);}else{C2D_DrawRectSolid(SCR_W-14,100,0.2f,14,70,C(0.4f,0.15f,0.1f));}
    char rl[48];const char* tn[4]={"FOREST","WATER","SPIDERS","TOADS"};snprintf(rl,48,"%s %d/%d",tn[dTh],dRoom+1,DUNGEON_ROOMS);text(6,6,0.5f,rl,1,0.9f,0.4f);
    bool boss=(dRoom==DUNGEON_ROOMS-1);
    for(int i=0;i<dCnt;i++){if(ehpz[i]<=0)continue;if(boss)bossD(exd[i],eyd[i]);else enemyD(exd[i],eyd[i]);}
    lamb(dpx,dpy,0.8f,1);
    if(dRed>0)C2D_DrawRectSolid(0,0,0.9f,SCR_W,SCR_H,C2D_Color32(255,0,0,60));
    if(boss&&dEnem>0){C2D_DrawRectSolid(140,22,0.5f,120,10,C(0.15f,0.05f,0.05f));float f=ehpz[0]/220.0f;if(f<0)f=0;C2D_DrawRectSolid(141,23,0.51f,118*f,8,C(0.9f,0.2f,0.15f));}
    C2D_TargetClear(bottom,C(0.08f,0.04f,0.04f));C2D_SceneBegin(bottom);C2D_DrawRectSolid(0,0,0,BOT_W,BOT_H,C(0.1f,0.05f,0.05f));
    text(30,20,0.6f,"A = FIGHT",1,0.6f,0.5f);text(30,60,0.6f,"B = back",1,0.6f,0.5f);
    char h[40];snprintf(h,40,"HP %d En %d",(int)php,dEnem);text(30,100,0.6f,h,1,0.8f,0.4f);
}
static void tmpIn(void){tPx=TEMPLE_W/2;tPy=TEMPLE_H-80;tAltarX=TEMPLE_W/2;tAltarY=TEMPLE_H/2;tmpSel=0;tmpUi=0;}
static void tmpUp(float dt){
    u32 k=hidKeysHeld();float dx=0,dy=0;if(k&KEY_CPAD_UP)dy-=1;if(k&KEY_CPAD_DOWN)dy+=1;if(k&KEY_CPAD_LEFT)dx-=1;if(k&KEY_CPAD_RIGHT)dx+=1;
    float l=sqrtf(dx*dx+dy*dy);if(l>0){dx/=l;dy/=l;}tPx+=dx*2.6f*60.f*dt;tPy+=dy*2.6f*60.f*dt;
    if(tPx<40)tPx=40;if(tPx>TEMPLE_W-40)tPx=TEMPLE_W-40;if(tPy<40)tPy=40;if(tPy>TEMPLE_H-40)tPy=TEMPLE_H-40;
    float da=hypotf(tAltarX-tPx,tAltarY-tPy);tmpUi=(da<40)?1:0;
    u32 down=hidKeysDown();if(down&KEY_B){state=ST_PLAY;}
    if(tmpUi){if(down&(KEY_DUP|KEY_CPAD_UP))tmpSel=(tmpSel+2)%3;if(down&(KEY_DDOWN|KEY_CPAD_DOWN))tmpSel=(tmpSel+1)%3;if(down&KEY_A)tmpAct(tmpSel);}
}
static void tmpDraw(void){
    tCamx=tPx-SCR_W/2;tCamy=tPy-SCR_H/2;if(tCamx<0)tCamx=0;if(tCamy<0)tCamy=0;if(tCamx>TEMPLE_W-SCR_W)tCamx=TEMPLE_W-SCR_W;if(tCamy>TEMPLE_H-SCR_H)tCamy=TEMPLE_H-SCR_H;
    C2D_TargetClear(top,C(0.18f,0.07f,0.28f));C2D_SceneBegin(top);C2D_DrawRectSolid(0,0,0,SCR_W,SCR_H,C(0.4f,0.2f,0.4f));
    float ax=tAltarX-tCamx,ay=tAltarY-tCamy;
    C2D_DrawCircleSolid(ax,ay,0.2f,40,C(0.55f,0.1f,0.1f));C2D_DrawCircleSolid(ax,ay,0.21f,32,C(0.25f,0.05f,0.05f));
    for(int i=0;i<5;i++){float a0=i*1.2566f-1.5708f,a1=(i+2)*1.2566f-1.5708f;C2D_DrawLine(ax+cosf(a0)*32,ay+sinf(a0)*32,C(0.9f,0.3f,0.3f),ax+cosf(a1)*32,ay+sinf(a1)*32,C(0.9f,0.3f,0.3f),2.0f,0.4f);}
    text(ax-40,ay+44,0.5f,"THE ALTAR",1,0.8f,0.4f);if(tmpUi)text(ax-20,ay-60,0.5f,"A to act",1,0.9f,0.4f);
    lamb(tPx-tCamx,tPy-tCamy,1.0f,0);text(SCR_W/2-60,SCR_H-20,0.5f,"B=leave",0.9f,0.8f,0.6f);
    C2D_TargetClear(bottom,C(0.15f,0.05f,0.18f));C2D_SceneBegin(bottom);C2D_DrawRectSolid(0,0,0,BOT_W,BOT_H,C(0.15f,0.05f,0.18f));
    if(tmpUi){const char* a[3]={"PREACH","RITUAL","LEAVE"};for(int i=0;i<3;i++){int y=30+i*60;u32 c=(i==tmpSel)?C(0.5f,0.1f,0.55f):C(0.2f,0.08f,0.25f);C2D_DrawRectSolid(40,y-4,0.3f,240,40,c);float cr=(i==tmpSel)?1.0f:0.9f;text(60,y,0.55f,a[i],cr,0.9f,0.9f);}}
    else{text(20,30,0.6f,"Move: pad",1,0.9f,0.4f);text(20,60,0.6f,"A: act",1,0.9f,0.4f);text(20,90,0.6f,"B: leave",1,0.9f,0.4f);}
}
static void tmpAct(int a){
    if(a==0){if(followCount>0){for(int i=0;i<followCount;i++){fwM[i]+=10;if(fwM[i]>100)fwM[i]=100;}score+=2;}}
    else if(a==1){if(ritualCd<=0&&score>=6){score-=6;for(int i=0;i<followCount;i++){fwM[i]+=30;if(fwM[i]>100)fwM[i]=100;}ritualCd=20.0f;int g=followCount?followCount/2:0;score+=g;}}
    else if(a==2){state=ST_PLAY;}
}
static void mTop(void){
    C2D_TargetClear(top,white);C2D_SceneBegin(top);C2D_DrawRectSolid(0,0,0,SCR_W,SCR_H,white);
    float cx=SCR_W/2,cy=SCR_H/2+10,R=90;C2D_DrawCircleSolid(cx,cy,0.2f,R,C(0.9f,0.15f,0.15f));C2D_DrawCircleSolid(cx,cy,0.21f,R-6,white);
    for(int i=0;i<5;i++){float a0=i*1.2566f-1.5708f,a1=(i+2)*1.2566f-1.5708f;C2D_DrawLine(cx+cosf(a0)*R,cy+sinf(a0)*R,C(0.9f,0.15f,0.15f),cx+cosf(a1)*R,cy+sinf(a1)*R,C(0.9f,0.15f,0.15f),3.0f,0.5f);}
    lamb(cx,cy-R-18,1.4f,0);text(40,12,0.8f,"CULT OF THE LAMB",0.9f,0.1f,0.1f);
}
static void mBot(void){
    C2D_TargetClear(bottom,black);C2D_SceneBegin(bottom);C2D_DrawRectSolid(0,0,0,BOT_W,BOT_H,black);
    text(40,6,0.5f,"CULT OF THE LAMB",1,0.2f,0.2f);
    const char* it[4]={"PLAY","NEW GAME","SETTINGS","EXIT"};
    for(int i=0;i<4;i++){
        int y=45+i*48;u32 c=(i==menuSel)?C(0.55f,0.1f,0.15f):C(0.18f,0.12f,0.1f);
        C2D_DrawRectSolid(40,y-4,0.3f,240,30,c);
        text(60,y-2,0.6f,it[i],0.9f,0.9f,0.9f);
    }
}
static void optDraw(void){
    C2D_TargetClear(top,C(0.3f,0.2f,0.2f));
    C2D_SceneBegin(top);
    C2D_DrawRectSolid(0,0,0,SCR_W,SCR_H,C(0.3f,0.2f,0.2f));
    text(60,20,0.7f,"SETTINGS",1,0.9f,0.3f);
    // two options
    const char* names[2]={"COOP","N3DS MODE"};
    for(int i=0;i<2;i++){
        int y=80+i*60;
        u32 c=(i==optSel)?C(0.5f,0.15f,0.1f):C(0.2f,0.2f,0.2f);
        C2D_DrawRectSolid(40,y-4,0.3f,320,44,c);
        char b[30];
        if(i==0)snprintf(b,30,"%s: %s",names[i],coopOn?"ON":"OFF");
        else snprintf(b,30,"%s: %s",names[i],n3dsOn?"ON":"OFF");
        text(60,y+6,0.55f,b,0.95f,0.95f,0.95f);
    }
    text(40,220,0.4f,"A=on/off B=back",0.8f,0.9f,0.8f);
    C2D_TargetClear(bottom,C(0.3f,0.2f,0.2f));
    C2D_SceneBegin(bottom);
    C2D_DrawRectSolid(0,0,0,BOT_W,BOT_H,C(0.3f,0.2f,0.2f));
    text(40,90,0.5f,"A=on/off B=back",1,1,1);
}
static void save(void){
    FILE* f=fopen(SV,"wb");if(!f)return;int n=followCount;
    fwrite(&score,4,1,f);fwrite(&hasKitchen,4,1,f);fwrite(&hasTemple,4,1,f);fwrite(&n,4,1,f);fwrite(&buildCount,4,1,f);
    for(int i=0;i<buildCount;i++){fwrite(&bx[i],4,1,f);fwrite(&by[i],4,1,f);fwrite(&btype[i],4,1,f);}
    for(int i=0;i<n;i++){fwrite(fwName[i],16,1,f);fwrite(&fwM[i],4,1,f);fwrite(&fwWork[i],4,1,f);fwrite(&fwX[i],4,1,f);fwrite(&fwY[i],4,1,f);}
    fclose(f);
}
static int load(void){
    FILE* f=fopen(SV,"rb");if(!f)return 0;int n=0;
    fread(&score,4,1,f);fread(&hasKitchen,4,1,f);fread(&hasTemple,4,1,f);fread(&n,4,1,f);fread(&buildCount,4,1,f);
    if(n<0||n>MAX_FOLLOW)n=0;if(buildCount<0||buildCount>MAX_BUILD)buildCount=0;
    for(int i=0;i<buildCount;i++){fread(&bx[i],4,1,f);fread(&by[i],4,1,f);fread(&btype[i],4,1,f);}
    followCount=n;for(int i=0;i<n;i++){fread(fwName[i],16,1,f);fread(&fwM[i],4,1,f);fread(&fwWork[i],4,1,f);fread(&fwX[i],4,1,f);fread(&fwY[i],4,1,f);fwT[i]=rnd(0,6);}
    fclose(f);return 1;
}
static void newGame(void){remove(SV);startGame();state=ST_PLAY;}
static void update(float dt){
    tclock+=dt;
    u32 all=hidKeysDown();
    u32 fresh=all & ~lastKeys;   // only newly-pressed
    lastKeys=all;
    if(state==ST_PLAY){
        u32 k=hidKeysHeld();float dx=0,dy=0;if(k&KEY_CPAD_UP)dy-=1;if(k&KEY_CPAD_DOWN)dy+=1;if(k&KEY_CPAD_LEFT)dx-=1;if(k&KEY_CPAD_RIGHT)dx+=1;
        float l=sqrtf(dx*dx+dy*dy);if(l>0){dx/=l;dy/=l;}px+=dx*2.6f*60.f*dt;py+=dy*2.6f*60.f*dt;
        if(px<20)px=20;if(px>WORLD_W-20)px=WORLD_W-20;if(py<20)py=20;if(py>WORLD_H-20)py=WORLD_H-20;
        if(p2Active){
            float d2x=0,d2y=0;
            if(k&KEY_DUP)d2y-=1;if(k&KEY_DDOWN)d2y+=1;if(k&KEY_DLEFT)d2x-=1;if(k&KEY_DRIGHT)d2x+=1;
            float l2=sqrtf(d2x*d2x+d2y*d2y);if(l2>0){d2x/=l2;d2y/=l2;}
            p2x+=d2x*2.6f*60.f*dt;p2y+=d2y*2.6f*60.f*dt;
            if(p2x<20)p2x=20;if(p2x>WORLD_W-20)p2x=WORLD_W-20;if(p2y<20)p2y=20;if(p2y>WORLD_H-20)p2y=WORLD_H-20;
        }
        for(int i=0;i<followCount;i++){fwT[i]+=dt*0.6f;fwX[i]+=cosf(fwT[i])*0.6f;fwY[i]+=sinf(fwT[i]*0.9f)*0.6f;if(fwX[i]<20)fwX[i]=20;if(fwX[i]>WORLD_W-20)fwX[i]=WORLD_W-20;if(fwY[i]<20)fwY[i]=20;if(fwY[i]>WORLD_H-20)fwY[i]=WORLD_H-20;}
        if(ritualCd>0)ritualCd-=dt;if(buildFail>0)buildFail-=dt;
        touchPosition tp;hidTouchRead(&tp);static float tcd=0;if(tcd>0)tcd-=dt;
        if((tp.px!=0||tp.py!=0)&&tcd<=0){if(tp.px<160){int s=(int)(tp.py/34);if(s>=0&&s<6)buildSel=s;}else{float wx=camx+((tp.px-170)/((float)BOT_W-165))*SCR_W,wy=camy+(tp.py/(float)BOT_H)*SCR_H;if(buildSel==5)demolish(wx,wy);else tryBuild(buildSel,wx,wy);}tcd=0.3f;}
        static float acc=0;acc+=dt;if(acc>2.0f){acc=0;int wt=0;for(int i=0;i<followCount;i++)if(fwWork[i])wt++;score+= wt+(hasKitchen?2:0)+followCount/3;}
        if(dmgFlash>0)dmgFlash-=dt;
        updateRes(dt);
        u32 down=hidKeysDown();bool acted=false;
        if(down&KEY_X){ chopNear(px,py); chipRock(px,py); }
        if(down&KEY_R && p2Active){ chopNear(p2x,p2y); chipRock(p2x,p2y); }
        if(down&KEY_Y){ craftOpen=1; craftSel=0; }
        if((down&KEY_A)&&hypotf(WORLD_W/2-px,20-py)<40){hubIn();state=ST_HUB;acted=true;}
        if(!acted&&(down&KEY_A)&&hasTemple){for(int i=0;i<buildCount;i++){if((int)btype[i]==4&&hypotf(bx[i]-px,by[i]-py)<50){tmpIn();state=ST_TMP;acted=true;break;}}}
        if(!acted&&(down&KEY_A)){for(int i=0;i<followCount;i++){if(hypotf(fwX[i]-px,fwY[i]-py)<28){talkTarget=i;talkSel=0;talkOpen=1;state=ST_TALK;break;}}}
        // craft menu handling
        if(craftOpen){
            if(down&KEY_B){craftOpen=0;}
            if(down&(KEY_DUP|KEY_CPAD_UP))craftSel=(craftSel+4)%5;
            if(down&(KEY_DDOWN|KEY_CPAD_DOWN))craftSel=(craftSel+1)%5;
            if(down&KEY_A){doCraft(craftSel);}
            touchPosition ctp;hidTouchRead(&ctp);static float ccd=0;if(ccd>0)ccd-=dt;
            if((ctp.px!=0||ctp.py!=0)&&ccd<=0){int row=(ctp.py-40)/40;if(row>=0&&row<5){craftSel=row;doCraft(row);}ccd=0.3f;}
        }
    }else if(state==ST_HUB){hubUp(dt);}
    else if(state==ST_DUN){dUp(dt);}
    else if(state==ST_TALK){touchPosition tp;hidTouchRead(&tp);u32 down=hidKeysDown();if(down&KEY_B){talkOpen=0;state=ST_PLAY;}if(down&(KEY_DUP|KEY_CPAD_UP))talkSel=(talkSel+3)%4;if(down&(KEY_DDOWN|KEY_CPAD_DOWN))talkSel=(talkSel+1)%4;static float tc=0;if(tc>0)tc-=dt;if((tp.px!=0||tp.py!=0)&&tc<=0){int row=(tp.py-20)/50;if(row>=0&&row<4){talkSel=row;doTalk(row);}tc=0.3f;}if(down&KEY_A)doTalk(talkSel);}
    else if(state==ST_TMP){tmpUp(dt);}
    else if(state==ST_OPT){
        if(fresh&(KEY_DUP|KEY_CPAD_UP))optSel=(optSel+1)%2;
        if(fresh&(KEY_DDOWN|KEY_CPAD_DOWN))optSel=(optSel+1)%2;
        if(fresh&KEY_A){
            if(optSel==0)coopOn=(coopOn?0:1);
            else n3dsOn=(n3dsOn?0:1);
        }
        if(fresh&KEY_B)state=ST_MENU;
    }
else if(state==ST_MENU){touchPosition tp;hidTouchRead(&tp);static float mc=0;if(mc>0)mc-=dt;if(fresh&(KEY_DUP|KEY_CPAD_UP))menuSel=(menuSel+3)%4;if(fresh&(KEY_DDOWN|KEY_CPAD_DOWN))menuSel=(menuSel+1)%4;if((tp.px!=0||tp.py!=0)&&mc<=0){int row=(tp.py-45)/48;if(row>=0&&row<4)menuSel=row;if(row==0){startGame();if(load())state=ST_PLAY;}else if(row==1)newGame();else if(row==2)state=ST_OPT;else if(row==3)wantExit=true;mc=0.3f;}if(fresh&KEY_A){if(menuSel==0){startGame();if(load())state=ST_PLAY;}else if(menuSel==1)newGame();else if(menuSel==2)state=ST_OPT;else if(menuSel==3)wantExit=true;}}
    }
int main(void){
    srand((unsigned)time(NULL));gfxInitDefault();C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);C2D_Init(C2D_DEFAULT_MAX_OBJECTS);C2D_Prepare();
    top=C2D_CreateScreenTarget(GFX_TOP,GFX_LEFT);bottom=C2D_CreateScreenTarget(GFX_BOTTOM,GFX_LEFT);
    fnt=C2D_FontLoadSystem(CFG_REGION_USA);tbuf=C2D_TextBufNew(4096);
    bg=C(0.2f,0.5f,0.15f);red=C(0.85f,0.05f,0.1f);cream=C(0.95f,0.9f,0.8f);black=C(0.05f,0.05f,0.05f);white=C(1,1,1);grey=C(0.45f,0.42f,0.42f);yellow=C(1,0.85f,0.2f);
    while(aptMainLoop()){
        hidScanInput();u32 down=hidKeysDown();if(down&KEY_START){save();break;}if(wantExit){save();break;}
        update(1.0f/60.0f);C2D_TextBufClear(tbuf);C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
        if(state==ST_PLAY){drawGame();drawBottom();}
        else if(state==ST_HUB){hubDraw();}
        else if(state==ST_DUN){dDraw();}
        else if(state==ST_TALK){talkTop();drawBottom();}
        else if(state==ST_TMP){tmpDraw();}
        else if(state==ST_OPT){optDraw();}
        else {mTop();mBot();}
        C3D_FrameEnd(0);
    }
    C2D_TextBufDelete(tbuf);C2D_FontFree(fnt);C2D_Fini();C3D_Fini();gfxExit();return 0;
}
