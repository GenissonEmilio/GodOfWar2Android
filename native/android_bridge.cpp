#include "android_bridge.h"
#include "raylib.h"
#include "runtime/ps2_perf.h"
#include <jni.h>
#include <GLES3/gl3.h>
#include <atomic>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <unistd.h>
#include <android/looper.h>
namespace {
std::atomic<uint64_t> input{0x808080800000ULL};
std::atomic<uint16_t> pendingPress{0}, physicalPress{0};
std::atomic<uint64_t> physicalInput{0x808080800000ULL};
std::atomic<bool> paused{false},backRequested{false},asyncFinish{false},shuttingDown{false};
std::atomic<bool> controllerConnected{false},controllerReady{false};
std::atomic<bool> movieActive{false};
std::atomic<uint64_t> movieFrames{0};
std::atomic<int> volume{80}, frameLimit{60}, appliedLimit{-1};
std::atomic<int> screenMode{0};
std::atomic<float> deadzone{0.14f}, sensitivity{1.0f};
uint8_t axis(float v) {
    const float dz=deadzone.load();
    float a=std::abs(v);
    v=a<=dz?0:std::copysign((a-dz)/(1-dz),v)*sensitivity.load();
    return static_cast<uint8_t>(std::clamp(128+int(v*127),1,255));
}
}
bool gowAndroidPaused() { return paused.load() || shuttingDown.load(); }
bool gowAndroidAsyncFinish() { return asyncFinish.load(); }
void gowAndroidReadPad(uint8_t *data) {
    uint64_t state=input.load()|pendingPress.exchange(0);
    if(controllerConnected.load()) {
        const uint64_t pad=physicalInput.load()|physicalPress.exchange(0);
        state|=pad&0xffff;
        for(int i=0;i<4;++i) {
            const int shift=16+8*i,touchAxis=(state>>shift)&255,padAxis=(pad>>shift)&255;
            if(std::abs(padAxis-128)>std::abs(touchAxis-128))
                state=(state&~(uint64_t(255)<<shift))|(uint64_t(padAxis)<<shift);
        }
    }
    if(paused.load())state=0x808080800000ULL;
    const auto buttons=static_cast<uint16_t>(~state);
    data[2]=buttons&255; data[3]=buttons>>8;
    for (int i=0;i<4;++i) data[4+i]=(state>>(16+8*i))&255;
    // DualShock 2 pressure channels: right, left, up, down, triangle, circle, cross, square, L1, R1, L2, R2.
    constexpr uint16_t bits[]{0x20,0x80,0x10,0x40,0x1000,0x2000,0x4000,0x8000,0x400,0x800,0x100,0x200};
    for (int i=0;i<12;++i) data[8+i]=(~buttons&bits[i])?255:0;
}
void gowAndroidFrame() {
    SetMasterVolume(gowAndroidPaused()?0.0f:volume.load()/100.0f);
    const int wanted=frameLimit.load();
    if (appliedLimit.exchange(wanted)!=wanted)SetTargetFPS(wanted);
    // Read raylib only on its host/input thread. The EE thread consumes an
    // atomic DualShock snapshot, including short press edges and pressure bytes.
    int slot=-1;for(int i=0;i<4;++i)if(IsGamepadAvailable(i)){slot=i;break;}
    controllerReady.store(slot>=0);
    uint64_t pad=0x808080800000ULL;
    if(slot>=0 && controllerConnected.load()) {
        constexpr int keys[]{GAMEPAD_BUTTON_MIDDLE_LEFT,GAMEPAD_BUTTON_LEFT_THUMB,GAMEPAD_BUTTON_RIGHT_THUMB,GAMEPAD_BUTTON_MIDDLE_RIGHT,
            GAMEPAD_BUTTON_LEFT_FACE_UP,GAMEPAD_BUTTON_LEFT_FACE_RIGHT,GAMEPAD_BUTTON_LEFT_FACE_DOWN,GAMEPAD_BUTTON_LEFT_FACE_LEFT,
            GAMEPAD_BUTTON_LEFT_TRIGGER_2,GAMEPAD_BUTTON_RIGHT_TRIGGER_2,GAMEPAD_BUTTON_LEFT_TRIGGER_1,GAMEPAD_BUTTON_RIGHT_TRIGGER_1,
            GAMEPAD_BUTTON_RIGHT_FACE_UP,GAMEPAD_BUTTON_RIGHT_FACE_RIGHT,GAMEPAD_BUTTON_RIGHT_FACE_DOWN,GAMEPAD_BUTTON_RIGHT_FACE_LEFT};
        for(int i=0;i<16;++i)if(IsGamepadButtonDown(slot,keys[i]))pad|=uint64_t(1)<<i;
        if(GetGamepadAxisMovement(slot,GAMEPAD_AXIS_LEFT_TRIGGER)>.1f)pad|=0x100;
        if(GetGamepadAxisMovement(slot,GAMEPAD_AXIS_RIGHT_TRIGGER)>.1f)pad|=0x200;
        constexpr int axes[]{GAMEPAD_AXIS_RIGHT_X,GAMEPAD_AXIS_RIGHT_Y,GAMEPAD_AXIS_LEFT_X,GAMEPAD_AXIS_LEFT_Y};
        for(int i=0;i<4;++i){const int shift=16+8*i;pad=(pad&~(uint64_t(255)<<shift))|(uint64_t(axis(GetGamepadAxisMovement(slot,axes[i])))<<shift);}
    }
    const auto oldPad=physicalInput.exchange(pad);
    physicalPress.fetch_or(static_cast<uint16_t>(pad&~oldPad));
    for (int key=GetKeyPressed();key!=0;key=GetKeyPressed())
        if(key==KEY_BACK || key==KEY_ESCAPE)backRequested.store(true);
    SetExitKey(KEY_NULL);
}
extern "C" JNIEXPORT void JNICALL Java_io_github_genissonemilio_gow2_GameActivity_nativeConfigure(JNIEnv *env,jclass,jstring path,jint scale,jboolean parallel) {
    input.store(0x808080800000ULL);physicalInput.store(0x808080800000ULL);
    pendingPress.store(0);physicalPress.store(0);paused.store(false);backRequested.store(false);shuttingDown.store(false);appliedLimit.store(-1);
    controllerConnected.store(false);controllerReady.store(false);movieActive.store(false);movieFrames.store(0);
    const char *p=env->GetStringUTFChars(path,nullptr);
    std::filesystem::create_directories(std::string(p)+"/memcards"); chdir(p);
    const std::string boot=std::string(p)+"/game/SCUS_974.81";
    setenv("GOW2_BOOT",boot.c_str(),1);
    setenv("GOW_MC0",(std::string(p)+"/memcards/Mcd001.ps2").c_str(),1);
    setenv("PS2X_GS_HW_SCALE",std::to_string(std::clamp(int(scale),1,3)).c_str(),1);
    setenv("PS2X_GS_WORKER",parallel?"1":"0",1);
    setenv("PS2X_GS_PIPELINE",parallel?"1":"0",1);
    setenv("GOW_PERF_FRAME_PC","0x14E380",1);
    const bool diagnostic=std::filesystem::exists(std::string(p)+"/perf-diagnostics");
    setenv("GOW_PERF_DIAG",diagnostic?"1":"frames",1);
    if(diagnostic){setenv("GOW_GS_HW_STATS","1",1);setenv("GOW_VU1_STATS","1",1);setenv("GOW_VBLANK_DIAG","1",1);setenv("GOW_MPEG_DIAG","1",1);}
    env->ReleaseStringUTFChars(path,p);
}
extern "C" JNIEXPORT void JNICALL Java_io_github_genissonemilio_gow2_GameActivity_nativeInput(JNIEnv *,jclass,jint buttons,jfloat lx,jfloat ly,jfloat rx,jfloat ry) {
    uint64_t state=static_cast<uint16_t>(buttons);
    const uint8_t axes[]{axis(rx),axis(ry),axis(lx),axis(ly)};
    for(int i=0;i<4;++i) state|=uint64_t(axes[i])<<(16+i*8);
    const auto previous=input.exchange(state);
    pendingPress.fetch_or(static_cast<uint16_t>(state & ~previous));
}
extern "C" JNIEXPORT void JNICALL Java_io_github_genissonemilio_gow2_GameActivity_nativePause(JNIEnv *,jclass,jboolean value) { paused.store(value); input.store(0x808080800000ULL); pendingPress.store(0); physicalPress.store(0); if (IsAudioDeviceReady()) SetMasterVolume(gowAndroidPaused()?0.0f:volume.load()/100.0f); }
extern "C" JNIEXPORT void JNICALL Java_io_github_genissonemilio_gow2_GameActivity_nativeSettings(JNIEnv *,jclass,jint vol,jint fps,jfloat dz,jfloat gain,jboolean graphicsAsync) {
    volume.store(std::clamp(int(vol),0,100)); frameLimit.store(fps==30?30:60);asyncFinish.store(graphicsAsync);
    deadzone.store(std::clamp(dz,0.0f,0.5f)); sensitivity.store(std::clamp(gain,0.1f,3.0f));
}

extern "C" JNIEXPORT jlong JNICALL Java_io_github_genissonemilio_gow2_GameActivity_nativeFrames(JNIEnv *,jclass) { return static_cast<jlong>(ps2_perf::guestFlips.load(std::memory_order_relaxed)); }

// raylib 5.5 calls the EXT spelling even with its ES3 backend. Android exports
// the equivalent core ES3 function; keep this compatibility shim local.
extern "C" void glDrawBuffersEXT(GLsizei count,const GLenum *buffers) { glDrawBuffers(count,buffers); }

extern "C" JNIEXPORT jboolean JNICALL Java_io_github_genissonemilio_gow2_GameActivity_nativeConsumeBack(JNIEnv *,jclass) { return backRequested.exchange(false); }

void gowAndroidMovieFrame() { if(!movieActive.exchange(true))pendingPress.store(0);movieFrames.fetch_add(1); }
void gowAndroidMovieEnd() { movieActive.store(false); }
extern "C" JNIEXPORT jlong JNICALL Java_io_github_genissonemilio_gow2_GameActivity_nativeMovieFrames(JNIEnv *,jclass) { return movieActive.load()?static_cast<jlong>(movieFrames.load()):-1; }

extern "C" JNIEXPORT jboolean JNICALL Java_io_github_genissonemilio_gow2_GameActivity_nativeGamepadReady(JNIEnv *,jclass) { return controllerConnected.load() && controllerReady.load(); }
extern "C" JNIEXPORT void JNICALL Java_io_github_genissonemilio_gow2_GameActivity_nativeControllerConnection(JNIEnv *,jclass,jboolean connected) {
    if(controllerConnected.exchange(connected) && !connected){physicalInput.store(0x808080800000ULL);physicalPress.store(0);}
}

// raylib 5.5 sets Window.shouldClose on APP_CMD_DESTROY, but then polls
// indefinitely once more while unfocused. NativeActivity.onDestroy waits for
// that native thread. Bound only shutdown waits so normal background blocking
// remains intact. --wrap applies this adapter only inside our library.
extern "C" int __real_ALooper_pollOnce(int,int*,int*,void**);
extern "C" int __wrap_ALooper_pollOnce(int timeout,int *fd,int *events,void **data) {
    if(shuttingDown.load() && timeout<0)timeout=16;
    return __real_ALooper_pollOnce(timeout,fd,events,data);
}
extern "C" JNIEXPORT void JNICALL Java_io_github_genissonemilio_gow2_GameActivity_nativeBeginShutdown(JNIEnv*,jclass) { shuttingDown.store(true); }
