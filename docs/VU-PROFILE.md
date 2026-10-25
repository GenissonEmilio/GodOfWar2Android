# Perfil local da VU no Android

O Clang pode usar as frequências reais de execução para organizar e otimizar os
blocos da VU. O fluxo usa IR-PGO e ThinLTO somente nos fontes `vu1n_part*.cpp`
e em `ps2_vu1_core.cpp`; EE, IOP, shaders e regras numéricas permanecem com
suas opções anteriores. A documentação do compilador descreve o
[treinamento por instrumentação](https://clang.llvm.org/docs/UsersManual.html#profile-guided-optimization).

É necessário um build ARM64 já compilado, o SDK/NDK usado nesse build e uma
captura local do próprio jogo. Não há captura nem perfil do jogo neste repositório.

1. Obtenha uma captura usando `perf-vu-record` e `perf-vu-capture-now`, conforme
   [ANDROID-VALIDATION.md](ANDROID-VALIDATION.md). Faça isso fora da medição de FPS.
2. Com o app pausado ou fechado e o aparelho autorizado no ADB, execute:

   ```sh
   export ANDROID_HOME=/caminho/do/android-sdk
   python3 tools/android/train_vu_profile.py \
     --build-dir artifacts/build/arm64 \
     --capture artifacts/vu-combat.bin \
     --output artifacts/vu-combat-production.profdata
   ```

   O script compila executáveis auxiliares em uma pasta separada, reproduz a
   captura no aparelho e aceita o perfil apenas se houver registros válidos e
   todos os hashes coincidirem. O hash compara estado VU, memória e pacotes GIF;
   não substitui a validação de timing, pixels e jogabilidade. Os arquivos
   temporários dessa execução no aparelho são removidos ao final.
3. Ative o perfil no build e recompile:

   ```sh
   cmake -S . -B artifacts/build/arm64 \
     -DGOW_ANDROID_VU_PROFILE="$PWD/artifacts/vu-combat-production.profdata"
   cmake --build artifacts/build/arm64 --target ps2EntryRunner -j3
   ```

   Use o CMake do SDK ou o mesmo executável que configurou o build existente.
   O empacotamento permanece em `tools/android/package-local.sh`.
4. Execute os probes VU/CPU, a comparação de captura e a medição no jogo. Um
   replay mais rápido não prova o mesmo ganho de FPS, nem desempenho na campanha.

O perfil acompanha um arquivo `.profdata.json` com versão do compilador e hashes
dos fontes e da captura. Após alterar o gerador, helpers ou versão do Clang,
retreine e confira avisos de perfil incompatível. Os perfis ficam ignorados pelo
Git. Para voltar ao build sem perfil, configure `-DGOW_ANDROID_VU_PROFILE=`.

Na experiência inicial com quatro rodadas alternadas e 2.000 registros reais,
as medianas para 8.000 replays aquecidos foram 84,202 ms sem perfil, 83,432 ms
com ThinLTO isolado e 75,677 ms com IR-PGO + ThinLTO. Todos os registros
coincidiram. São tempos da execução VU isolada, não FPS do jogo.
