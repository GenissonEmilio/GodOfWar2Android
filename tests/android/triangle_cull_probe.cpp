#include "runtime/gs/gs_triangle_cull.h"
#include <array>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <vector>
struct Point {double x,y;};
static std::vector<Point> clip(std::vector<Point> p,unsigned axis,double plane,bool lower){
    std::vector<Point> out;
    if(p.empty())return out;
    auto value=[&](Point a){return axis?a.y:a.x;};
    auto inside=[&](Point a){return lower?value(a)>=plane:value(a)<=plane;};
    Point a=p.back();bool ai=inside(a);
    for(Point b:p){bool bi=inside(b);
        if(ai!=bi){double f=(plane-value(a))/(value(b)-value(a));out.push_back({a.x+f*(b.x-a.x),a.y+f*(b.y-a.y)});}
        if(bi)out.push_back(b);a=b;ai=bi;
    }
    return out;
}
static double visibleArea(const GSDrawState &s,const std::array<GSVertex,3> &v){
    const auto &r=s.context.scissor;if(r.x0>r.x1 || r.y0>r.y1)return 0;
    std::vector<Point> p;for(auto a:v)p.push_back({a.x-s.context.xyoffset.ofx/16.0,a.y-s.context.xyoffset.ofy/16.0});
    p=clip(p,0,r.x0-.5,true);p=clip(p,0,r.x1+.5,false);
    p=clip(p,1,r.y0-.5,true);p=clip(p,1,r.y1+.5,false);
    double area=0;for(size_t i=1;i+1<p.size();++i)area+=(p[i].x-p[0].x)*(p[i+1].y-p[0].y)-(p[i+1].x-p[0].x)*(p[i].y-p[0].y);
    return std::abs(area);
}
int main(){
    uint32_t seed=0x31415926;auto random=[&](){seed^=seed<<13;seed^=seed>>17;seed^=seed<<5;return seed;};
    unsigned rejected=0;
    for(unsigned i=0;i<200000;++i){
        GSDrawState s{};s.context.xyoffset={uint16_t(random()),uint16_t(random())};
        auto &r=s.context.scissor;r={uint16_t(random()%1024),uint16_t(random()%2048),uint16_t(random()%1024),uint16_t(random()%2048)};
        std::array<GSVertex,3> v{};for(auto &p:v){p.x=(random()%65536)/16.0f;p.y=(random()%65536)/16.0f;}
        if(i%4==0)v[2]=v[1];
        auto c=gsTriangleCull(s,v);
        if(c!=GSTriangleCull::Keep){++rejected;if(visibleArea(s,v)>1e-7){std::fprintf(stderr,"Visible triangle rejected in case %u\n",i);return 1;}}
    }
    GSDrawState s{};s.context.scissor={16,31,16,31};std::array<GSVertex,3> v{};
    v[0].x=15.5;v[0].y=15.5;v[1].x=32.5;v[1].y=15.5;v[2].x=15.5;v[2].y=32.5;
    if(gsTriangleCull(s,v)!=GSTriangleCull::Keep)return 2;
    v[0].x=std::numeric_limits<float>::quiet_NaN();if(gsTriangleCull(s,v)!=GSTriangleCull::Keep)return 3;
    std::printf("200000 triangles checked against polygon clipping, plus scissor edge and NaN: PASS (%u conservative rejections)\n",rejected);
}
