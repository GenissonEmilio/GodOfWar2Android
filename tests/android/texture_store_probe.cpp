// Synthetic palette loads: CLD bank state must survive skipped loads and TEX0
// writes for non-indexed textures. No game data or graphics context is needed.
#include "gs_texture_store.h"
#include <cstdio>
#include <cstdlib>
#include <vector>

int main()
{
    std::vector<uint8_t> memory(4u<<20);
    GSTextureStore store;
    store.Initialize(memory.data(),memory.size());
    GSTex0Reg t{};t.psm=GS_PSM_T8;t.cbp=10;t.cpsm=GS_PSM_CT32;
    GSTexClutReg layout{};
    unsigned reads=0,checks=0;uint32_t color=0x80402010;
    const auto source=[&](const GSTex0Reg&,uint32_t,const uint16_t*,uint32_t n,uint32_t *words){
        ++reads;for(uint32_t i=0;i<n;++i)words[i]=color+i;return true;
    };
    const auto load=[&](uint8_t cld,uint32_t cbp,bool expected){
        t.cld=cld;t.cbp=cbp;
        if(store.NeedsClutLoad(t)!=expected)std::abort();
        const unsigned before=reads;
        store.LoadClut(t,layout,source);
        if((reads!=before)!=expected)std::abort();
        ++checks;
    };
    load(0,10,false);load(1,10,true);const auto original=store.ClutHash();
    load(2,10,true);color=0x80A0B0C0;load(4,10,false);
    if(store.ClutHash()!=original)std::abort();
    load(1,10,true);if(store.ClutHash()==original)std::abort();
    load(4,10,false);load(4,11,true);load(4,11,false);load(4,10,true);
    load(3,20,true);load(5,20,false);load(5,21,true);load(5,21,false);
    load(6,30,false);load(7,30,false);
    // Non-indexed TEX0 writes cannot update the CLUT comparison banks.
    for(uint8_t psm:{uint8_t(GS_PSM_CT32),uint8_t(GS_PSM_CT24),uint8_t(GS_PSM_CT16),uint8_t(GS_PSM_Z32)}){
        t.psm=psm;for(uint8_t cld=0;cld<8;++cld)load(cld,50+cld,false);
        t.psm=GS_PSM_T8;load(4,10,false);load(5,21,false);
    }
    // All five indexed formats share the same bank rules.
    for(uint8_t psm:{uint8_t(GS_PSM_T8),uint8_t(GS_PSM_T8H),uint8_t(GS_PSM_T4),uint8_t(GS_PSM_T4HL),uint8_t(GS_PSM_T4HH)}){
        t.psm=psm;load(2,30,true);load(4,30,false);load(3,40,true);load(5,40,false);
    }
    // Invalid CPSM still latches CLD=2/3's CBP before rejecting the pixels.
    t.cpsm=15;t.cld=2;t.cbp=60;store.LoadClut(t,layout,source);
    t.cpsm=GS_PSM_CT32;load(4,60,false);
    // An unconditional reload still reads the source, but identical pixels do
    // not disturb queued draws. A changed palette flushes while the old one is
    // still available to those draws.
    t.cld=1;store.LoadClut(t,layout,source);
    unsigned changes=0;const auto oldHash=store.ClutHash();
    auto beforeChange=[&]{if(store.ClutHash()!=oldHash)std::abort();++changes;};
    if(store.LoadClut(t,layout,source,beforeChange) || changes)std::abort();
    ++color;
    if(!store.LoadClut(t,layout,source,beforeChange) || changes!=1 || store.ClutHash()==oldHash)std::abort();
    checks+=2;
    store.Reset();load(4,0,false);load(5,0,false);load(4,60,true);
    std::printf("CLUT unconditional/conditional loads, shared banks, no-op formats and reset: PASS (%u checks)\n",checks);
}
