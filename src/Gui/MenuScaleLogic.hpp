#pragma once
#include <algorithm>
#include <cmath>
namespace MenuScaleLogic {
inline float Resolve(int setting,float current,float width,float height,float baseWidth,float baseHeight,bool dragging){
 float requested=0.5f+static_cast<float>((std::clamp)(setting,0,100))*0.01f;
 float fit=1.5f;
 if(width>0&&height>0&&baseWidth>0&&baseHeight>0)
  fit=(std::max)(0.1f,(std::min)(width/baseWidth,height/baseHeight));
 // A drag uses one fixed coordinate system until the button is released.
 if(dragging&&std::isfinite(current)&&current>0)return (std::min)(current,fit);
 return (std::min)(requested,fit);
}
inline float CoordinateRatio(float previous,float next){return previous/next;}
}

