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
                {
                    std::fprintf(stderr, "[gow2-watch]   thread id=%d prio=%d status=%d wait=%d/%d pc=%08x ra=%08x sp=%08x\n", t.id,
                                 t.currentPriority, int(t.status), int(t.waitReason), t.waitId, t.pc, t.ra, t.sp);
                    if (t.pc == 0x0027D7E8u) // bucle de lista de 0x27D788: [sp] = cabeza, [sp+4] = iterador, nodo: +0 next, +8 objeto
                    {
                        const uint8_t *ram = runtime.memory().getRDRAM();
                        auto word = [ram](uint32_t a) { uint32_t v; std::memcpy(&v, ram + (a & 0x01FFFFFFu), 4); return v; };
                        const uint32_t head = word(t.sp), it = word(t.sp + 4u);
                        std::fprintf(stderr, "[gow2-watch]     list head=%08x it=%08x nodes:", head, it);
                        uint32_t node = word(head);
                        for (int i = 0; i < 8; ++i, node = word(node))
                            std::fprintf(stderr, " %08x(obj=%08x)", node, word(node + 8u));
                        std::fprintf(stderr, "\n");
                        // 0x18D5BC: obj22 = [0x362CA0]; estado = obj22->vtable[0x94]() → [0x2D814C]; si estado, push
                        const uint32_t obj22 = word(0x00362CA0u), vt = obj22 ? word(obj22 + 0x20u) : 0u;
                        std::fprintf(stderr, "[gow2-watch]     obj22=%08x vtable=%08x vt[0x94]=%08x adj=%d state=[0x2D814C]=%08x\n",
                                     obj22, vt, vt ? word(vt + 0x94u) : 0u,
                                     vt ? static_cast<int16_t>(word(vt + 0x90u) & 0xFFFFu) : 0, word(0x002D814Cu));
                    }
                }
                {
                    // árbol de 0x183330: centinela [0x2D7FC0], base del pool de nodos [0x2D7FC8] (hijos = índices de 16 bits)
                    const uint8_t *ram = runtime.memory().getRDRAM();
                    auto word = [ram](uint32_t a) { uint32_t v; std::memcpy(&v, ram + (a & 0x01FFFFFFu), 4); return v; };
                    const uint32_t sentinel = word(0x002D7FC0u), pool = word(0x002D7FC8u);
                    std::fprintf(stderr, "[gow2-watch]   tree sentinel=%08x pool=%08x distance=0x%x heap=[%08x,%08x)\n", sentinel, pool,
                                 sentinel - pool, runtime.guestHeapBase(), runtime.guestHeapLimit());
                }
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

    // 0x2D4850 int sceSifLoadStartModuleBuffer(iopAddr, argLen, args, int *result)
    // El arranque (0x1887E4 → 0x2BF660) copia a la memoria del IOP un módulo mínimo que va dentro del ELF (0x2DB590,
    // ~0x360 bytes) y lo arranca con esta función; luego exige id >= 0 y (*result & ~3) == 0, o se cuelga a propósito
    // en 0x18D5A0 (bloqueo 5 de la fase 2). El runtime aún no ejecuta módulos cargados desde la memoria del IOP, así
    // que se responde como una consola: cargado (id válido) y terminado con NO_RESIDENT_END (1). Como en GoW1.
    void gow2LoadStartModuleBuffer(uint8_t *rdram, R5900Context *ctx, PS2Runtime *)
    {
        gow2WriteGuest32(rdram, GPR_U32(ctx, 7), 1u); // NO_RESIDENT_END
        SET_GPR_U32(ctx, 2, 0x40001000u);
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

        // El juego crea su heap principal (0x146318, antes de main) desde el final del .bss hasta 0x01FF7FF8. El heap
        // interno del runtime nacía en la misma zona (con el SetupHeap del crt0: [0x37F4D0, 0x1F00000)), así que lo que
        // reservaban los stubs pisaba las estructuras del juego: el "RB NODE Heap" daba nodos en 0x84000010 y la búsqueda
        // de 0x183330 no terminaba nunca (bloqueo 3 de la fase 2). Como en GoW1, a la memoria libre bajo el ELF (0x100000).
        runtime.setPrivateGuestHeap(0x000A0000u, 0x000FF000u);

        const bool dualInfo = runtime.replaceFunction(0x002B6BF0u, gow2CdReadDvdDualInfo);
        const bool bootDevice = runtime.replaceFunction(0x001875E0u, gow2BootDeviceFromArgs);
        const bool moduleBuffer = runtime.replaceFunction(0x002D4850u, gow2LoadStartModuleBuffer);
        std::fprintf(stderr, "[gow2-override] sceCdReadDvdDualInfo=%d bootDevice=%d moduleBuffer=%d\n", dualInfo, bootDevice,
                     moduleBuffer);
    }
}

PS2_REGISTER_GAME_OVERRIDE("God of War II (SCUS-97481)", "SCUS_974.81", 0x00100008u, 0u, applyGow2Overrides)
