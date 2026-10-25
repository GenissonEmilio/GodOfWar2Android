// Compare the bulk PACKED decoder against its byte-fragmented reference path.
#include "capture_backend.h"
#include "runtime/gs/gs_state_compare.h"
#include <algorithm>
#include <bit>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <tuple>
bool gowAndroidAsyncFinish(){return false;}
static void require(bool ok){if(!ok){std::fprintf(stderr,"GIF stream mismatch\n");std::exit(1);}}
static auto vertexKey(const GSVertex &v){return std::tuple{
    std::bit_cast<uint32_t>(v.x),std::bit_cast<uint32_t>(v.y),std::bit_cast<uint64_t>(v.z),
    v.r,v.g,v.b,v.a,std::bit_cast<uint32_t>(v.q),std::bit_cast<uint32_t>(v.s),
    std::bit_cast<uint32_t>(v.t),v.u,v.v,v.fog};}
static std::vector<GSPrimitiveBatch> decode(const std::vector<uint64_t> &words,unsigned fragment,GifPathId path){
    GS gs;auto backend=std::make_unique<CaptureBackend>();auto *out=backend.get();gs.setRasterBackend(std::move(backend));
    gs.writeRegister(GS_REG_UV,13u|(37ull<<16));gs.writeRegister(GS_REG_FOG,0xA5ull<<56);
    auto *bytes=reinterpret_cast<const uint8_t*>(words.data());const auto size=words.size()*8;
    for(size_t i=0;i<size;i+=fragment)gs.processGIFPacket(bytes+i,std::min<size_t>(fragment,size-i),path);
    require(!gs.hasPendingGIFPacket(path));return std::move(out->draws);
}
int main(){
    unsigned cases=0;
    for(unsigned prim=0;prim<7;++prim)for(unsigned xyz:{4u,5u})for(unsigned path=1;path<=3;++path){
        std::vector<uint64_t> words;
        for(unsigned tag=0;tag<4;++tag){
            const unsigned count=tag?19:0; // NLOOP=0, followed by several tags.
            words.push_back((3ull<<60)|(1ull<<46)|(uint64_t(prim|(1u<<4)|(1u<<8))<<47)|count);
            words.push_back(2u|(1u<<4)|(uint64_t(xyz)<<8));
            for(unsigned i=0;i<count;++i){
                constexpr uint32_t qs[]{0,0x80000000u,0x3f000000u,0x3f800000u,0x7fc01234u,0x7f800000u};
                words.push_back(uint64_t(0x3f000000u+i)|(uint64_t(0x3f800000u+i)<<32));words.push_back(qs[i%6]);
                words.push_back(uint64_t(19+i)|(uint64_t(27+i)<<32));words.push_back(uint64_t(43+i)|(uint64_t(129+i)<<32));
                words.push_back(uint64_t(160+i*16)|(uint64_t(320+i*16)<<32));
                words.push_back((xyz==4?(uint64_t(0xffffffu-i)<<4)|(uint64_t(89+i)<<36):0xffffffffu-i)|(uint64_t(i%4==0)<<47));
            }
        }
        // An ST-only vertex after the bulk packet checks the retained RGBAQ.Q.
        words.insert(words.end(),{(2ull<<60)|1,2u|(uint64_t(xyz)<<4),0,0x40000000u,64u|(128ull<<32),97u});
        const auto expected=decode(words,1,static_cast<GifPathId>(path));
        require(!expected.empty());
        for(unsigned fragment:{7u,16u,31u,47u,48u,49u,63u,128u,65536u}){
            const auto actual=decode(words,fragment,static_cast<GifPathId>(path));require(actual.size()==expected.size());
            for(size_t i=0;i<actual.size();++i){
                require(actual[i].vertexCount==expected[i].vertexCount && gsSameDrawState(actual[i].state,expected[i].state));
                for(unsigned v=0;v<actual[i].vertexCount;++v)require(vertexKey(actual[i].vertices[v])==vertexKey(expected[i].vertices[v]));
            }++cases;
        }
    }
    std::printf("%u GIF stream cases: primitives, all paths, fragmentation, ADC, Q latch, zero/NaN and Z32: PASS\n",cases);
    GS gs;auto backend=std::make_unique<CaptureBackend>();auto *out=backend.get();gs.setRasterBackend(std::move(backend));
    std::vector<uint64_t> packet{(3ull<<60)|(1ull<<46)|4095,0x512};
    for(unsigned i=0;i<4095;++i)packet.insert(packet.end(),{0,0x3f800000,0,0,0,0});
    const auto start=std::chrono::steady_clock::now();size_t draws=0;
    for(unsigned i=0;i<128;++i){gs.processGIFPacket(reinterpret_cast<uint8_t*>(packet.data()),packet.size()*8);draws+=out->draws.size();out->draws.clear();}
    std::printf("PACKED ST/RGBAQ/XYZ: %zu vertices in %.3f ms\n",draws,std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count());
}
