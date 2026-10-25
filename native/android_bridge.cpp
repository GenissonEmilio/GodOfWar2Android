#include "android_bridge.h"
#include "raylib.h"
#include "runtime/ps2_perf.h"
#include <jni.h>
#include <atomic>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <unistd.h>
namespace {
std::atomic<uint64_t> input{0x808080800000ULL};
std::atomic<bool> paused{false};
std::atomic<int> volume{80}, frameLimit{60};
std::atomic<int> screenMode{0};
std::atomic<float> deadzone{0.14f}, sensitivity{1.0f};
uint8_t axis(float v) {
    const float dz=deadzone.load();
    float a=std::abs(v);
    v=a<=dz?0:std::copysign((a-dz)/(1-dz),v)*sensitivity.load();
    return static_cast<uint8_t>(std::clamp(128+int(v*127),1,255));
}
}
bool gowAndroidPaused() { return paused.load(); }
void gowAndroidReadPad(uint8_t *data) {
    const auto state=paused.load()?0x808080800000ULL:input.load();
    const auto buttons=static_cast<uint16_t>(~state);
    data[2]=buttons&255; data[3]=buttons>>8;
    for (int i=0;i<4;++i) data[4+i]=(state>>(16+8*i))&255;
    // DualShock 2 pressure channels: right, left, up, down, triangle, circle, cross, square, L1, R1, L2, R2.
    constexpr uint16_t bits[]{0x20,0x80,0x10,0x40,0x1000,0x2000,0x4000,0x8000,0x400,0x800,0x100,0x200};
    for (int i=0;i<12;++i) data[8+i]=(~buttons&bits[i])?255:0;
}
void gowAndroidFrame() {
    SetMasterVolume(paused.load()?0.0f:volume.load()/100.0f);
    SetTargetFPS(frameLimit.load());
    SetExitKey(KEY_NULL);
}
extern "C" JNIEXPORT void JNICALL Java_io_github_genissonemilio_gow2_GameActivity_nativeConfigure(JNIEnv *env,jclass,jstring path,jint scale) {
    const char *p=env->GetStringUTFChars(path,nullptr);
    std::filesystem::create_directories(std::string(p)+"/memcards"); chdir(p);
    const std::string boot=std::string(p)+"/game/SCUS_974.81";
    setenv("GOW2_BOOT",boot.c_str(),1);
    setenv("GOW_MC0",(std::string(p)+"/memcards/Mcd001.ps2").c_str(),1);
    setenv("PS2X_GS_HW_SCALE",std::to_string(std::clamp(int(scale),1,3)).c_str(),1);
    setenv("GOW_PERF_FRAME_PC","0x14E380",1);
    setenv("GOW_PERF_DIAG","frames",1);
    env->ReleaseStringUTFChars(path,p);
}
extern "C" JNIEXPORT void JNICALL Java_io_github_genissonemilio_gow2_GameActivity_nativeInput(JNIEnv *,jclass,jint buttons,jfloat lx,jfloat ly,jfloat rx,jfloat ry) {
    uint64_t state=static_cast<uint16_t>(buttons);
    const uint8_t axes[]{axis(rx),axis(ry),axis(lx),axis(ly)};
    for(int i=0;i<4;++i) state|=uint64_t(axes[i])<<(16+i*8);
    input.store(state);
}
extern "C" JNIEXPORT void JNICALL Java_io_github_genissonemilio_gow2_GameActivity_nativePause(JNIEnv *,jclass,jboolean value) { paused.store(value); input.store(0x808080800000ULL); if (IsAudioDeviceReady()) SetMasterVolume(value?0.0f:volume.load()/100.0f); }
extern "C" JNIEXPORT void JNICALL Java_io_github_genissonemilio_gow2_GameActivity_nativeSettings(JNIEnv *,jclass,jint vol,jint fps,jfloat dz,jfloat gain) {
    volume.store(std::clamp(int(vol),0,100)); frameLimit.store(fps==30?30:60);
    deadzone.store(std::clamp(dz,0.0f,0.5f)); sensitivity.store(std::clamp(gain,0.1f,3.0f));
}

extern "C" JNIEXPORT jlong JNICALL Java_io_github_genissonemilio_gow2_GameActivity_nativeFrames(JNIEnv *,jclass) { return static_cast<jlong>(ps2_perf::guestFlips.load(std::memory_order_relaxed)); }
