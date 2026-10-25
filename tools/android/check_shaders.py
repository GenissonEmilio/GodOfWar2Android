#!/usr/bin/env python3
"""Generate a standalone, ROM-free GLES shader probe from the actual runtime shaders."""
from pathlib import Path
import re
root=Path(__file__).resolve().parents[2]
source=(root / 'PS2Recomp/ps2xRuntime/src/lib/gs/gpu/gs_hw_backend.cpp').read_text()
parts=dict(re.findall(r'const char \*(k\w+) = R"\((.*?)\)";',source,re.S))
print(list(parts))
cpp='''#include <EGL/egl.h>
#include <GLES3/gl32.h>
#include <cstdio>
#include <string>
GLuint shader(GLenum type,const char *src) {auto sh=glCreateShader(type);glShaderSource(sh,1,&src,nullptr);glCompileShader(sh);GLint ok=0;glGetShaderiv(sh,GL_COMPILE_STATUS,&ok);if(!ok){char log[8192];glGetShaderInfoLog(sh,sizeof(log),nullptr,log);printf("SHADER ERROR: %s\\n",log);return 0;}return sh;}
bool pair(const char *name,const char *vs,const char *fs) {GLuint v=shader(GL_VERTEX_SHADER,vs),f=shader(GL_FRAGMENT_SHADER,fs);if(!v||!f)return false;GLuint p=glCreateProgram();glAttachShader(p,v);glAttachShader(p,f);glLinkProgram(p);GLint ok=0;glGetProgramiv(p,GL_LINK_STATUS,&ok);if(!ok){char log[8192];glGetProgramInfoLog(p,sizeof(log),nullptr,log);printf("LINK ERROR: %s\\n",log);}printf("%s: %s\\n",name,ok?"PASS":"FAIL");return ok;}
int main(){auto d=eglGetDisplay(EGL_DEFAULT_DISPLAY);EGLint n;eglInitialize(d,nullptr,nullptr);EGLConfig cfg;EGLint attrs[]={EGL_SURFACE_TYPE,EGL_PBUFFER_BIT,EGL_RENDERABLE_TYPE,EGL_OPENGL_ES3_BIT,EGL_RED_SIZE,8,EGL_GREEN_SIZE,8,EGL_BLUE_SIZE,8,EGL_NONE};eglChooseConfig(d,attrs,&cfg,1,&n);EGLint size[]={EGL_WIDTH,16,EGL_HEIGHT,16,EGL_NONE},version[]={EGL_CONTEXT_CLIENT_VERSION,3,EGL_NONE};auto surface=eglCreatePbufferSurface(d,cfg,size);auto context=eglCreateContext(d,cfg,EGL_NO_CONTEXT,version);if(!eglMakeCurrent(d,surface,surface,context)){printf("EGL error %x\\n",eglGetError());return 2;}printf("%s / %s\\n",glGetString(GL_RENDERER),glGetString(GL_VERSION));bool ok=true;
'''
def convert(s):
 return s.replace('#version 430 core','#version 320 es\nprecision highp float;\nprecision highp int;\nprecision highp usampler2D;').replace('noperspective ','').replace('layout(location = 0, index = 0)','layout(location = 0)').replace('layout(location = 0, index = 1) out vec4 oBlend;','vec4 oBlend;')
for name,vs,fs in [('draw','kVsSource',['kFsSource','kReinterpretSource','kFsTail']),('shuffle','kShuffleVs',['kShuffleFs']),('present','kPresentVs',['kPresentFs']),('clut','kPresentVs',['kClutFsHead','kReinterpretSource','kClutFsMain'])]:
 if all(k in parts for k in [vs]+fs):
  cpp+='ok=pair("'+name+'",R"GOW('+convert(parts[vs])+')GOW",R"GOW('+convert(''.join(parts[k] for k in fs))+')GOW") && ok;\n'
cpp+='return ok?0:1;}\n'
(root / 'artifacts').mkdir(exist_ok=True)
(root / 'artifacts/gles-probe.cpp').write_text(cpp)
