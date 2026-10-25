#include "MiniTest.h"
#include <atomic>
#include <GLES3/gl3.h>
extern "C" void glDrawBuffersEXT(GLsizei n,const GLenum *b){glDrawBuffers(n,b);}
#include <cstdint>
#include <cstring>
bool gowAndroidPaused(){return false;}
bool gowAndroidAsyncFinish(){return false;}
void gowAndroidReadPad(uint8_t *data){std::memset(data,0,20);data[2]=data[3]=255;for(int i=4;i<8;++i)data[i]=128;}
void gowAndroidFrame(){}
void gowAndroidMovieFrame(){}
void gowAndroidMovieEnd(){}
void register_ps2_runtime_kernel_tests();
void reset_ps2_test_function_table();
int main(){MiniTest::BeforeEach(reset_ps2_test_function_table);register_ps2_runtime_kernel_tests();return MiniTest::Run();}
