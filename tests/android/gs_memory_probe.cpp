#include "runtime/gs/ps2_gs_memory.h"
#include <algorithm>
#include <array>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <vector>
using namespace GSMem;
using Read=u32(*)(u8*,u32,u32,u32,u32);
using Write=void(*)(u8*,u32,u32,u32,u32,u32);
struct Format{PixelStorageMode psm;Read read;Write write;};
static const Format formats[]{
 {C32,ReadCT32,WriteCT32},{Z32,ReadZ32,WriteZ32},{C24,ReadCT24,WriteCT24},{Z24,ReadZ24,WriteZ24},
 {C16,ReadCT16,WriteCT16},{C16S,ReadCT16S,WriteCT16S},{Z16,ReadZ16,WriteZ16},{Z16S,ReadZ16S,WriteZ16S},
 {P8,ReadP8,WriteP8},{P8H,ReadP8H,WriteP8H},{P4,ReadP4,WriteP4},{P4HL,ReadP4HL,WriteP4HL},{P4HH,ReadP4HH,WriteP4HH}};
int main(){
 InitLookupTables();std::vector<u8> vram(MEMORY_SIZE),expected(MEMORY_SIZE),source(4096),indices(1024);
 uint32_t rng=17;auto random=[&]{rng=rng*1664525u+1013904223u;return rng;};
 for(auto &byte:vram)byte=random()>>24;for(auto &byte:source)byte=random()>>24;
 unsigned cases=0;
 for(const auto &f:formats)for(unsigned test=0;test<128;++test){
   const u32 bp=(test&1)?(random()&0x3fffu):0x3fffu,bw=random()%16;
   const u32 x=(test%3)?(random()%4096):0xffffffc0u,y=random()%2048;
   const u32 count=random()%513,skip=random()%17,bits=BitsPerPixel(f.psm);
   expected=vram;
   for(u32 i=0;i<count;++i){
     u32 value=0;const u32 bit=(skip+i)*bits;
     if(bits==4)value=(source[bit/8]>>(bit%8))&15;
     else for(u32 byte=0;byte<bits/8;++byte)value|=u32(source[bit/8+byte])<<(byte*8);
     f.write(expected.data(),bp,bw,x+i,y,value);
   }
   WriteRun(vram.data(),u32(f.psm),bp,bw,x,y,source.data(),skip,count);
   if(vram!=expected){std::fprintf(stderr,"write FAIL psm=%u test=%u x=%u count=%u\n",u32(f.psm),test,x,count);return 1;}
   if(IsPaletted(f.psm)){
     ReadIndexRun(vram.data(),u32(f.psm),bp,bw,x,y,count,indices.data());
     for(u32 i=0;i<count;++i)if(indices[i]!=f.read(vram.data(),bp,bw,x+i,y)){
       std::fprintf(stderr,"index FAIL psm=%u test=%u x=%u y=%u i=%u actual=%u expected=%u\n",u32(f.psm),test,x,y,i,indices[i],f.read(vram.data(),bp,bw,x+i,y));return 2;
     }
   }
   ++cases;
 }
 std::printf("%u GS row cases: all 13 formats, page/VRAM/coordinate wrap, partial rows and preserved lanes: PASS\n",cases);
 // Same workload can be compiled with the old source for an A/B CPU benchmark.
 volatile u32 sink=0;
 for(auto psm:{P8,P4}){
   const auto start=std::chrono::steady_clock::now();
   for(unsigned repeat=0;repeat<2000;++repeat)for(u32 y=0;y<256;++y){
     ReadIndexRun(vram.data(),u32(psm),0,4,0,y,256,indices.data());sink=sink^indices[(repeat+y)&255];
   }
   const double ms=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();
   std::printf("Read psm=%u 131072000 pixels: %.3f ms\n",u32(psm),ms);
 }
 const auto start=std::chrono::steady_clock::now();
 for(unsigned repeat=0;repeat<1000;++repeat)for(u32 y=0;y<256;++y)WriteRun(vram.data(),u32(C32),0,4,0,y,source.data(),0,256);
 std::printf("Write CT32 65536000 pixels: %.3f ms (sink %u)\n",std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count(),u32(sink));
}
