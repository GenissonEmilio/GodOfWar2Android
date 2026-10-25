#pragma once
#include "runtime/gs/gs_frontend.h"
#include <vector>
struct CaptureBackend final : GSRasterBackend {
    std::vector<GSPrimitiveBatch> draws;
    std::vector<uint64_t> tokens;
    void Initialize(uint8_t*,uint32_t)override{} void Reset()override{}
    void Submit(const GSPrimitiveBatch &batch)override{draws.push_back(batch);}
    void SubmitCached(const GSPrimitiveBatch &batch,uint64_t token)override{tokens.push_back(token);Submit(batch);}
    void LoadClut(const GSTex0Reg&,const GSTexClutReg&)override{}
    void BeginTransfer(const GSTransferCommand&)override{} void UploadImage(const uint8_t*,uint32_t)override{}
    void Flush()override{} void TextureFlush()override{} void Sync(GSSyncReason)override{}
    PresentationFrame Present(const GSPresentationRequest&)override{return{};}
    bool ClearFramebuffer(const GSContext&,uint32_t)override{return false;}
    uint32_t ConsumeLocalToHostBytes(uint8_t*,uint32_t)override{return 0;}
    uint32_t ReadVram(uint32_t,uint32_t,uint32_t,uint32_t,uint32_t)const override{return 0;}
    void WriteVram(uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t)override{}
    void SnapshotVram(std::vector<uint8_t>&)const override{}
    GSTransferSnapshot GetTransferSnapshot()const override{return{};}
};
