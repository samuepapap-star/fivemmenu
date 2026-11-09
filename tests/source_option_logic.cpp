#include "../src/Core/Features/SourceOptionLogic.hpp"
#include <cassert>
#include <limits>
int main() {
    using namespace SourceOptionLogic;
    assert(DistanceLimit(200,true,true,80.9f)==80);
    assert(DistanceLimit(50,true,true,100.f)==50); // Never exceed the user's cap.
    assert(DistanceLimit(200,false,true,20.f)==200);
    assert(DistanceLimit(200,true,false,20.f)==200); // A failed read is not zero range.
    for(float value:{0.f,-1.f,0.5f,10000.f,std::numeric_limits<float>::infinity(),
        std::numeric_limits<float>::quiet_NaN()})
        assert(DistanceLimit(200,true,true,value)==200);
    assert(DistanceLimit(0,true,false,0)==1);
    assert(!KnownWeaponRangeLayout(3889));
    assert(KnownWeaponRangeLayout(2802));
    assert(CameraDistance(-100.f)==-5.f);
    assert(std::abs(CameraDistance(50.f)+2.f)<0.0001f);
    assert(std::abs(CameraDistance(1000.f)-1.f)<0.0001f);
    assert(std::abs(CameraDistance(std::numeric_limits<float>::quiet_NaN())-1.f)<0.0001f);
}
