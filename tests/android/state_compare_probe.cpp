#include "runtime/gs/gs_state_compare.h"
#include <cstddef>
#include <cstring>
#include <cstdio>
int main() {
    GSDrawState a{},b{};
    constexpr size_t padding=offsetof(GSFrameReg,psm)+sizeof(GSFrameReg::psm);
    static_assert(offsetof(GSFrameReg,fbmsk)>padding);
    reinterpret_cast<unsigned char *>(&b.context.frame)[padding]=0xA5;
    if(std::memcmp(&a,&b,sizeof(a))==0 || !gsSameDrawState(a,b))return 1;
    b.context.alpha=0x44;
    if(gsSameDrawState(a,b))return 2;
    b.context.alpha=0;
    b.prim.tme=true;
    if(gsSameDrawState(a,b))return 3;
    b.prim.tme=false;b.context.tex0.tbp0=77;b.context.tex1=0x55;
    b.context.clamp=0x123;b.context.miptbp1=0x44;b.texa.ta0=127;
    b.texclut.cov=31;b.textureWidth=64;b.linearFilter=true;
    if(gsSameDrawState(a,b) || !gsSameDrawState(a,b,true))return 4;
    a.prim.tme=b.prim.tme=true;
    if(gsSameDrawState(a,b,true))return 5;
    std::puts("Padding does not split equal render state; real blend/texture changes remain distinct");
    return 0;
}
