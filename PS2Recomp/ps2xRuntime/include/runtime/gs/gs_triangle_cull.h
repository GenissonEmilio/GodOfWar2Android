#pragma once
#include "runtime/gs/gs_types.h"
#include <algorithm>
#include <cmath>

enum class GSTriangleCull { Keep, Outside, Degenerate };

// Match the positions sent to GL. The extra pixel is conservative at every
// supported internal scale, including the half-host-pixel centre adjustment.
inline GSTriangleCull gsTriangleCull(const GSDrawState &s, const std::array<GSVertex,3> &v)
{
    double x[3], y[3];
    const double ox=s.context.xyoffset.ofx/16.0, oy=s.context.xyoffset.ofy/16.0;
    for(unsigned i=0;i<3;++i){
        if(!std::isfinite(v[i].x) || !std::isfinite(v[i].y))return GSTriangleCull::Keep;
        x[i]=double(v[i].x)-ox;y[i]=double(v[i].y)-oy;
    }
    const auto &clip=s.context.scissor;
    if(clip.x0>clip.x1 || clip.y0>clip.y1 ||
       std::max({x[0],x[1],x[2]})<double(clip.x0)-1 ||
       std::min({x[0],x[1],x[2]})>double(clip.x1)+1 ||
       std::max({y[0],y[1],y[2]})<double(clip.y0)-1 ||
       std::min({y[0],y[1],y[2]})>double(clip.y1)+1)return GSTriangleCull::Outside;
    const double area=(x[1]-x[0])*(y[2]-y[0])-(x[2]-x[0])*(y[1]-y[0]);
    if(area!=0)return GSTriangleCull::Keep;
    // Real GS XY coordinates are unsigned 12.4. Restrict exact-area rejection
    // to that grid, where these double products/subtractions are exact. Other
    // synthetic/future callers retain the ordinary raster path.
    for(const auto &p:v)
        if(p.x<0 || p.x>4096 || p.y<0 || p.y>4096 ||
           std::trunc(double(p.x)*16)!=double(p.x)*16 ||
           std::trunc(double(p.y)*16)!=double(p.y)*16)return GSTriangleCull::Keep;
    return GSTriangleCull::Degenerate;
}
