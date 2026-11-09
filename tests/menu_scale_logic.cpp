#include "../src/Gui/MenuScaleLogic.hpp"
#include <cassert>
#include <cmath>
using namespace MenuScaleLogic;
int main(){
 float scale=Resolve(100,1.f,1920,1080,920,600,false);assert(std::fabs(scale-1.5f)<0.001f);
 // Regression: dragging 150% down to 92% must not move/resize the slider under the pointer.
 for(int value:{90,70,42}){float next=Resolve(value,scale,1920,1080,920,600,true);assert(next==scale);}
 float next=Resolve(42,scale,1920,1080,920,600,false);assert(std::fabs(next-0.92f)<0.001f);
 float logicalX=200.f,logicalY=100.f,mouseX=400.f;
 const float screenX=logicalX*scale,screenY=logicalY*scale,screenMouse=mouseX*scale;
 float ratio=CoordinateRatio(scale,next);logicalX*=ratio;logicalY*=ratio;mouseX*=ratio;
 assert(std::fabs(logicalX*next-screenX)<0.001f&&std::fabs(logicalY*next-screenY)<0.001f&&std::fabs(mouseX*next-screenMouse)<0.001f);
 scale=next;
 for(int value:{0,100,50,42,100,0,50}){next=Resolve(value,scale,1920,1080,920,600,false);logicalX*=CoordinateRatio(scale,next);scale=next;assert(std::fabs(logicalX*scale-screenX)<0.01f);}
 float small=Resolve(100,1.5f,640,480,920,600,false);assert(920.f*small<=640.01f&&600.f*small<=480.01f);
 assert(Resolve(-20,1,1920,1080,920,600,false)==0.5f);
 assert(Resolve(200,1,1920,1080,920,600,false)==1.5f);
}

