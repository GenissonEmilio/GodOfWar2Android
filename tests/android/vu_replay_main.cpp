// Standalone driver; the owned game's local capture is supplied at runtime.
#include "runtime/ps2_memory.h"
#include "runtime/ps2_vu1.h"
#include "runtime/gs/gs_frontend.h"
#include <cstdio>
#include <cstdlib>
bool gowAndroidAsyncFinish() { return false; }
int main(int argc, char **argv)
{
    if (argc != 2) return 1;
    FILE *input = std::fopen(argv[1], "rb");
    if (!input) return 2;
    std::fclose(input);
    setenv("GOW_VU1N_REPLAY", argv[1], 1);
    setenv("GOW_VU1N_REPLAY_REPS", "5", 1);
    PS2Memory memory;
    if (!memory.initialize()) return 3;
    GS gs;
    VU1Interpreter vu;
    vu.execute(memory.getVU1Code(), PS2_VU1_CODE_SIZE, memory.getVU1Data(), PS2_VU1_DATA_SIZE,
               gs, &memory, 0, 0, 0, 64);
}
