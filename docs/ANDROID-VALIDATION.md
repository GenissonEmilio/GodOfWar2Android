# Validação Android

Status: em desenvolvimento. Nenhuma declaração de campanha completa ou de
paridade de experiência com Black Android foi estabelecida.

## Ambiente observado

- POCO M8 Pro 5G; Android 16; ARM64; Snapdragon SM7635.
- Adreno 810; driver reporta OpenGL ES 3.2; páginas de memória de 4096 bytes.
- SDK 35; NDK 28.2.13676358; Clang; CMake 3.31.6.

## Evidência inicial

- Código Java compilado contra Android SDK 35.
- Runtime C++ compilado para ARM64 com a camada SSE2NEON.
- Quatro importações inválidas rejeitadas pelo importador Java: entrada truncada,
  arquivo de nome malicioso com conteúdo inválido, ZIP sem ISO e ZIP com dois ISOs.
  Os dados anteriores foram preservados e os temporários removidos.
- Os dados locais vêm da cópia já importada e validada pela base PC:
  `SCUS_974.81` SHA256
  `fd403603be46dea70d130695820d28b3f31de6d7e48484a5f006398a605f7dd6`.

## Critérios restantes

- Build e instalação do APK final; carregamento de todos os shaders no driver móvel.
- Logos, menu, vídeo inicial e entrada em Rhodes com imagem e áudio corretos.
- Combate e QTE com multitoque; reconexão de controle físico.
- Pausa/retomada, saves/carregamento, editor e persistência das preferências.
- Medições do FPS real, consumo de memória e aquecimento em sessões longas.
- Validação da campanha inteira, incluindo bosses, vídeos e as duas camadas do DVD.
- Integração dos ajustes aos menus originais do jogo, além do overlay móvel.
