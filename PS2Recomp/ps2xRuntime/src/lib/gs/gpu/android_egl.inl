// EGL contexts share GPU textures with raylib's NativeActivity surface.
#include <EGL/egl.h>
#include <dlfcn.h>
namespace {
EGLDisplay hostDisplay = EGL_NO_DISPLAY;
EGLContext hostContext = EGL_NO_CONTEXT;
EGLSurface hostDraw = EGL_NO_SURFACE, hostRead = EGL_NO_SURFACE;
EGLConfig hostConfig = nullptr;
void *loadProc(const char *name) {
    void *p = reinterpret_cast<void *>(eglGetProcAddress(name));
    if (!p) { static void *gles = dlopen("libGLESv3.so", RTLD_NOW); p = dlsym(gles,name); }
    return p;
}
struct EglWorker { EGLDisplay display; EGLContext context; EGLSurface surface; };
EglWorker *newWorker(EGLContext shared) {
    const EGLint attrs[]{EGL_CONTEXT_CLIENT_VERSION,3,EGL_NONE};
    const EGLint size[]{EGL_WIDTH,16,EGL_HEIGHT,16,EGL_NONE};
    auto *worker = new EglWorker{hostDisplay,eglCreateContext(hostDisplay,hostConfig,shared,attrs),EGL_NO_SURFACE};
    if (worker->context != EGL_NO_CONTEXT) worker->surface = eglCreatePbufferSurface(hostDisplay,hostConfig,size);
    if (worker->context == EGL_NO_CONTEXT || worker->surface == EGL_NO_SURFACE) {
        if (worker->context != EGL_NO_CONTEXT) eglDestroyContext(hostDisplay,worker->context);
        delete worker; return nullptr;
    }
    return worker;
}
}
namespace GSSharedPresent {
void CaptureHostContext() {
    hostDisplay=eglGetCurrentDisplay(); hostContext=eglGetCurrentContext();
    hostDraw=eglGetCurrentSurface(EGL_DRAW); hostRead=eglGetCurrentSurface(EGL_READ);
    EGLint id=0, count=0;
    eglQueryContext(hostDisplay,hostContext,EGL_CONFIG_ID,&id);
    const EGLint attrs[]{EGL_CONFIG_ID,id,EGL_NONE};
    eglChooseConfig(hostDisplay,attrs,&hostConfig,1,&count);
    std::string missing;
    loadHostApi(loadProc,s_hostApi,missing);
}
void RestoreHostContext() { if (hostContext != EGL_NO_CONTEXT) eglMakeCurrent(hostDisplay,hostDraw,hostRead,hostContext); }
void ShutdownHost() {
    std::lock_guard<std::mutex> lock(s_sharedMutex); releaseReading();
    s_sharedActive.store(false,std::memory_order_release);
    hostContext=EGL_NO_CONTEXT;
}
}
namespace GSGL {
bool Context::Create(std::string &error) {
    Destroy();
    auto *worker=newWorker(hostContext);
    if (!worker || !eglMakeCurrent(worker->display,worker->surface,worker->surface,worker->context)) {
        if (worker) DestroyWorker(worker);
        error="Unable to create shared Android EGL context"; return false;
    }
    m_window=worker; m_context=worker->context; m_shared=true;
    std::string missing; loadContextApi(loadProc,m_api,missing);
    if (!missing.empty() || !m_api.hwRaster) {
        error="GLES entry points missing: "+missing; Destroy(); return false;
    }
    std::fprintf(stderr,"[gs-egl] %s / %s\n",m_api.GetString(kVersion),m_api.GetString(kRenderer));
    s_sharedActive.store(true,std::memory_order_release);
    return true;
}
void *Context::CreateWorker() const { return newWorker(static_cast<EGLContext>(m_context)); }
bool Context::MakeWorkerCurrent(void *ptr) const {
    auto *w=static_cast<EglWorker *>(ptr);
    return w && eglMakeCurrent(w->display,w->surface,w->surface,w->context);
}
void Context::DestroyWorker(void *ptr) {
    auto *w=static_cast<EglWorker *>(ptr); if (!w) return;
    if (eglGetCurrentContext()==w->context) eglMakeCurrent(w->display,EGL_NO_SURFACE,EGL_NO_SURFACE,EGL_NO_CONTEXT);
    eglDestroySurface(w->display,w->surface); eglDestroyContext(w->display,w->context); delete w;
}
void Context::Destroy() {
    DestroyWorker(m_window); m_window=nullptr; m_context=nullptr; m_api={}; m_shared=false;
}
}
