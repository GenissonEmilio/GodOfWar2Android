# Validação Android

Status: em desenvolvimento. Nenhuma declaração de campanha completa ou de
paridade de experiência com Black Android foi estabelecida.

Data dos testes: 24 e 25 de outubro de 2026 (America/Fortaleza).

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
  Os dois PAKs têm os tamanhos esperados. Os hashes integrais dos PAKs da cópia
  local de referência foram depois comparados com os do aparelho: coincidiram.
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

- New Game e seleção de dificuldade foram acionados pelo botão touch. O vídeo
  de abertura foi visto no aparelho; depois o jogo entrou na primeira sala de
  Rhodes, exibindo Kratos, inimigos, HUD e o tutorial de combate. Essa é evidência
  de entrada na primeira área, não de campanha completa ou combate fluido.
- O controle de opacidade foi alterado no menu e o valor persistiu após atualizar
  o APK; o diálogo pausou a execução. O botão Voltar do Android abriu a pausa.
  No editor, L1 foi arrastado de (0,09; 0,18) para aproximadamente (0,12; 0,218),
  e a nova posição apareceu nas preferências persistidas. Restaurar layout removeu
  essa personalização. O aumento individual de L1 também persistiu (raio 1,1×).
  O preset de quatro dedos foi selecionado e apareceu no jogo.
- Ir para a tela inicial e retornar pelo launcher preservou o mesmo processo e o
  menu renderizado. Não foi observado reinício do runtime nesse teste.
- O teste sintético foi ampliado com uma cópia CT16 entre canais do mesmo alvo:
  produziu RGB (150,150,150) a partir do resultado anterior. Passou no caminho
  com framebuffer fetch e no fallback por cópias. Sem erros GL.

## Desempenho e controles após as correções

- A comparação do estado gráfico agora ignora o preenchimento de estruturas C++.
  O diagnóstico no aparelho confirmou mais de mil divisões de lotes causadas
  somente por esses bytes. O menu passou a apresentar janelas próximas de 25–29 FPS
  e amostras de 30 FPS; há variação entre execuções. Não é um resultado da campanha.
- Na primeira sala de Rhodes, foram observados aproximadamente 10–19 FPS com
  diagnósticos ativos. O desempenho de combate continua insuficiente.
- Ataques pelo quadrado produziram a animação das lâminas e contato com inimigos.
  Um probe externo injetou oito toques de ataque enquanto mantinha o analógico de
  movimento. Os eventos foram aceitos, e o jogo registrou as transições do botão;
  não foram validados todos os combos ou QTEs.
- O perfil simpleperf de 12 segundos coletou 9.514 amostras sem perda. Os custos
  aparecem em VIF/VU, processamento GIF, preparação de vértices e na fila do GS;
  a busca repetida de glClearDepthf também apareceu e agora é armazenada em cache.
- O estado do controle físico agora é capturado na thread de entrada e entregue
  atomicamente à thread do jogo, com retenção de toques curtos, gatilhos e pressão
  dos botões DS2. Não havia um gamepad físico conectado para validar essa rota.
- A retomada do relógio Android foi ajustada para evitar que uma pausa ou cena
  lenta acumulasse atraso e acelerasse o vídeo seguinte. Após a alteração, os
  quadros 420 e 450 do vídeo foram entregues com 999,9 ms entre eles; cada grupo
  avançou 90.090 ticks de PTS. Depois de uma pausa longa nos ajustes, os quadros
  1530 a 1890 continuaram avançando em grupos de 30 por segundo, e o VBlank
  estabilizou em 60,00 Hz sem recuperar o atraso por aceleração.
  O limitador de apresentação mediu 29,97–30,12 Hz quando selecionado.
- O cache VU/IOP utilizado foi capturado no início do jogo pela base PC. Blocos
  posteriores não capturados continuam tendo o intérprete como fallback; isso
  precisa ser medido e ampliado ao percorrer a campanha.

- O usuário confirmou que o áudio do vídeo de abertura estava normal e
  sincronizado no POCO. Isso não valida todas as faixas de áudio da campanha.
- O caminho de compilação `tools/android/build-local.sh` foi executado com o SDK
  local: recompilou, empacotou, alinhou e assinou o APK sem usar Gradle.
- A CI do commit aa5bdf3 passou, incluindo o novo teste de preenchimento de estado
  gráfico em C++ no Linux.
- Um probe ARM64 da fila de produção verificou 200 sequências de desenho/FINISH,
  a ordem preservada no modo assíncrono, a drenagem antes de leitura e a espera do
  modo síncrono. O ajuste assíncrono permanece experimental e desligado por padrão.
  No menu sem os diagnósticos, o modo normal mediu cerca de 25 FPS, e o assíncrono
  aproximadamente 24–26 FPS: não foi demonstrado ganho material nesse cenário.

- Os PAKs no aparelho coincidiram com a referência local por SHA256 completo:
  PART1 `6e7ec60e56939c5f9bf6d0d5954b416e7dd82064d062dc560917943c6be8770c`;
  PART2 `58f79aef9bfc55190f64f3e66cc20049587fa7d4661cee671a224ecec8f0b3e0`.
- Com diagnósticos desligados, a primeira luta continuou com aproximadamente
  10–11 FPS. A opção assíncrona não demonstrou ganho suficiente nesse trecho;
  foi desativada novamente. No vídeo, o contador manteve 30 FPS.
- Em um snapshot durante o vídeo, dumpsys registrou aproximadamente 309 MiB PSS,
  372 MiB RSS e 71 MiB de swap. Não são medidas de pico nem de uma sessão longa.

## Encerramento e novas sessões

- O teste de sair pelo menu revelou uma espera infinita do loop raylib após
  APP_CMD_DESTROY. A espera durante o encerramento agora tem limite; o bloqueio
  normal de segundo plano foi preservado.
- O caminho Android retorna de main, permitindo o retorno ao launcher. O logger
  é reutilizado entre sessões, e o limite de FPS é reaplicado na nova janela.
- Foi corrigida a ordem de encerramento de áudio do raylib 5.5 em uma cópia no
  diretório de build: o dispositivo AAudio para antes da destruição do mutex.
  O checkout raylib compartilhado com a base PC não foi alterado.
- Os recursos de apresentação compartilhada são destruídos com o contexto EGL;
  isso corrigiu a imagem magenta na segunda abertura.
- Três aberturas sucessivas no mesmo processo exibiram o menu original com imagem.
  As duas saídas intermediárias retornaram ao launcher, e o áudio foi fechado sem
  o erro de mutex destruído observado antes da correção.
- O teste de pixels de produção agora recria três grupos EGL completos e verifica
  também os pixels da textura compartilhada. Os três ciclos passaram tanto com
  EXT framebuffer fetch quanto no fallback por cópias, sem erros GL.
- A pausa agora deixa de consumir a fila de áudio, além de silenciar a saída;
  isso mantém a fila junto do relógio congelado do jogo.

## Compilação ThinLTO

- Foi gerado um APK separado com otimização entre unidades de tradução. O mesmo
  teste de pixels, com três ciclos EGL e os dois caminhos de feedback, passou.
  O teste de ordem da fila também passou com a biblioteca otimizada.
- No menu, as janelas estáveis ficaram próximas de 25–28 FPS, sem ganho material
  sobre a compilação normal. A opção continua desligada por padrão.
- Na primeira sala, a versão ThinLTO mediu aproximadamente 11–13 FPS, com
  trechos de introdução próximos de 18–20 FPS. Não resolveu o gargalo de combate.
- A CI do commit 71eae71, que inclui as correções de ciclo de vida, passou.

## Publicação por pacote e envio de vetores

- A fila do GS agora mantém um pacote GIF sob o mesmo bloqueio de produção,
  preservando a ordem das chamadas síncronas internas e impedindo a intercalação
  de outro produtor. Os registros são escritos contiguamente no buffer da fila.
- O estado de desenho é reconstruído somente após uma escrita de estado; mudar
  coordenadas, cores e outros atributos de vértice continua atualizando os vértices.
- O caminho V4-32 sem máscara/soma copia segmentos contíguos para a memória VU,
  respeitando os saltos de STCYCL e o retorno ao início dos 16 KiB.
- O probe ARM64 passou em 2.400 casos de V4-32 contra uma referência escalar,
  incluindo TOPS, NUM=0, endereços que dão a volta na memória, modos e máscaras.
  O probe do frontend verificou texturas, contexto, globais, PACKED A+D e reset.
- O probe da fila passou com produtor concorrente, pacotes maiores que um chunk,
  escopos aninhados, chamadas síncronas dentro do pacote e saída por exceção.
- Os testes de pixels passaram novamente nos três ciclos EGL, com framebuffer
  fetch e com cópias, sem erros GL. Os 24 testes Linux também passaram.
- No POCO, sem o processamento paralelo, o menu ficou em cerca de 26–29 FPS,
  e a primeira luta apresentou janelas de aproximadamente 12,8–13,4 FPS.
  Houve melhora, mas o combate ainda precisa de otimização substancial.
- A opção de processamento paralelo utiliza o worker VIF/VU e o estágio GIF da
  base PC. É experimental, vem desligada por padrão e vale na próxima abertura.
  A seleção do estágio passou a ser feita por sessão, permitindo alternar o modo
  após sair e entrar no jogo dentro do mesmo processo Android.
- O modo paralelo abriu o menu com imagem no mesmo processo de uma sessão
  anterior sem workers. O vídeo inicial continuou em 30 FPS, e as primeiras
  janelas de combate ficaram em cerca de 17–19 FPS; durante um ataque o contador
  mostrou 14 FPS. Não é um resultado estável da campanha inteira.
- Com paralelo e FINISH assíncrono juntos, as janelas ficaram aproximadamente
  em 15–18 FPS, sem ganho adicional demonstrado. FINISH assíncrono foi desligado
  novamente. Esses testes não usam replay determinístico e variam com a câmera
  e a posição dos inimigos.
- Sair do combate em modo paralelo retornou ao launcher no mesmo PID; o log
  confirmou fechamento da janela e do áudio sem fatal. O POCO ficou com paralelo
  ligado e FINISH assíncrono desligado nas preferências de teste. Uma nova
  abertura paralela no mesmo PID exibiu o menu com imagem; o jogo foi pausado.
- APK local testado: `artifacts/GodOfWar2Android-debug.apk`, SHA256
  `2bc618b33f3e00a35c664fa4bb86405501e03fafbd82a7482cf5097d170357fb`.
  A assinatura e o alinhamento ZIP de 16 KiB foram verificados. O aparelho usa
  páginas de 4 KiB; isso não é um teste de boot num Android de páginas de 16 KiB.

## Critérios restantes

- Desempenho de combate estável e correção visual em outras áreas e shaders.
- Combate e QTE com multitoque; reconexão de controle físico.
- Saves/carregamento; mais ciclos de pausa/retomada.
- Medições do FPS real, consumo de memória e aquecimento em sessões longas.
- Validação da campanha inteira, incluindo bosses, vídeos e as duas camadas do DVD.
- Integração dos ajustes aos menus originais do jogo, além do overlay móvel.
