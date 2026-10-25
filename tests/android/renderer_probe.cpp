// Original synthetic input only. Exercises the production GLES GS backend.
#include "runtime/gs/gs_hw_backend.h"
#include "runtime/gs/gs_shared_present.h"
#include "runtime/gs/ps2_gs_memory.h"
#include <EGL/egl.h>
#include <GLES3/gl32.h>
#include <vector>
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <chrono>
static bool pixel(const PresentationFrame &frame,int r,int g,int b,unsigned x=16,unsigned y=16) {
    if(frame.width<32 || frame.height<32 || frame.pixels.size()<frame.width*frame.height*4u)return false;
    size_t at=(std::max(frame.width,640u)*y+x)*4u;
    std::printf("pixel %u,%u,%u expected %d,%d,%d; %ux%u\n",frame.pixels[at],frame.pixels[at+1],frame.pixels[at+2],r,g,b,frame.width,frame.height);
    return std::abs(int(frame.pixels[at])-r)<=2 && std::abs(int(frame.pixels[at+1])-g)<=2 && std::abs(int(frame.pixels[at+2])-b)<=2;
}
static int runEpoch(int epoch){
    std::printf("EGL lifetime %d\n",epoch);
    setenv("PS2X_GS_HW_SCALE","1",1);
    EGLDisplay display=eglGetDisplay(EGL_DEFAULT_DISPLAY);EGLint count=0;
    if(!eglInitialize(display,nullptr,nullptr))return 10;
    EGLConfig config;const EGLint attrs[]{EGL_SURFACE_TYPE,EGL_PBUFFER_BIT,EGL_RENDERABLE_TYPE,EGL_OPENGL_ES3_BIT,EGL_RED_SIZE,8,EGL_GREEN_SIZE,8,EGL_BLUE_SIZE,8,EGL_NONE};
    if(!eglChooseConfig(display,attrs,&config,1,&count)||!count)return 11;
    const EGLint size[]{EGL_WIDTH,64,EGL_HEIGHT,64,EGL_NONE},version[]{EGL_CONTEXT_CLIENT_VERSION,3,EGL_NONE};
    EGLSurface surface=eglCreatePbufferSurface(display,config,size);EGLContext ctx=eglCreateContext(display,config,EGL_NO_CONTEXT,version);
    if(!eglMakeCurrent(display,surface,surface,ctx))return 12;
    std::printf("%s / %s\n",glGetString(GL_RENDERER),glGetString(GL_VERSION));
    GSSharedPresent::CaptureHostContext();
    std::vector<uint8_t> vram(4u*1024u*1024u);bool ok=true;
    { GSHwBackend backend;backend.Initialize(vram.data(),vram.size());
    if(!backend.UsesSharedPresentation())return 13;
    GSPrimitiveBatch batch;batch.vertexCount=2;batch.state.prim.type=GS_PRIM_SPRITE;batch.state.colclamp=1;
    batch.state.context.frame.fbw=1;batch.state.context.scissor={0,63,0,63};batch.state.context.zbuf.zbp=64;batch.state.context.zbuf.zmask=true;
    batch.vertices[0].x=0;batch.vertices[0].y=0;batch.vertices[1].x=64;batch.vertices[1].y=64;
    for(auto &v:batch.vertices){v.r=200;v.g=100;v.b=40;v.a=128;}
    backend.Submit(batch);backend.Flush();
    {unsigned char px[4]{};glReadPixels(16,16,1,1,GL_RGBA,GL_UNSIGNED_BYTE,px);printf("raw draw pixel %u,%u,%u fbo error %x\n",px[0],px[1],px[2],glGetError());}
    GSPresentationRequest request;request.pmode=1;request.dispfb1=1ULL<<9;request.display1=(63ULL<<32)|(63ULL<<44);request.forceReadback=true;
    auto frame=backend.Present(request);
    size_t nonzero=0;for(size_t i=0;i<frame.pixels.size();i+=4)nonzero+=frame.pixels[i]>0;
    printf("nonzero red %zu/%zu\n",nonzero,frame.pixels.size()/4);
    ok=pixel(frame,200,100,40);
    batch.state.prim.abe=true;batch.state.context.alpha=0 | (1ULL<<2) | (0ULL<<4) | (1ULL<<6);
    for(auto &v:batch.vertices){v.r=100;v.g=200;v.b=80;v.a=64;}
    backend.Submit(batch);backend.Flush();
    ok=pixel(backend.Present(request),150,150,60)&&ok;
    GSPrimitiveBatch shuffle=batch;shuffle.state.prim.abe=false;shuffle.state.prim.tme=true;shuffle.state.prim.fst=true;
    shuffle.state.context.frame.psm=2;shuffle.state.context.tex0.psm=2;shuffle.state.context.tex0.tbw=1;
    shuffle.state.context.tex0.tfx=1;shuffle.state.textureWidth=64;shuffle.state.textureHeight=64;shuffle.state.texa.ta1=128;
    shuffle.vertices[0].x=8;shuffle.vertices[0].y=0;shuffle.vertices[1].x=16;shuffle.vertices[1].y=8;
    shuffle.vertices[0].u=0;shuffle.vertices[0].v=0;shuffle.vertices[1].u=128;shuffle.vertices[1].v=128;
    backend.Submit(shuffle);backend.Flush();
    ok=pixel(backend.Present(request),150,150,150,4,4)&&ok;
    request.forceReadback=false;backend.Present(request);glFinish();
    GSSharedPresent::Frame shared;
    if(!GSSharedPresent::Acquire(shared) || !shared.texture)ok=false;
    else {
        GLuint fbo=0;glGenFramebuffers(1,&fbo);glBindFramebuffer(GL_FRAMEBUFFER,fbo);
        glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,shared.texture,0);
        unsigned char px[4]{};glReadPixels(4,4,1,1,GL_RGBA,GL_UNSIGNED_BYTE,px);
        std::printf("shared pixel %u,%u,%u\n",px[0],px[1],px[2]);
        for(int i=0;i<3;++i)ok=std::abs(int(px[i])-150)<=2 && ok;
        glDeleteFramebuffers(1,&fbo);
    }
    backend.Present(request);glFinish();GSSharedPresent::Frame repeated;
    if(!GSSharedPresent::Acquire(repeated) || repeated.sequence!=shared.sequence)ok=false;
    request.pmode=0;backend.Present(request);glFinish();GSSharedPresent::Frame blank;
    if(!GSSharedPresent::Acquire(blank) || !blank.blank)ok=false;
    request.pmode=1;backend.Present(request);glFinish();GSSharedPresent::Frame restored;
    if(!GSSharedPresent::Acquire(restored) || restored.blank || restored.sequence<=blank.sequence)ok=false;
    if(!backend.ClearFramebuffer(batch.state.context,0x8038220c))ok=false;
    backend.Present(request);glFinish();GSSharedPresent::Frame cleared;
    if(!GSSharedPresent::Acquire(cleared) || cleared.sequence<=restored.sequence || !cleared.texture)ok=false;
    else{
        GLuint fbo=0;glGenFramebuffers(1,&fbo);glBindFramebuffer(GL_FRAMEBUFFER,fbo);
        glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,cleared.texture,0);
        uint8_t px[4]{};glReadPixels(4,4,1,1,GL_RGBA,GL_UNSIGNED_BYTE,px);
        ok=std::abs(int(px[0])-12)<=1 && std::abs(int(px[1])-34)<=1 && std::abs(int(px[2])-56)<=1 && ok;
        glDeleteFramebuffers(1,&fbo);
    }
    GSPrimitiveBatch update=batch;update.state.prim.abe=false;
    for(auto &v:update.vertices){v.r=90;v.g=80;v.b=70;v.a=128;}
    backend.Submit(update);backend.Flush();backend.Present(request);glFinish();
    GSSharedPresent::Frame updated;
    if(!GSSharedPresent::Acquire(updated) || updated.sequence<=cleared.sequence)ok=false;
    // Byte vertex attributes must retain the unnormalized 0..255 GS range,
    // including the alpha channel and values on either side of 128.
    constexpr uint8_t colors[]{0,1,127,128,254,255};
    batch.state.prim.abe=false;
    for(unsigned i=0;i<6;++i){
        const uint8_t expected[]{colors[i],colors[(i+2)%6],colors[(i+3)%6],colors[(i+4)%6]};
        for(auto &v:batch.vertices){v.r=expected[0];v.g=expected[1];v.b=expected[2];v.a=expected[3];}
        backend.Submit(batch);backend.Flush();
        uint8_t actual[4]{};glReadPixels(16,16,1,1,GL_RGBA,GL_UNSIGNED_BYTE,actual);
        for(unsigned ch=0;ch<4;++ch)ok=std::abs(int(actual[ch])-expected[ch])<=1 && ok;
        std::printf("vertex RGBA %u,%u,%u,%u expected %u,%u,%u,%u\n",actual[0],actual[1],actual[2],actual[3],expected[0],expected[1],expected[2],expected[3]);
    }
    const auto start=std::chrono::steady_clock::now();
    for(unsigned i=0;i<1000;++i){
        batch.state.context.tex0.tbp0=i;batch.state.context.clamp=i%4;
        batch.state.textureWidth=1+(i%64);batch.state.linearFilter=(i&1)!=0;
        backend.Submit(batch);
        GSTex0Reg noop{};noop.cld=1; // A direct-color TEX0 cannot load a CLUT.
        backend.LoadClut(noop,{});
        noop.psm=GS_PSM_T8;noop.cld=0;backend.LoadClut(noop,{});
        GSMem::WriteCT32(vram.data(),0x3000,1,0,0,0x80000000u|i);
        noop.cld=1;noop.cbp=0x3000;noop.cpsm=GS_PSM_CT32;backend.LoadClut(noop,{});
    }
    backend.Flush();
    uint8_t batched[4]{};glReadPixels(16,16,1,1,GL_RGBA,GL_UNSIGNED_BYTE,batched);
    ok=batched[0]==255 && batched[1]==1 && batched[2]==127 && batched[3]==128 && ok;
    std::printf("1000 untextured sprites with texture-state/palette changes: %.3f ms, RGBA %u,%u,%u,%u\n",std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count(),batched[0],batched[1],batched[2],batched[3]);
    batch.state.context.tex0={};batch.state.context.clamp=0;batch.state.linearFilter=false;
    batch.state.prim.tme=true;batch.state.prim.fst=true;
    auto &tex0=batch.state.context.tex0;
    tex0.tbp0=0x2000;tex0.tbw=1;tex0.psm=GS_PSM_T8;tex0.tw=6;tex0.th=6;
    tex0.tcc=1;tex0.tfx=1;tex0.cbp=0x3000;tex0.cpsm=GS_PSM_CT32;tex0.cld=1;
    batch.state.textureWidth=64;batch.state.textureHeight=64;
    batch.vertices[0].u=0;batch.vertices[0].v=0;batch.vertices[1].u=1024;batch.vertices[1].v=1024;
    // Index memory is zero. CLUT changes must leave the pending left half red
    // before the right half is drawn using the newly loaded blue palette.
    GSMem::WriteCT32(vram.data(),0x3000,1,0,0,0x800000c8);
    backend.LoadClut(tex0,{});
    batch.vertices[0].x=0;batch.vertices[1].x=32;backend.Submit(batch);
    backend.LoadClut(tex0,{}); // Identical reload: keep the previous batch.
    GSMem::WriteCT32(vram.data(),0x3000,1,0,0,0x80c80000);
    backend.LoadClut(tex0,{});
    batch.vertices[0].x=32;batch.vertices[1].x=64;backend.Submit(batch);backend.Flush();
    request.forceReadback=true;auto palettes=backend.Present(request);
    ok=pixel(palettes,200,0,0,16,16)&&ok;
    ok=pixel(palettes,0,0,200,48,16)&&ok;
    // The CLUT may itself be supplied by a not-yet-submitted colour draw.
    batch.state.prim.tme=false;batch.vertices[0].x=0;batch.vertices[1].x=64;
    for(auto &v:batch.vertices){v.r=20;v.g=40;v.b=60;v.a=128;}
    backend.Submit(batch);
    tex0.cbp=0;backend.LoadClut(tex0,{});
    batch.state.context.frame.fbp=8;batch.state.prim.tme=true;backend.Submit(batch);backend.Flush();
    request.dispfb1=8|(1ULL<<9);
    ok=pixel(backend.Present(request),20,40,60)&&ok;
    // A narrow FRAME can reuse an existing wider render target. A CSM2 palette
    // on row 32 still depends on the pending draw across the owner's width.
    batch.state.prim.tme=false;batch.state.context.frame.fbw=4;
    for(auto &v:batch.vertices){v.r=80;v.g=100;v.b=120;}
    backend.Submit(batch);backend.Flush();
    batch.state.context.frame.fbw=1;
    for(auto &v:batch.vertices){v.r=30;v.g=50;v.b=70;}
    backend.Submit(batch);
    tex0.cbp=8*32;tex0.csm=1;
    GSTexClutReg csm2{};csm2.cbw=4;csm2.cov=32;
    backend.LoadClut(tex0,csm2);
    batch.state.context.frame.fbp=16;batch.state.prim.tme=true;batch.state.texclut=csm2;
    backend.Submit(batch);backend.Flush();request.dispfb1=16|(1ULL<<9);
    ok=pixel(backend.Present(request),30,50,70)&&ok;
    // Reusing immutable state must still submit changing vertices/colours.
    // A legacy (untagged) call between tagged calls must invalidate the shortcut.
    batch.state.prim.tme=false;batch.state.prim.abe=false;batch.state.context.frame.fbmsk=0;
    const uint64_t stateToken=gsNewDrawStateToken();
    batch.vertices[0].x=0;batch.vertices[1].x=32;
    for(auto &v:batch.vertices){v.r=200;v.g=0;v.b=0;}
    backend.SubmitCached(batch,stateToken);
    auto masked=batch;masked.state.context.frame.fbmsk=0x00ffffff;
    masked.vertices[0].x=32;masked.vertices[1].x=64;
    backend.Submit(masked);
    batch.vertices[0].x=32;batch.vertices[1].x=64;
    for(auto &v:batch.vertices){v.r=0;v.g=0;v.b=200;}
    backend.SubmitCached(batch,stateToken);backend.Flush();
    auto tagged=backend.Present(request);
    ok=pixel(tagged,200,0,0,16,16)&&pixel(tagged,0,0,200,48,16)&&ok;
    batch.state.context.frame.fbmsk=0xffff00ff;
    for(auto &v:batch.vertices)v.g=220;
    backend.SubmitCached(batch,gsNewDrawStateToken());backend.Flush();
    ok=pixel(backend.Present(request),0,220,200,48,16)&&ok;
    batch.state.context.frame.fbmsk=0;
    // GoW's depth inversion pass marks a Z buffer as inverted without changing
    // the host depth texture. Test both comparison directions and restoration.
    batch.vertices[0].x=0;batch.vertices[1].x=64;
    batch.state.context.zbuf.zmask=false;batch.state.context.zbuf.psm=GS_PSM_Z32;
    batch.state.context.test=(1ULL<<16)|(1ULL<<17);
    for(auto &v:batch.vertices){v.r=200;v.g=0;v.b=0;v.z=0xbfffffff;}
    backend.Submit(batch);backend.Flush();
    auto inversion=batch;inversion.state.context.frame.fbp=64;inversion.state.context.frame.psm=GS_PSM_CT24;
    inversion.state.context.zbuf.zmask=true;inversion.state.context.test=0;
    inversion.state.prim.abe=true;inversion.state.context.alpha=0x80000000a4ULL;
    for(auto &v:inversion.vertices){v.r=255;v.g=255;v.b=255;}
    backend.Submit(inversion);backend.Flush();
    batch.state.context.zbuf.zmask=true;batch.state.context.test=(1ULL<<16)|(2ULL<<17);
    for(auto &v:batch.vertices){v.r=0;v.g=0;v.b=200;v.z=0x7fffffff;}
    backend.Submit(batch);backend.Flush();ok=pixel(backend.Present(request),0,0,200)&&ok;
    for(auto &v:batch.vertices){v.r=200;v.b=0;v.z=0;}
    backend.Submit(batch);backend.Flush();ok=pixel(backend.Present(request),0,0,200)&&ok;
    for(auto &v:batch.vertices){v.r=0;v.g=200;v.b=0;v.z=0xffffffff;}
    backend.Submit(batch);backend.Flush();ok=pixel(backend.Present(request),0,200,0)&&ok;
    backend.Submit(inversion);backend.Flush(); // Restore normal depth comparison.
    for(auto &v:batch.vertices){v.r=200;v.g=200;v.b=0;v.z=0x7fffffff;}
    backend.Submit(batch);backend.Flush();ok=pixel(backend.Present(request),0,200,0)&&ok;
    for(auto &v:batch.vertices)v.z=0xffffffff;
    backend.Submit(batch);backend.Flush();ok=pixel(backend.Present(request),200,200,0)&&ok;
    batch.state.context.test=0;
    GSPrimitiveBatch visible=batch;visible.vertexCount=3;visible.state.prim.type=GS_PRIM_TRIANGLE;
    visible.vertices[0].x=0;visible.vertices[0].y=0;
    visible.vertices[1].x=64;visible.vertices[1].y=0;
    visible.vertices[2].x=0;visible.vertices[2].y=64;
    for(auto &v:visible.vertices){v.r=200;v.g=0;v.b=0;}
    backend.Submit(visible);backend.Flush();
    auto visibleFrame=backend.Present(request);
    ok=pixel(visibleFrame,200,0,0,16,16)&&pixel(visibleFrame,200,200,0,48,48)&&ok;
    auto outside=visible;for(auto &v:outside.vertices){v.x+=80;v.r=v.g=v.b=255;}
    backend.Submit(outside);backend.Flush();ok=pixel(backend.Present(request),200,0,0)&&ok;
    auto degenerate=visible;degenerate.vertices[1]=degenerate.vertices[0];
    for(auto &v:degenerate.vertices)v.r=v.g=v.b=255;
    backend.Submit(degenerate);backend.Flush();ok=pixel(backend.Present(request),200,0,0)&&ok;
    visible.state.context.scissor={16,31,16,31};visible.state.prim.tme=true;visible.state.prim.fst=true;
    visible.state.context.tex0={};visible.state.context.tex0.tbp0=0x3100;visible.state.context.tex0.tbw=1;
    visible.state.context.tex0.psm=GS_PSM_CT32;visible.state.context.tex0.tfx=1;visible.state.context.tex0.tcc=1;
    visible.state.textureWidth=visible.state.textureHeight=1;
    backend.WriteVram(GS_PSM_CT32,0x3100,1,0,0,0x80825a28);
    visible.vertices[0].x=15.5;visible.vertices[0].y=15.5;
    visible.vertices[1].x=32.5;visible.vertices[1].y=15.5;
    visible.vertices[2].x=15.5;visible.vertices[2].y=32.5;
    for(auto &v:visible.vertices)v.u=v.v=0;
    backend.Submit(visible);backend.Flush();auto edge=backend.Present(request);
    ok=pixel(edge,40,90,130,16,16)&&pixel(edge,200,0,0,30,30)&&ok;
    // New colour/depth targets must be initialized even if the previous pass
    // leaves a channel mask, a narrow scissor and disabled depth writes.
    auto maskedSeed=batch;maskedSeed.state.context.frame.fbmsk=0xffff00ff;
    maskedSeed.state.context.scissor={16,31,16,31};
    backend.Submit(maskedSeed);backend.Flush();
    GLboolean writeMask[4]{};glGetBooleanv(GL_COLOR_WRITEMASK,writeMask);
    if(writeMask[0] || !writeMask[1] || writeMask[2] || writeMask[3])ok=false;
    auto fresh=maskedSeed;fresh.state.context.frame.fbp=24;fresh.state.context.frame.fbmsk=0;
    fresh.state.context.test=(1ULL<<16)|(2ULL<<17);
    for(auto &v:fresh.vertices){v.r=40;v.g=90;v.b=130;v.z=0x7fffffff;}
    backend.Submit(fresh);backend.Flush();request.dispfb1=24|(1ULL<<9);
    auto initialized=backend.Present(request);
    ok=pixel(initialized,40,90,130,16,16)&&pixel(initialized,0,0,0,48,48)&&ok;
    request.dispfb1=16|(1ULL<<9);
    // Negative repeating UVs must retain floor-modulo semantics, including all
    // four neighbours used by bilinear filtering across a palette seam.
    auto repeat=batch;repeat.state.prim.tme=true;repeat.state.prim.fst=false;
    repeat.state.context.tex0={};auto &repeatTex=repeat.state.context.tex0;
    repeatTex.tbp0=0x3200;repeatTex.tbw=1;repeatTex.psm=GS_PSM_T8;repeatTex.tw=repeatTex.th=1;
    repeatTex.tcc=1;repeatTex.tfx=1;repeatTex.cbp=0x3300;repeatTex.cpsm=GS_PSM_CT32;repeatTex.cld=1;
    repeat.state.textureWidth=repeat.state.textureHeight=2;repeat.state.context.clamp=0;
    const uint32_t repeatColors[]{0x800000c8,0x8000c800,0x80c80000,0x8000c8c8};
    for(unsigned i=0;i<4;++i){
        backend.WriteVram(GS_PSM_CT32,0x3300,1,i,0,repeatColors[i]);
        backend.WriteVram(GS_PSM_T8,0x3200,1,i%2,i/2,i);
    }
    backend.LoadClut(repeatTex,{});
    repeat.vertices[0].s=-8.125;repeat.vertices[0].t=-6.125;
    repeat.vertices[1].s=23.875;repeat.vertices[1].t=25.875;
    for(auto &v:repeat.vertices)v.q=1;
    repeat.state.linearFilter=false;backend.Submit(repeat);backend.Flush();
    std::vector<uint8_t> rawRepeat(64*64*4);
    glReadPixels(0,0,64,64,GL_RGBA,GL_UNSIGNED_BYTE,rawRepeat.data());
    auto repeatFrame=backend.Present(request);
    ok=pixel(repeatFrame,200,200,0,16,16)&&ok;
    repeat.state.linearFilter=true;backend.Submit(repeat);backend.Flush();
    std::vector<uint8_t> rawFiltered(64*64*4);
    glReadPixels(0,0,64,64,GL_RGBA,GL_UNSIGNED_BYTE,rawFiltered.data());
    auto filtered=backend.Present(request);
    ok=pixel(filtered,125,150,38,16,16)&&pixel(filtered,75,50,113,17,16)&&ok;
    // Validate the raw target; presentation deliberately repeats its overscan
    // border and therefore cannot serve as an oracle for these edge texels.
    const auto checkRepeated=[&](const std::vector<uint8_t> &pixels,bool linear){
        for(unsigned y=0;y<64;++y)for(unsigned x=0;x<64;++x){
            const double u=-16.25+x-(linear?.5:0),v=-12.25+y-(linear?.5:0);
            const int ix=int(std::floor(u)),iy=int(std::floor(v));
            const double wx=linear?u-ix:0,wy=linear?v-iy:0;
            auto at=[&](int dx,int dy,unsigned ch){return (repeatColors[((iy+dy)%2+2)%2*2+((ix+dx)%2+2)%2]>>(8*ch))&255;};
            for(unsigned ch=0;ch<3;++ch){
                const double expected=(at(0,0,ch)*(1-wx)+at(1,0,ch)*wx)*(1-wy)+(at(0,1,ch)*(1-wx)+at(1,1,ch)*wx)*wy;
                const auto actual=pixels[(y*64+x)*4+ch];
                if(std::abs(double(actual)-expected)>2){std::fprintf(stderr,"Repeating palette mismatch %u,%u channel %u: %u expected %.3f\n",x,y,ch,actual,expected);return false;}
            }
        }
        return true;
    };
    ok=checkRepeated(rawRepeat,false)&&checkRepeated(rawFiltered,true)&&ok;
    auto counting=batch;counting.state.context.frame.fbp=32;
    counting.state.context.frame.fbmsk=0xffff00ff;counting.state.context.test=0;
    counting.state.prim.abe=true;counting.state.colclamp=0;
    counting.state.context.alpha=0x8000000068ULL;
    backend.ClearFramebuffer(counting.state.context,0x807f7f7f);
    for(auto &v:counting.vertices){v.r=0;v.g=1;v.b=0;v.a=128;}
    for(unsigned i=0;i<5;++i)backend.Submit(counting);backend.Flush();
    for(auto &v:counting.vertices)v.g=255;
    for(unsigned i=0;i<3;++i)backend.Submit(counting);backend.Flush();
    request.dispfb1=32|(1ULL<<9);
    ok=pixel(backend.Present(request),127,129,127)&&ok;
    counting.state.context.test=1; // Alpha NEVER must retain the general discard path.
    backend.Submit(counting);backend.Flush();ok=pixel(backend.Present(request),127,129,127)&&ok;
    counting.state.context.test=1ULL<<14; // DATE=1, DATM=0 rejects stored alpha >=128.
    backend.Submit(counting);backend.Flush();ok=pixel(backend.Present(request),127,129,127)&&ok;
    // Switch between ordinary textured draws, alpha-fail passes and reads of
    // destination alpha/colour. Each program has its own uniform locations.
    auto composite=repeat;composite.state.context.frame.fbp=40;
    composite.state.context.frame.fbmsk=0;composite.state.context.test=0;
    composite.state.context.tex0={};composite.state.context.tex0.tbp0=0x3500;
    composite.state.context.tex0.tbw=1;composite.state.context.tex0.psm=GS_PSM_CT32;
    composite.state.context.tex0.tfx=1;composite.state.context.tex0.tcc=1;
    composite.state.textureWidth=composite.state.textureHeight=1;composite.state.linearFilter=false;
    for(auto &v:composite.vertices){v.s=v.t=0;v.q=1;}
    backend.WriteVram(GS_PSM_CT32,0x3500,1,0,0,0x40c8a064);
    request.dispfb1=40|(1ULL<<9);
    backend.ClearFramebuffer(composite.state.context,0x80402810);
    composite.state.context.test=1|(6ULL<<1)|(100ULL<<4)|(3ULL<<12); // Failed alpha writes RGB, retains alpha.
    backend.Submit(composite);backend.Flush();
    ok=pixel(backend.Present(request),100,160,200)&&ok;
    uint8_t compositePixel[4]{};
    // Present changes the bound FBO, so sample alpha before presentation on
    // the next draw, alongside a rejected DATE test.
    composite.state.context.test=1ULL<<14;
    backend.Submit(composite);backend.Flush();glReadPixels(16,16,1,1,GL_RGBA,GL_UNSIGNED_BYTE,compositePixel);
    ok=compositePixel[3]==128 && pixel(backend.Present(request),100,160,200)&&ok;
    composite.state.context.test=(1ULL<<14)|(1ULL<<15); // Accept alpha >=128, replace it with texture alpha 64.
    backend.Submit(composite);backend.Flush();glReadPixels(16,16,1,1,GL_RGBA,GL_UNSIGNED_BYTE,compositePixel);
    ok=compositePixel[3]==64 && pixel(backend.Present(request),100,160,200)&&ok;
    composite.state.context.test=0;composite.state.prim.abe=true;composite.state.context.alpha=0x54;
    backend.ClearFramebuffer(composite.state.context,0x403c2814);
    backend.Submit(composite);backend.Flush();ok=pixel(backend.Present(request),60,100,130)&&ok;
    composite.state.prim.abe=false;
    backend.Submit(composite);backend.Flush();ok=pixel(backend.Present(request),100,160,200)&&ok;
    // Independent GS equation oracle across the 128 boundary. Changing the
    // texel/CLUT alpha must invalidate its bound, including every TFX/TCC mode.
    auto dual=composite;dual.state.context.frame.fbp=56;dual.state.context.test=0;
    dual.state.prim.abe=true;dual.state.context.alpha=0x44;
    const int background[]{32,80,120},textureRgb[]{200,100,40},vertexRgb[]{96,128,160};
    unsigned blendCases=0;
    const auto uploadPixel=[&](unsigned psm,unsigned base,uint32_t value){
        GSTransferCommand transfer{};transfer.direction=0;transfer.bitbltbuf.dpsm=psm;
        transfer.bitbltbuf.dbp=base;transfer.bitbltbuf.dbw=1;transfer.trxreg.rrw=transfer.trxreg.rrh=1;
        backend.BeginTransfer(transfer);backend.UploadImage(reinterpret_cast<const uint8_t *>(&value),psm==GS_PSM_T8?1:4);
    };
    for(bool fixed:{false,true})for(bool paletted:{false,true})for(unsigned tfx=0;tfx<4;++tfx)for(unsigned tcc=0;tcc<2;++tcc)
    for(unsigned ta:{0u,64u,128u,129u,255u})for(unsigned va:{0u,64u,128u,255u}){
        dual.state.context.alpha=fixed?((uint64_t(ta)<<32)|0x64):0x44;
        auto &t=dual.state.context.tex0;t={};t.tbp0=0x3600;t.tbw=1;t.tfx=tfx;t.tcc=tcc;
        const uint32_t texel=(ta<<24)|0x2864c8;
        if(paletted){
            t.psm=GS_PSM_T8;t.cbp=0x3700;t.cpsm=GS_PSM_CT32;t.cld=1;
            uploadPixel(GS_PSM_T8,t.tbp0,0);
            uploadPixel(GS_PSM_CT32,t.cbp,texel);backend.LoadClut(t,{});
        }else{t.psm=GS_PSM_CT32;uploadPixel(GS_PSM_CT32,t.tbp0,texel);}
        for(auto &v:dual.vertices){v.r=96;v.g=128;v.b=160;v.a=va;}
        const unsigned a=!tcc?va:tfx==0?std::min(255u,ta*va/128u):tfx==2?std::min(255u,ta+va):ta;
        backend.ClearFramebuffer(dual.state.context,0x5a785020);backend.Submit(dual);backend.Flush();
        uint8_t actual[4]{};glReadPixels(16,16,1,1,GL_RGBA,GL_UNSIGNED_BYTE,actual);
        for(unsigned ch=0;ch<4;++ch){
            int expected=a;
            if(ch<3){
                int source=tfx==1?textureRgb[ch]:std::min(255,textureRgb[ch]*vertexRgb[ch]/128+int(tfx>=2?va:0));
                expected=std::clamp(int(std::floor(double(source-background[ch])*(fixed?ta:a)/128.0))+background[ch],0,255);
            }
            if(std::abs(int(actual[ch])-expected)>2){
                std::fprintf(stderr,"Blend mismatch fixed=%d pal=%d TFX=%u TCC=%u ta=%u va=%u ch=%u: %u expected %d\n",fixed,paletted,tfx,tcc,ta,va,ch,actual[ch],expected);ok=false;
            }
        }
        ++blendCases;
    }
    std::printf("GS source-alpha equation oracle: %u cases\n",blendCases);
    auto alias=composite;alias.state.prim.tme=false;alias.state.context.frame.fbp=48;
    alias.state.context.zbuf.zbp=48;alias.state.context.zbuf.psm=GS_PSM_Z24;alias.state.context.zbuf.zmask=true;
    alias.state.context.test=(1ULL<<16)|(2ULL<<17);
    for(auto &v:alias.vertices){v.r=40;v.g=60;v.b=80;v.a=128;v.z=0x200000;}
    request.dispfb1=48|(1ULL<<9);backend.ClearFramebuffer(alias.state.context,0x80400000);
    backend.Submit(alias);backend.Flush();ok=pixel(backend.Present(request),0,0,64)&&ok;
    for(auto &v:alias.vertices)v.z=0x600000;
    backend.Submit(alias);backend.Flush();ok=pixel(backend.Present(request),40,60,80)&&ok;
    alias.state.context.zbuf.zmask=false;
    for(auto &v:alias.vertices)v.z=0x700000;
    backend.Submit(alias);backend.Flush();ok=pixel(backend.Present(request),0,0,112)&&ok;
    // Reuse each ring region while earlier draws are still queued. Read back
    // once after all wraps; every cell must retain its last submitted colour.
    auto streamed=batch;streamed.state.context.frame.fbp=60;
    streamed.state.context.frame.fbmsk=0;streamed.state.context.test=0;
    streamed.state.prim.tme=false;streamed.state.prim.abe=false;
    for(unsigned i=0;i<2048;++i){
        const unsigned cell=i%64,x=cell%8,y=cell/8;
        streamed.vertices[0].x=x*8;streamed.vertices[0].y=y*8;
        streamed.vertices[1].x=x*8+8;streamed.vertices[1].y=y*8+8;
        for(auto &v:streamed.vertices){v.r=i%251;v.g=(i*7)%253;v.b=(i*13)%255;v.a=128;}
        backend.Submit(streamed);backend.Flush();
    }
    std::vector<uint8_t> streamPixels(64*64*4);glReadPixels(0,0,64,64,GL_RGBA,GL_UNSIGNED_BYTE,streamPixels.data());
    for(unsigned cell=0;cell<64;++cell){
        const unsigned i=1984+cell,x=(cell%8)*8+4,y=(cell/8)*8+4;
        const unsigned expected[]{i%251,(i*7)%253,(i*13)%255,128};
        for(unsigned ch=0;ch<4;++ch)if(std::abs(int(streamPixels[(y*64+x)*4+ch])-int(expected[ch]))>1)ok=false;
    }
    std::printf("2048 queued upload batches across ring wraps: %s\n",ok?"PASS":"FAIL");
    request.dispfb1=16|(1ULL<<9);
    GSPrimitiveBatch triangles=batch;triangles.vertexCount=3;
    triangles.state.prim.type=GS_PRIM_TRIANGLE;triangles.state.prim.tme=false;
    for(unsigned i=0;i<3;++i){triangles.vertices[i].x=-128.0f+(i==1?16.0f:0.0f);triangles.vertices[i].y=-128.0f+(i==2?16.0f:0.0f);triangles.vertices[i].z=123456;}
    for(uint8_t zpsm:{uint8_t(GS_PSM_Z32),uint8_t(GS_PSM_Z24),uint8_t(GS_PSM_Z16)}){
        triangles.state.context.zbuf.psm=zpsm;
        for(unsigned i=0;i<30000;++i)backend.Submit(triangles);
        backend.Flush();glFinish();
        const uint64_t triangleToken=gsNewDrawStateToken();
        for(bool cached:{false,true}){
            const auto begin=std::chrono::steady_clock::now();
            for(unsigned i=0;i<30000;++i){
                if(cached)backend.SubmitCached(triangles,triangleToken);else backend.Submit(triangles);
            }
            const double cpuMs=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-begin).count();
            backend.Flush();glFinish();
            std::printf("30000 offscreen triangles Z=%u cached=%d CPU: %.3f ms\n",zpsm,cached,cpuMs);
        }
    }
    const auto error=glGetError();ok=(error==GL_NO_ERROR)&&ok;
    std::printf("Production GLES raster, blend, shuffle and shared presentation: %s (GL error %x)\n",ok?"PASS":"FAIL",error);
    } // Destroy shared textures/fences before terminating this EGL share group.
    GSSharedPresent::RestoreHostContext();GSSharedPresent::ShutdownHost();
    eglMakeCurrent(display,EGL_NO_SURFACE,EGL_NO_SURFACE,EGL_NO_CONTEXT);
    eglDestroyContext(display,ctx);eglDestroySurface(display,surface);eglTerminate(display);
    return ok?0:20;
}
int main(){for(int epoch=1;epoch<=3;++epoch){const int code=runEpoch(epoch);if(code)return code;}return 0;}
