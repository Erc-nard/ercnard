// DOTLAND: tiny native Win32 two-player territory game. No engine or assets.
#include <windows.h>
#include <vector>
#include <cmath>
#include <algorithm>
#include <random>
#include <string>

constexpr int BW=900, BH=600, OX=15, OY=86, DOTS=17, GRID=3, GW=BW/GRID, GH=BH/GRID;
struct Pt { float x,y; };
struct Edge { int a,b,p; };
struct Snap { std::vector<Edge> e; std::vector<signed char> land; int turn; };
std::vector<Pt> dots; std::vector<Edge> edges; std::vector<signed char> land(GW*GH,-1); std::vector<Snap> history;
int turn=0, selected=-1; bool ended=false;
const COLORREF COL[2]={RGB(255,82,109),RGB(69,207,255)};

float dist(Pt a,Pt b){float x=a.x-b.x,y=a.y-b.y;return sqrtf(x*x+y*y);}
float cross(Pt a,Pt b,Pt c){return (b.x-a.x)*(c.y-a.y)-(b.y-a.y)*(c.x-a.x);}
bool between(Pt a,Pt b,Pt p){return p.x>=std::min(a.x,b.x)-.01f&&p.x<=std::max(a.x,b.x)+.01f&&p.y>=std::min(a.y,b.y)-.01f&&p.y<=std::max(a.y,b.y)+.01f;}
bool segCross(Pt a,Pt b,Pt c,Pt d){float x=cross(a,b,c),y=cross(a,b,d),z=cross(c,d,a),w=cross(c,d,b);return x*y<-.01f&&z*w<-.01f;}
bool legal(int a,int b){
  if(a==b)return false;
  for(auto e:edges){if((e.a==a&&e.b==b)||(e.a==b&&e.b==a))return false;}
  for(int i=0;i<(int)dots.size();i++)if(i!=a&&i!=b&&fabsf(cross(dots[a],dots[b],dots[i]))<.01f&&between(dots[a],dots[b],dots[i]))return false;
  for(auto e:edges)if(e.a!=a&&e.a!=b&&e.b!=a&&e.b!=b&&segCross(dots[a],dots[b],dots[e.a],dots[e.b]))return false;
  return true;
}
bool inside(float x,float y,const std::vector<int>& path){
  bool hit=false; for(int i=0,j=(int)path.size()-1;i<(int)path.size();j=i++){
    Pt a=dots[path[i]],b=dots[path[j]]; if((a.y>y)!=(b.y>y)&&x<(b.x-a.x)*(y-a.y)/(b.y-a.y)+a.x)hit=!hit;
  } return hit;
}
float area(const std::vector<int>& p){float a=0;for(int i=0;i<(int)p.size();i++){Pt q=dots[p[(i+1)%p.size()]],r=dots[p[i]];a+=r.x*q.y-q.x*r.y;}return fabsf(a*.5f);}
void findPaths(int goal,int v,std::vector<std::vector<int>>& adj,std::vector<int>& path,std::vector<char>& seen,std::vector<std::vector<int>>& out){
  if(out.size()>180)return; if(v==goal){if(path.size()>=3)out.push_back(path);return;}
  for(int n:adj[v])if(!seen[n]){seen[n]=1;path.push_back(n);findPaths(goal,n,adj,path,seen,out);path.pop_back();seen[n]=0;}
}
int claim(int a,int b,int player){
  std::vector<std::vector<int>> adj(DOTS), loops; for(auto e:edges){adj[e.a].push_back(e.b);adj[e.b].push_back(e.a);}
  std::vector<int> p{a};std::vector<char> seen(DOTS);seen[a]=1;findPaths(b,a,adj,p,seen,loops);int gained=0;
  for(auto& loop:loops){ if(area(loop)<150)continue; float minx=BW,miny=BH,maxx=0,maxy=0;for(int i:loop){minx=std::min(minx,dots[i].x);maxx=std::max(maxx,dots[i].x);miny=std::min(miny,dots[i].y);maxy=std::max(maxy,dots[i].y);}
    for(int y=std::max(0,(int)(miny/GRID));y<=std::min(GH-1,(int)(maxy/GRID));y++)for(int x=std::max(0,(int)(minx/GRID));x<=std::min(GW-1,(int)(maxx/GRID));x++){int n=y*GW+x;if(land[n]<0&&inside((x+.5f)*GRID,(y+.5f)*GRID,loop)){land[n]=player;gained++;}}
  } return gained*GRID*GRID;
}
int score(int p){return (int)std::count(land.begin(),land.end(),(signed char)p)*GRID*GRID;}
bool available(){for(int a=0;a<DOTS;a++)for(int b=a+1;b<DOTS;b++)if(legal(a,b))return true;return false;}
void reset(){dots.clear();edges.clear();history.clear();land.assign(GW*GH,-1);turn=0;selected=-1;ended=false;std::mt19937 rng((unsigned)GetTickCount());std::uniform_real_distribution<float>x(55,BW-55),y(55,BH-55);while((int)dots.size()<DOTS){Pt p{x(rng),y(rng)};bool ok=true;for(auto q:dots)if(dist(p,q)<72)ok=false;if(ok)dots.push_back(p);}}
void play(int a,int b){history.push_back({edges,land,turn});edges.push_back({a,b,turn});claim(a,b,turn);selected=-1;if(!available())ended=true;else turn=1-turn;}
void drawText(HDC h,int x,int y,const std::wstring& s,int size,COLORREF c,bool bold=false){HFONT f=CreateFontW(size,0,0,0,bold?800:400,0,0,0,DEFAULT_CHARSET,0,0,0,0,L"Malgun Gothic");SelectObject(h,f);SetTextColor(h,c);SetBkMode(h,TRANSPARENT);TextOutW(h,x,y,s.c_str(),(int)s.size());DeleteObject(f);}
void paint(HWND w,HDC dc){
  RECT rc;GetClientRect(w,&rc);HDC mem=CreateCompatibleDC(dc);HBITMAP bm=CreateCompatibleBitmap(dc,rc.right,rc.bottom);SelectObject(mem,bm);HBRUSH bg=CreateSolidBrush(RGB(7,17,31));FillRect(mem,&rc,bg);DeleteObject(bg);
  drawText(mem,15,12,L"DOTLAND",38,RGB(235,248,255),true);drawText(mem,210,26,L"1.44MB LOCAL 2P",13,RGB(145,180,204));
  std::wstring rs=L"RED  "+std::to_wstring(score(0))+L" ㎡",bs=L"BLUE  "+std::to_wstring(score(1))+L" ㎡";drawText(mem,15,57,rs,16,COL[0],true);drawText(mem,735,57,bs,16,COL[1],true);
  std::wstring msg=ended?(score(0)==score(1)?L"DRAW!":(score(0)>score(1)?L"RED WINS!":L"BLUE WINS!")):(turn?L"BLUE의 차례 — 점 두 개를 선택하세요":L"RED의 차례 — 점 두 개를 선택하세요");drawText(mem,290,57,msg,15,ended?RGB(255,234,150):COL[turn],true);
  HBRUSH board=CreateSolidBrush(RGB(16,40,58));RECT br{OX,OY,OX+BW,OY+BH};FillRect(mem,&br,board);DeleteObject(board);FrameRect(mem,&br,(HBRUSH)GetStockObject(WHITE_BRUSH));
  for(int gy=0;gy<GH;gy++)for(int gx=0;gx<GW;gx++){int o=land[gy*GW+gx];if(o>=0){HBRUSH b=CreateSolidBrush(o?RGB(21,82,105):RGB(112,37,54));RECT r{OX+gx*GRID,OY+gy*GRID,OX+(gx+1)*GRID,OY+(gy+1)*GRID};FillRect(mem,&r,b);DeleteObject(b);}}
  for(auto e:edges){HPEN p=CreatePen(PS_SOLID,5,COL[e.p]);SelectObject(mem,p);MoveToEx(mem,OX+(int)dots[e.a].x,OY+(int)dots[e.a].y,0);LineTo(mem,OX+(int)dots[e.b].x,OY+(int)dots[e.b].y);DeleteObject(p);}
  for(int i=0;i<DOTS;i++){HBRUSH b=CreateSolidBrush(i==selected?COL[turn]:RGB(238,250,255));SelectObject(mem,b);Ellipse(mem,OX+(int)dots[i].x-(i==selected?13:9),OY+(int)dots[i].y-(i==selected?13:9),OX+(int)dots[i].x+(i==selected?13:9),OY+(int)dots[i].y+(i==selected?13:9));DeleteObject(b);}
  drawText(mem,15,697,L"클릭: 점 연결    U: 한 수 무르기    N: 새 게임    · 점을 통과하거나 선을 교차할 수 없습니다.",13,RGB(163,190,207));
  BitBlt(dc,0,0,rc.right,rc.bottom,mem,0,0,SRCCOPY);DeleteObject(bm);DeleteDC(mem);
}
LRESULT CALLBACK proc(HWND w,UINT m,WPARAM wp,LPARAM lp){switch(m){case WM_CREATE:reset();return 0;case WM_PAINT:{PAINTSTRUCT ps;HDC h=BeginPaint(w,&ps);paint(w,h);EndPaint(w,&ps);return 0;}case WM_LBUTTONDOWN:{if(ended)break;int x=LOWORD(lp)-OX,y=HIWORD(lp)-OY;for(int i=0;i<DOTS;i++)if(dist(dots[i],{(float)x,(float)y})<19){if(selected<0)selected=i;else if(selected==i)selected=-1;else if(legal(selected,i))play(selected,i);else selected=i;InvalidateRect(w,0,FALSE);break;}return 0;}case WM_KEYDOWN:if(wp=='N'){reset();InvalidateRect(w,0,FALSE);}if(wp=='U'&&!history.empty()){auto s=history.back();history.pop_back();edges=s.e;land=s.land;turn=s.turn;selected=-1;ended=false;InvalidateRect(w,0,FALSE);}return 0;case WM_DESTROY:PostQuitMessage(0);return 0;}return DefWindowProcW(w,m,wp,lp);}
int WINAPI WinMain(HINSTANCE h,HINSTANCE,LPSTR,int){WNDCLASSW c{};c.hInstance=h;c.lpszClassName=L"Dotland";c.lpfnWndProc=proc;c.hCursor=LoadCursor(0,IDC_CROSS);c.hbrBackground=(HBRUSH)(COLOR_WINDOW+1);RegisterClassW(&c);HWND w=CreateWindowW(L"Dotland",L"DOTLAND — 1.44MB",WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX,120,80,950,760,0,0,h,0);ShowWindow(w,SW_SHOW);MSG m;while(GetMessage(&m,0,0,0)){TranslateMessage(&m);DispatchMessage(&m);}return 0;}
