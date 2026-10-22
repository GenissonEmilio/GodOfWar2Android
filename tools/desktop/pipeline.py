"""Local static recompilation pipeline. Generated game code stays in user data."""
from __future__ import annotations
import concurrent.futures
import hashlib
import json
import os
from pathlib import Path
import runpy
import subprocess
import sys
import time
import threading

BOOT = "SCUS_974.81"


def helper(source, name, arguments):
    source = Path(source).resolve()
    allowed = {"iop": source / "tools/iop/iopn_gen.py", "vu1": source / "tools/vu1/vu1n_gen.py"}
    if name not in allowed:
        raise ValueError("Unknown pipeline helper")
    sys.argv = [str(allowed[name]), *map(str, arguments)]
    runpy.run_path(sys.argv[0], run_name="__main__")


class Pipeline:
    def __init__(self, program, data, environment, progress=lambda s: None):
        self.program, self.data = Path(program), Path(data)
        self.env, self.progress = environment, progress
        self.bundle = self.program / "compiler"
        self.source = self.bundle / "source"
        self.manifest = json.loads((self.bundle / "manifest.json").read_text())
        self.build_id = self.manifest["build_id"]
        self.work = self.data / "builds" / self.build_id
        self.work.mkdir(parents=True, exist_ok=True)
        self.log = self.data / "last-prepare.log"

    def run(self, args, cwd=None, env=None):
        with self.log.open("a") as stream:
            subprocess.run(list(map(str,args)), cwd=cwd or self.work, env=env or self.env,
                           stdout=stream, stderr=subprocess.STDOUT, check=True)

    def tool(self, name):
        return self.bundle / "bin" / name

    def helper(self, name, args, cwd=None):
        entry = [sys.executable] if getattr(sys,"frozen",False) else [sys.executable,str(Path(__file__).with_name("main.py"))]
        self.run([*entry, "--pipeline-helper", self.source, name, *args], cwd)

    def compiler_env(self):
        env = dict(self.env)
        tc = self.bundle / "toolchain"
        env["LD_LIBRARY_PATH"] = str(tc / "lib")
        env["PATH"] = str(tc / "bin")+os.pathsep+env.get("PATH", "")
        env["GCC_EXEC_PREFIX"] = str(tc / "lib/gcc")+"/"
        return env

    def compile(self, directory):
        generated = directory / "output"
        cpp = sorted(generated.glob("*.cpp"))
        native = directory / "native"
        extra = sorted(native.glob("*.cpp"))
        if not cpp:
            raise ValueError("O recompilador não produziu código.")
        tc = self.bundle / "toolchain"
        includes = [generated, self.source / "PS2Recomp/ps2xRuntime/include",
                    self.source / "PS2Recomp/ps2xIOP/include", self.source / "raylib",
                    self.source / "PS2Recomp/ps2xRuntime/src/lib/Kernel",
                    self.source / "PS2Recomp/ps2xIOP/src",
                    self.source / "PS2Recomp/ps2xRuntime/src/lib/vu"]
        flags = [str(tc / "bin/g++"), "--sysroot="+str(tc / "sysroot"),
                 "-B"+str(tc / "lib/gcc/x86_64-linux-gnu/13")+"/",
                 "-std=c++20", "-O2", "-DNDEBUG", "-msse4.1", "-fno-strict-aliasing",
                 "-nostdinc++", "-isystem", str(tc / "include/c++/13"),
                 "-isystem", str(tc / "include/x86_64-linux-gnu/c++/13"),
                 "-isystem", str(tc / "include/c++/13/backward"),
                 *["-I"+str(p) for p in includes],
                 "-DPS2_RUNTIME_LOGS=1", "-DPS2X_ENABLE_IOP_RPC_TRACE=0"]
        objects = directory / "objects"
        objects.mkdir(exist_ok=True)
        batches = [cpp[i:i+32] for i in range(0,len(cpp),32)] + [[p] for p in extra]
        header_key = hashlib.sha256(self.build_id.encode()+repr(flags).encode())
        for header in sorted(generated.glob("*.h"))+sorted(native.glob("*.h")):
            header_key.update(header.read_bytes())
        failed = threading.Event()
        def batch(item):
            if failed.is_set():
                raise RuntimeError("Compilation cancelled after an earlier error")
            index, files = item
            unity = objects / f"batch-{index:04}.cpp"
            unity.write_text("\n".join('#include "'+str(f)+'"' for f in files)+"\n")
            obj = unity.with_suffix(".o")
            key = header_key.copy()
            for source in files:
                key.update(source.name.encode()); key.update(source.read_bytes())
            stamp = obj.with_suffix(".key")
            expected = key.hexdigest()
            if not (obj.is_file() and stamp.is_file() and stamp.read_text() == expected):
                try:
                    self.run([*flags,"-c",unity,"-o",obj],env=self.compiler_env())
                    stamp.write_text(expected)
                except Exception:
                    failed.set()
                    raise
            return obj
        result = []
        with concurrent.futures.ThreadPoolExecutor(max_workers=2) as pool:
            for i, obj in enumerate(pool.map(batch,enumerate(batches)),1):
                result.append(obj)
                self.progress(f"Compilando para Linux: {i}/{len(batches)} blocos…")
        executable = directory / "game"
        libs = [self.bundle / p for p in self.manifest["link_inputs"]]
        self.run([*flags, *result, "-Wl,--whole-archive", libs[0], "-Wl,--no-whole-archive",
                  "-Wl,--start-group", *libs[1:], "-Wl,--end-group",
                  "-static-libstdc++", "-static-libgcc", "-Wl,-rpath-link,"+str(self.bundle / "lib"),
                  "-Wl,-rpath-link,"+str(tc / "sysroot/usr/lib/x86_64-linux-gnu"),
                  "-Wl,-rpath-link,"+str(tc / "lib"), "-pthread", "-ldl", "-lm", "-o",executable],
                 env=self.compiler_env())
        return executable

    def prepare(self, disc):
        import fcntl
        disc = Path(disc).resolve()
        with (self.data / ".build.lock").open("w") as lock:
            fcntl.flock(lock, fcntl.LOCK_EX)
            stage = self.work / "native-game"
            final = stage / "game"
            stamp = self.work / "complete.json"
            if final.is_file() and stamp.is_file():
                try:
                    saved = json.loads(stamp.read_text())
                    if saved.get("sha256") == hashlib.sha256(final.read_bytes()).hexdigest():
                        self.progress("Compilação pronta. Reutilizando o cache…")
                        return final
                except (OSError,ValueError,AttributeError):
                    pass
            self.log.write_text("God of War II local static recompilation — "+self.build_id+"\n")
            stage.mkdir(exist_ok=True)
            config = (self.source / "config/recomp.template.toml").read_text()
            config = config.replace("@ELF@", str(disc / BOOT)).replace("@MAP@",str(self.source / "config/funcmap.csv")).replace("@OUT@",str(stage / "output")+"/")
            (stage / "config.toml").write_text(config)
            if not (stage / "output/ps2_recompiled_functions.h").is_file():
                self.progress("Traduzindo o executável do PS2 para C++…")
                self.run([self.tool("ps2_recomp"),stage / "config.toml"])
            self.progress("Compilando o jogo para Linux…")
            executable = self.compile(stage)
            captures = self.work / "captures"
            vu1, iop = captures / "vu1", captures / "iop"
            for folder in (vu1,iop): folder.mkdir(parents=True,exist_ok=True)
            captured = captures / "complete.json"
            if not captured.is_file():
                self.progress("Otimizando o início do jogo automaticamente (4 minutos)…")
                capture_env = dict(self.env, GOW_VU1_CAPTURA=str(vu1), GOW_VU1_NATIVE_OFF="1",
                    PS2X_IOP_DUMP_MODULES=str(iop), GOW2_PAD_SCRIPT="15:start,20-220/3:x",
                    GOW2_EXIT_SECONDS="240", GOW_WINDOW="960x540", PS2X_HIDDEN_WINDOW="1")
                capture_env.pop("GOW2_SHADER",None)
                capture_player = self.work / "capture-player"
                (capture_player / "memcards").mkdir(parents=True,exist_ok=True)
                capture_env["GOW_MC0"] = str(capture_player / "memcards/Mcd001.ps2")
                with self.log.open("a") as output:
                    process = subprocess.Popen([str(executable),str(disc / BOOT)],cwd=capture_player,
                        env=capture_env,stdout=output,stderr=subprocess.STDOUT)
                    try:
                        code = process.wait(timeout=300)
                        if code: raise ValueError(f"O jogo não terminou a otimização. Consulte {self.log}")
                    finally:
                        if process.poll() is None:
                            process.terminate()
                            try: process.wait(timeout=10)
                            except subprocess.TimeoutExpired: process.kill();process.wait()
                count = len(list(vu1.glob("*.bin")))
                if not count or not (iop / "modules.txt").is_file():
                    raise ValueError(f"A captura de otimização ficou incompleta. Consulte {self.log}")
                captured.write_text(json.dumps({"vu1_images":count})+"\n")
            native = stage / "native"
            native.mkdir(exist_ok=True)
            self.progress("Gerando os módulos nativos de áudio e gráficos…")
            if not (native / "iopn_registry.cpp").is_file(): self.helper("iop",[iop,native])
            if not (native / "vu1n_part0.cpp").is_file(): self.helper("vu1",[native,vu1,"--heads",self.source / "tools/vu1/vu1n_heads.txt","--parts","32"])
            executable = self.compile(stage)
            stamp.write_text(json.dumps({"build_id":self.build_id,"sha256":hashlib.sha256(executable.read_bytes()).hexdigest(),
                "native_iop":True,"native_vu1":True})+"\n")
            return executable
