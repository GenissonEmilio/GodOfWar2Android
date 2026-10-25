// Diagnostic capture remains idle until armed, then replays synthetic programs.
#include "runtime/ps2_vu1_native.h"
#include "runtime/ps2_memory.h"
#include "runtime/gs/gs_frontend.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <string>
bool gowAndroidAsyncFinish(){return false;}
static uint32_t one(VU1NativeCtx &n){n.s->vi[1]=1;return kVu1nEnded|16u;}
int main(int argc,char **argv){
    if(argc!=3)return 1;
    const std::string file=argv[2],trigger=file+".arm";
    const bool record=std::string(argv[1])=="record";
    if(record){
        std::filesystem::remove(file);std::filesystem::remove(trigger);
        setenv("GOW_VU1N_RECORD",file.c_str(),1);setenv("GOW_VU1N_RECORD_ARM",trigger.c_str(),1);
        setenv("GOW_VU1N_RECORD_N","16",1);setenv("GOW_VU1N_RECORD_STRIDE","1",1);
    }else{
        setenv("GOW_VU1N_REPLAY",file.c_str(),1);setenv("GOW_VU1N_REPLAY_REPS","2",1);
    }
    PS2Memory memory;if(!memory.initialize())return 2;
    const uint32_t code[]{0x10010001u,0x400002ffu,0x8000033cu,0x000002ffu};
    std::memcpy(memory.getVU1Code(),code,sizeof(code));memory.markVU1CodeModified();
    vu1nRegisterBlock(vu1n::blockKey(0,code,4),one,2,nullptr);
    GS gs;VU1Interpreter vu;
    const auto execute=[&]{vu.execute(memory.getVU1Code(),PS2_VU1_CODE_SIZE,memory.getVU1Data(),PS2_VU1_DATA_SIZE,gs,&memory,0,0,0,64);};
    execute();
    if(record){
        if(std::filesystem::exists(file))return 3;
        FILE *arm=std::fopen(trigger.c_str(),"wb");if(!arm)return 4;std::fclose(arm);
        for(unsigned i=0;i<4112;++i)execute();
        std::filesystem::remove(trigger);
        if(!std::filesystem::exists(file) || std::filesystem::file_size(file)==0)return 5;
        std::puts("VU capture: no output before trigger, bounded capture after arming: PASS");
    }
    return vu.state().vi[1]==1?0:6;
}
