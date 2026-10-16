// GoW2-Port: overrides de God of War II (SCUS-97481 v1.01, SHA1 E6A62A0F3E781C29513F563F4625B9B3A9103C56).
// Las direcciones solo valen para ese ELF. Modelo: referencia/gow1/overrides/gow_overrides.cpp (Waffle565, vía KIexster).
// Cada override responde a un bloqueo apuntado en docs/NOTES.md ("Bloqueos").
#include "runtime/ee_scheduler.h"
#include "game_overrides.h"
#include "ps2_runtime.h"
#include "ps2_runtime_macros.h"
#include <ps2_recompiled_functions.h>
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <thread>

namespace
{
    // ---- Diagnóstico (solo con variables de entorno; no cambia el comportamiento) ----

    // GOW2_BOOT_TRACE=1: cuenta las llamadas a las funciones de la secuencia de arranque (0x188700…) y las
    // imprime cada segundo, para ver en qué bucle de espera se queda.
    struct Gow2BootCall
    {
        uint32_t address;
        const char *name;
        PS2Runtime::RecompiledFunction original;
        std::atomic<uint32_t> count{0};
        std::atomic<int32_t> lastV0{0};
    };
    Gow2BootCall g_gow2BootCalls[] = {
        {0x002D4C88u, "sceSifRebootIop", FUN_002d4c88_0x2d4c88},
        {0x002D4C38u, "sceSifSyncIop", FUN_002d4c38_0x2d4c38},
        {0x002B6900u, "sceCdInit", FUN_002b6900_0x2b6900},
        {0x002B6CC0u, "sceCdMmode", FUN_002b6cc0_0x2b6cc0},
        {0x002B67A8u, "cdCheckSCmd", FUN_002b67a8_0x2b67a8},
        {0x002D27B0u, "sceSifInitRpc", FUN_002d27b0_0x2d27b0},
        {0x002D42D8u, "sceSifInitIopHeap", FUN_002d42d8_0x2d42d8},
    };

    template <size_t I>
    void gow2BootTraceCall(uint8_t *rdram, R5900Context *ctx, PS2Runtime *runtime)
    {
        Gow2BootCall &call = g_gow2BootCalls[I];
        call.original(rdram, ctx, runtime);
        call.count.fetch_add(1u, std::memory_order_relaxed);
        call.lastV0.store(static_cast<int32_t>(GPR_U32(ctx, 2)), std::memory_order_relaxed);
    }

    template <size_t... I>
    void gow2InstallBootTrace(PS2Runtime &runtime, std::index_sequence<I...>)
    {
        (runtime.replaceFunction(g_gow2BootCalls[I].address, gow2BootTraceCall<I>), ...);
    }

    std::string gow2GuestString(const uint8_t *rdram, uint32_t addr)
    {
        std::string text;
        for (uint32_t i = 0; addr != 0u && i < 128u; ++i)
        {
            const char c = static_cast<char>(rdram[(addr + i) & 0x01FFFFFFu]);
            if (c == '\0')
                break;
            text.push_back((c >= 0x20 && c < 0x7F) ? c : '?');
        }
        return text;
    }

    // 0x2D4A98 sceSifLoadModule(path, argLen, args): lo usa el cargador del arranque (0x1885F8) para 8 módulos
    void gow2TraceLoadModule(uint8_t *rdram, R5900Context *ctx, PS2Runtime *runtime)
    {
        static std::atomic<int> logged{0};
        const std::string path = gow2GuestString(rdram, GPR_U32(ctx, 4));
        const std::string args = gow2GuestString(rdram, GPR_U32(ctx, 6));
        FUN_002d4a98_0x2d4a98(rdram, ctx, runtime);
        if (logged.fetch_add(1) < 64)
            std::fprintf(stderr, "[gow2-boot] sceSifLoadModule(\"%s\", args=\"%s\") -> %d\n", path.c_str(), args.c_str(),
                         static_cast<int32_t>(GPR_U32(ctx, 2)));
    }

    // 0x2D2DF8 sceSifBindRpc(client, sid, mode): devuelve <0 si falla; client+0x24 (server) queda a 0 si no existe
    void gow2TraceBindRpc(uint8_t *rdram, R5900Context *ctx, PS2Runtime *runtime)
    {
        static std::atomic<int> logged{0};
        const uint32_t client = GPR_U32(ctx, 4);
        const uint32_t sid = GPR_U32(ctx, 5);
        FUN_002d2df8_0x2d2df8(rdram, ctx, runtime);
        uint32_t server = 0;
        std::memcpy(&server, rdram + ((client + 0x24u) & 0x01FFFFFFu), sizeof(server));
        if (logged.fetch_add(1) < 32)
            std::fprintf(stderr, "[gow2-boot] sceSifBindRpc(client=%08x, sid=%08x) -> %d server=%08x\n", client, sid,
                         static_cast<int32_t>(GPR_U32(ctx, 2)), server);
    }

    // GOW2_THREAD_WATCH=<s>: cada <s> segundos, los hilos del EE (estado, espera, pc, ra) y, con GOW2_BOOT_TRACE,
    // los contadores de arranque. Para bloqueos silenciosos (como GOW_THREAD_WATCH en GoW1).
    void gow2StartWatch(PS2Runtime &runtime, double period, bool bootTrace)
    {
        std::thread([&runtime, period, bootTrace] {
            for (;;)
            {
                std::this_thread::sleep_for(std::chrono::duration<double>(period));
                const EeKernelSnapshot snapshot = runtime.eeScheduler().snapshot();
                std::fprintf(stderr, "[gow2-watch] running=%d cycle=%llu\n", snapshot.runningThreadId,
                             static_cast<unsigned long long>(snapshot.eeCycle));
                for (const EeThreadSnapshot &t : snapshot.threads)
                    std::fprintf(stderr, "[gow2-watch]   thread id=%d prio=%d status=%d wait=%d/%d pc=%08x ra=%08x\n", t.id,
                                 t.currentPriority, int(t.status), int(t.waitReason), t.waitId, t.pc, t.ra);
                if (bootTrace)
                    for (const Gow2BootCall &call : g_gow2BootCalls)
                        std::fprintf(stderr, "[gow2-watch]   boot %-18s %08x calls=%u last_v0=%d\n", call.name, call.address,
                                     call.count.load(), call.lastV0.load());
            }
        }).detach();
    }

    // ---- Overrides ----

    // Sector donde empieza la capa 1 del DVD-9 de GoW II (game/disc/.disc_layout, "layer1"; docs/NOTES.md).
    constexpr uint32_t kGow2Layer1StartLbn = 2083872u;

    void gow2WriteGuest32(uint8_t *rdram, uint32_t addr, uint32_t value)
    {
        if (addr == 0u)
            return;
        std::memcpy(rdram + (addr & 0x01FFFFFFu), &value, sizeof(value));
    }

    // 0x2B6BF0 int sceCdReadDvdDualInfo(int *on_dual, unsigned int *layer1_start)
    // El original primero comprueba que el servidor de comandos S del CDVD (0x2B67A8) esté listo y, si no,
    // devuelve 0 sin llamar al IOP. El arranque (0x188738) la repite mientras devuelva 0, así que se quedaba
    // ahí para siempre (bloqueo 1 de la fase 2; en GoW1, el mismo con su gowCdReadDvdDualInfo).
    // Se responde como el lector de una consola: disco de doble capa y dónde empieza la capa 1.
    void gow2CdReadDvdDualInfo(uint8_t *rdram, R5900Context *ctx, PS2Runtime *)
    {
        gow2WriteGuest32(rdram, GPR_U32(ctx, 4), 1u);
        gow2WriteGuest32(rdram, GPR_U32(ctx, 5), kGow2Layer1StartLbn);
        SET_GPR_U32(ctx, 2, 1u);
        ctx->pc = GPR_U32(ctx, 31);
    }

    // 0x1875E0 int bootDeviceFromArgs(int argc, char **argv), llamada desde main (0x1463F0).
    // Guarda en 0x2D7FE8 de dónde arrancó el juego (0 = "cdrom…", 1 = otro, es decir, host0: del kit de desarrollo).
    // Con ese valor, 0x187630 construye las rutas: "cdrom0:\RUTA;1" o "host0:ruta". Si argc es 0, el original no
    // escribe nada, el valor se queda en -1 (el de .data) y las rutas salen VACÍAS: no se carga ningún IRX y el
    // arranque se queda esperando al servidor de DBCMAN (bloqueo 2 de la fase 2).
    // TEMPORAL: el SetupThread del runtime (syscall 0x3C) ignora el bloque de argumentos ($a3 = 0x2FBE00), así que
    // main recibe argc = 0. Una PS2 da argc = 1 y argv[0] = "cdrom0:\SCUS_974.81;1"; aquí se hace como si fuera así.
    // Sobra cuando el runtime rellene los argumentos (docs/NOTES.md, "Bloqueos").
    void gow2BootDeviceFromArgs(uint8_t *rdram, R5900Context *ctx, PS2Runtime *)
    {
        const uint32_t argc = GPR_U32(ctx, 4);
        const uint32_t argv = GPR_U32(ctx, 5);
        int32_t fromHost = 0; // argc == 0: como una consola, arrancado desde el DVD
        if (argc != 0u)
        {
            uint32_t arg0 = 0;
            std::memcpy(&arg0, rdram + (argv & 0x01FFFFFFu), sizeof(arg0));
            fromHost = std::memcmp(rdram + (arg0 & 0x01FFFFFFu), "cdrom", 5) != 0 ? 1 : 0;
        }
        std::memcpy(rdram + 0x002D7FE8u, &fromHost, sizeof(fromHost));
        SET_GPR_U32(ctx, 2, static_cast<uint32_t>(fromHost));
        ctx->pc = GPR_U32(ctx, 31);
    }

    void applyGow2Overrides(PS2Runtime &runtime)
    {
        const bool bootTrace = std::getenv("GOW2_BOOT_TRACE") != nullptr;
        if (bootTrace)
        {
            gow2InstallBootTrace(runtime, std::make_index_sequence<std::size(g_gow2BootCalls)>{});
            runtime.replaceFunction(0x002D4A98u, gow2TraceLoadModule);
            runtime.replaceFunction(0x002D2DF8u, gow2TraceBindRpc);
        }
        if (const char *watch = std::getenv("GOW2_THREAD_WATCH"))
            gow2StartWatch(runtime, std::max(1.0, std::atof(watch)), bootTrace);

        const bool dualInfo = runtime.replaceFunction(0x002B6BF0u, gow2CdReadDvdDualInfo);
        const bool bootDevice = runtime.replaceFunction(0x001875E0u, gow2BootDeviceFromArgs);
        std::fprintf(stderr, "[gow2-override] sceCdReadDvdDualInfo=%d bootDevice=%d\n", dualInfo, bootDevice);
    }
}

PS2_REGISTER_GAME_OVERRIDE("God of War II (SCUS-97481)", "SCUS_974.81", 0x00100008u, 0u, applyGow2Overrides)
