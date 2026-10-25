// Original synthetic input only. Exercises the production GLES GS backend.
#include "runtime/gs/gs_hw_backend.h"
#include "runtime/gs/gs_shared_present.h"
#include <EGL/egl.h>
#include <GLES3/gl32.h>
#include <vector>
#include <cstdio>
#include <cstdlib>
#include <cmath>
static bool pixel(const PresentationFrame &frame,int r,int g,int b) {
    if(frame.width<32 || frame.height<32 || frame.pixels.size()<frame.width*frame.height*4u)return false;
    size_t at=(std::max(frame.width,640u)*16u+16u)*4u;
    std::printf("pixel %u,%u,%u expected %d,%d,%d; %ux%u\n",frame.pixels[at],frame.pixels[at+1],frame.pixels[at+2],r,g,b,frame.width,frame.height);
    return std::abs(int(frame.pixels[at])-r)<=2 && std::abs(int(frame.pixels[at+1])-g)<=2 && std::abs(int(frame.pixels[at+2])-b)<=2;
}
int main(){
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
    std::vector<uint8_t> vram(4u*1024u*1024u);GSHwBackend backend;backend.Initialize(vram.data(),vram.size());
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
    bool ok=pixel(frame,200,100,40);
    batch.state.prim.abe=true;batch.state.context.alpha=0 | (1ULL<<2) | (0ULL<<4) | (1ULL<<6);
    for(auto &v:batch.vertices){v.r=100;v.g=200;v.b=80;v.a=64;}
    backend.Submit(batch);backend.Flush();
    ok=pixel(backend.Present(request),150,150,60)&&ok;
    std::printf("Production GLES raster + shader blending: %s (GL error %x)\n",ok?"PASS":"FAIL",glGetError());
    return ok?0:20;
}
