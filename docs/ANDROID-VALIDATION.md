# Validação Android

Status: em desenvolvimento. Nenhuma declaração de campanha completa ou de
paridade de experiência com Black Android foi estabelecida.

Data dos testes: 24 de outubro de 2026 (America/Fortaleza).

## Ambiente observado

- POCO M8 Pro 5G; Android 16; ARM64; Snapdragon SM7635.
- Adreno 810; driver reporta OpenGL ES 3.2; páginas de memória de 4096 bytes.
- SDK 35; NDK 28.2.13676358; Clang; CMake 3.31.6.

## Evidência inicial

- Código Java compilado contra Android SDK 35; D8 gerou o bytecode Android e
  AAPT empacotou o manifesto para API 29–35.
- Os quatro programas reais de shader (desenho, shuffle, apresentação e CLUT)
  compilaram e passaram pela linkedição no driver Adreno 810 do aparelho, usando
  um probe EGL independente gerado de `gs_hw_backend.cpp`. Isso testa a aceitação
  dos shaders, não a correção visual da campanha.
- Os dados do DVD foram transferidos para a pasta privada externa do aplicativo.
  O executável foi verificado novamente por SHA256 no aparelho; PART1 e PART2
  têm 4.264.382.464 e 4.265.340.928 bytes, respectivamente.
- Runtime C++ compilado para ARM64 com a camada SSE2NEON.
- A suíte herdada passou em 24 testes sem jogo.
- Quatro importações inválidas rejeitadas pelo importador Java: entrada truncada,
  arquivo de nome malicioso com conteúdo inválido, ZIP sem ISO e ZIP com dois ISOs.
  Os dados anteriores foram preservados e os temporários removidos.
- Os dados locais vêm da cópia já importada e validada pela base PC:
  `SCUS_974.81` SHA256
  `fd403603be46dea70d130695820d28b3f31de6d7e48484a5f006398a605f7dd6`.

- O importador Java processou o ZIP de 6,7 GiB fornecido pelo usuário e extraiu
  os 16 arquivos de ambas as camadas. O executável, GODOFWAR.TOC, .disc_layout
  e .disc_meta foram comparados byte a byte com a extração PC e são idênticos.
  Os dois PAKs têm os tamanhos esperados. Não foi comparado o hash integral dos PAKs.
- [CI sem dados do jogo](https://github.com/GenissonEmilio/GodOfWar2Android/actions/runs/40150496466): passou.

- O renderizador de produção `GSHwBackend` foi executado no aparelho em um
  teste sintético independente: desenhou RGB (200,100,40), depois misturou
  RGB (100,200,80) com fator 0,5 e produziu (150,150,60). Os pixels foram
  lidos do framebuffer e verificados. Não houve erro GL no teste final.
  Esse teste identificou e corrigiu a configuração inicial dos samplers GLES.

- O mesmo teste sintético passou novamente com a biblioteca estática de produção
  recompilada, incluindo a otimização que copia o framebuffer somente quando uma
  textura é simultaneamente fonte e destino.

- APK ARM64 de desenvolvimento gerado e instalado por USB no aparelho. O jogo
  carregou o ELF, os oito módulos IOP nativos e o áudio a 48 kHz, chegando ao
  menu original com imagem. A presença de saída de áudio não valida sua qualidade.
- O overlay touch aparece sobre a superfície nativa; o diálogo de pausa foi
  aberto no aparelho. Sua presença visual foi confirmada; os demais fluxos ainda
  estão sendo testados.
- O fallback inicial por cópias tinha cerca de 1 FPS no menu. O caminho com
  `GL_EXT_shader_framebuffer_fetch` passou pelo teste de pixels e mostrou trechos
  de 8–16 FPS no menu. Esses valores são observações iniciais, com desempenho
  ainda insuficiente, e não um benchmark da campanha.

## Critérios restantes

- Build e instalação do APK final; carregamento de todos os shaders no driver móvel.
- Logos, menu, vídeo inicial e entrada em Rhodes com imagem e áudio corretos.
- Combate e QTE com multitoque; reconexão de controle físico.
- Pausa/retomada, saves/carregamento, editor e persistência das preferências.
- Medições do FPS real, consumo de memória e aquecimento em sessões longas.
- Validação da campanha inteira, incluindo bosses, vídeos e as duas camadas do DVD.
- Integração dos ajustes aos menus originais do jogo, além do overlay móvel.
