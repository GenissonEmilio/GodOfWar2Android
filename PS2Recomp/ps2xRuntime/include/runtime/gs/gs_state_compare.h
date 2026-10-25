#pragma once
#include "runtime/gs/gs_types.h"
// Padding is not part of the GS state. The queue compares full snapshots;
// raster batches may also ignore texture fields when both draws disable TME.
inline bool gsSameDrawState(const GSDrawState &a,const GSDrawState &b,bool ignoreUnusedTextures=false) {
    const bool textureMatters=!ignoreUnusedTextures || a.prim.tme || b.prim.tme;
    return a.context.frame.fbp == b.context.frame.fbp
        && a.context.frame.fbw == b.context.frame.fbw
        && a.context.frame.psm == b.context.frame.psm
        && a.context.frame.fbmsk == b.context.frame.fbmsk
        && a.context.scissor.x0 == b.context.scissor.x0
        && a.context.scissor.x1 == b.context.scissor.x1
        && a.context.scissor.y0 == b.context.scissor.y0
        && a.context.scissor.y1 == b.context.scissor.y1
        && a.context.xyoffset.ofx == b.context.xyoffset.ofx
        && a.context.xyoffset.ofy == b.context.xyoffset.ofy
        && a.context.zbuf.zbp == b.context.zbuf.zbp
        && a.context.zbuf.psm == b.context.zbuf.psm
        && a.context.zbuf.zmask == b.context.zbuf.zmask
        && a.context.alpha == b.context.alpha
        && a.context.test == b.context.test
        && a.context.fba == b.context.fba
        && a.prim.type == b.prim.type
        && a.prim.iip == b.prim.iip
        && a.prim.tme == b.prim.tme
        && a.prim.fge == b.prim.fge
        && a.prim.abe == b.prim.abe
        && a.prim.aa1 == b.prim.aa1
        && a.prim.fst == b.prim.fst
        && a.prim.ctxt == b.prim.ctxt
        && a.prim.fix == b.prim.fix
        && a.pabe == b.pabe
        && a.scanmsk == b.scanmsk
        && a.dimx == b.dimx
        && a.dthe == b.dthe
        && a.colclamp == b.colclamp
        && a.fogR == b.fogR
        && a.fogG == b.fogG
        && a.fogB == b.fogB
        && (!textureMatters || (a.context.tex0.tbp0 == b.context.tex0.tbp0
            && a.context.tex0.tbw == b.context.tex0.tbw
            && a.context.tex0.psm == b.context.tex0.psm
            && a.context.tex0.tw == b.context.tex0.tw
            && a.context.tex0.th == b.context.tex0.th
            && a.context.tex0.tcc == b.context.tex0.tcc
            && a.context.tex0.tfx == b.context.tex0.tfx
            && a.context.tex0.cbp == b.context.tex0.cbp
            && a.context.tex0.cpsm == b.context.tex0.cpsm
            && a.context.tex0.csm == b.context.tex0.csm
            && a.context.tex0.csa == b.context.tex0.csa
            && a.context.tex0.cld == b.context.tex0.cld
            && a.context.tex1 == b.context.tex1
            && a.context.miptbp1 == b.context.miptbp1
            && a.context.miptbp2 == b.context.miptbp2
            && a.context.clamp == b.context.clamp
            && a.texa.ta0 == b.texa.ta0
            && a.texa.aem == b.texa.aem
            && a.texa.ta1 == b.texa.ta1
            && a.texclut.cbw == b.texclut.cbw
            && a.texclut.cou == b.texclut.cou
            && a.texclut.cov == b.texclut.cov
            && a.textureWidth == b.textureWidth
            && a.textureHeight == b.textureHeight
            && a.linearFilter == b.linearFilter));
}
