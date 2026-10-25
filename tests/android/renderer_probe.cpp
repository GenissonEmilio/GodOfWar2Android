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
