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

## Otimizações ARM64 e ritmo dos frames

- A invalidação do cache VU usa um bitmap de 256 bytes, e cada entrada de
  despacho ocupa 64 bytes, reduzindo o conjunto de entradas em 128 KiB.
  Reenvios MPG idênticos conservam o código já resolvido. O probe ARM64 passou
  em 34 casos, incluindo troca de programa, fallback, limites do bitmap e
  comparação das operações DIV/WAITQ e EFU/WAITP com o intérprete.
- A acumulação dos ciclos IOP passou a ser inline. Um milhão de partições
  aleatórias preservou exatamente as concessões de ciclos e o resto da divisão
  EE/IOP, incluindo drenagem parcial, fronteira de serviço e reset.
- Leituras indexadas P8/P4 usam tabelas NEON, e escritas CT32/Z32 agrupam pares
  adjacentes. Os 1.664 casos de linhas compararam os 13 formatos com operações
  escalares, incluindo retorno da VRAM e preservação dos outros canais.
- Os caminhos VIF V2-16, V3-8 e V4-32/16/8 expandem vetores em blocos.
  Os 43.200 casos verificaram extensões com e sem sinal, máscaras, STMOD,
  STCYCL, TOPS, NUM=0 e retorno da memória VU.
- A análise PACKED agrupa ST/RGBAQ/XYZ. Os 378 casos verificaram os três PATHs,
  fragmentos, tipos de primitivas, ADC, Q, NaN e profundidade de 32 bits.
  O envio imediato do árbitro evita uma cópia intermediária quando a fila está
  vazia; os testes verificaram prioridade, IMAGE fragmentado, posse dos buffers,
  reentrância e limpeza após exceção.
- A sincronização de registradores VIF1 e memória VU espera o worker VU,
  permitindo que o frontend processe sua cópia dos pacotes. Acessos GS/GIF e
  sincronização explícita continuam aguardando o frontend. O probe manteve
  um pacote bloqueado e verificou essas duas formas de espera e a imutabilidade
  dos bytes já enviados.
- O controle de ponto flutuante ARM64 usa uma leitura de FPCR, preservando o
  mapeamento SSE2NEON. Os 128 casos cobriram arredondamento, FZ/DAZ e bits
  preservados. A fila também verificou mudanças desse controle entre desenhos.
- O relógio de frames aplica a meta de 30 FPS na entrada real do frame do jogo,
  além da apresentação. Descarta atraso após pausas e frames lentos, e não
  transforma frames repetidos em frames novos. O probe passou em 100.000 prazos.
  O log `[gow-frames]` registra média, p50, p95, p99 e máximo dos intervalos.
- A sessão opcional de desempenho usa a
  [API oficial de hints do Android](https://developer.android.com/ndk/reference/group/a-performance-hint),
  resolvida dinamicamente para manter API 29. No POCO, a sessão aceitou as quatro
  threads do pipeline. Sua criação não comprova ganho de FPS.
- A biblioteca de produção passou novamente nos testes de pixels GLES, incluindo
  três ciclos EGL com framebuffer fetch e três com o fallback por cópias.
  Os 33 testes de kernel/agendamento passaram no aparelho. Uma expectativa antiga
  do teste de heap foi corrigida para excluir os pools reservados de RPC/TLS.
- Os 24 testes locais e os testes Java do importador também passaram.

O menu apresentou janelas próximas de 29,7–30 FPS. Isso não comprova 30 FPS em
combate. As medições anteriores à última expansão dos caminhos VIF continuaram
aproximadamente em 16–20 FPS na primeira luta. A espera cooperativa de FINISH
foi testada e retirada porque não demonstrou ganho material nesse trecho.

Os probes medem correção e custo isolado. Por exemplo, no mesmo núcleo do POCO,
524.160 vértices PACKED passaram de 22,659 para 16,884 ms; isso não representa
25% de ganho no FPS do jogo. Os resultados da campanha continuam pendentes.

Para diagnóstico, os arquivos `perf-counters`, `perf-diagnostics` e `perf-vif-only`
na pasta de dados ativam respectivamente contadores, escopos completos e apenas
estatísticas de formatos VIF. Reabra o processo ao alterá-los. `perf-adpf-off`
desativa os hints e pode ser alternado durante o jogo; a mudança é observada na
próxima janela de medição. O arquivo `perf-stage-barriers` repõe as barreiras completas para comparação
(requer novo processo). O arquivo `perf-gl-state-off` desativa o novo cache raster GLES para comparação
(requer novo processo). Remova esses arquivos ao concluir a medição.

## Rastreamento do pipeline e custo dos desenhos

- Um rastreamento Perfetto de 15 segundos na primeira luta registrou 12,15 s de
  CPU na thread GS, 9,83 s no worker VU, 8,59 s no EE e 5,12 s no frontend.
  A thread GS ficou executando por cerca de 81% do intervalo. A frequência média
  durante sua execução foi aproximadamente 1,65 GHz sem hints; outro trecho
  com hints ativos chegou a aproximadamente 1,96 GHz. As cenas não são um replay
  determinístico; esses números não estabelecem um ganho causal de FPS.
- A identificação das threads dos hints passou a usar registros dos próprios
  workers. O driver Adreno cria auxiliares com o nome herdado `GSThread`, o que
  fazia a busca por nome incluir oito threads. Os IDs registrados no log foram
  comparados com o processo: a sessão manteve os quatro workers reais, excluindo
  os quatro auxiliares, inclusive depois do vídeo de abertura.
- As cores dos vértices enviados à GPU usam quatro bytes não normalizados,
  preservando os valores 0–255 dos atributos anteriores. O vértice passou de
  64 para 52 bytes. Os testes de pixels verificaram também RGBA com 0, 1, 127,
  128, 254 e 255 nos dois caminhos GLES e em três grupos EGL sucessivos.
- O cache GLES evita reenviar propriedades raster idênticas. O cache de uniforms
  usa índices para locations densos e mantém o fallback para locations esparsos.
  Esses caches pertencem ao contexto do worker e são reiniciados por sessão EGL.
- Escritas TEX0 que não carregam CLUT deixaram de encerrar lotes. Recarregamentos
  com os mesmos pixels também preservam o lote; uma mudança real termina os
  desenhos anteriores antes de substituir a paleta. Se o próprio desenho fornece
  os pixels da CLUT, ele é enviado antes de ler essa fonte na GPU.
  O probe da CLUT passou em 81 verificações. O teste GLES verificou dois meios
  de uma imagem usando paletas vermelha e azul e uma paleta obtida de um desenho
  ainda pendente, mantendo os pixels esperados nos dois caminhos de feedback.
- A versão anterior à comparação de pixels da CLUT foi medida sem simpleperf:
  seis janelas consecutivas da luta ficaram entre 16,93 e 18,53 FPS; os p95 dos
  intervalos ficaram entre 60,55 e 65,61 ms. A introdução da sala foi excluída.
  Não houve estabilização em 30 FPS. Após a comparação de pixels da CLUT,
  outras seis janelas ficaram entre 18,75 e 19,38 FPS, com p95 entre 55,07 e
  63,42 ms. As execuções variam e não constituem um replay determinístico.
- A dependência da CLUT também considera a largura de um alvo gráfico já
  existente quando FRAME.FBW diminui. O teste reproduziu a cor antiga
  (80,100,120) sem essa proteção; com a correção, obteve (30,50,70), tanto com
  framebuffer fetch quanto por cópias, nos três ciclos EGL.
- A [CI do checkpoint 384d38d](https://github.com/GenissonEmilio/GodOfWar2Android/actions/runs/40180721600)
  passou. A variante ThinLTO também passou nos probes CPU, CLUT, kernel, fila e
  pixels GLES; isso não comprova ganho de FPS.
- Os rastreamentos completos, imagens, dados do jogo e APKs continuam locais.
  Estes testes não validam a campanha inteira.

## Lotes sem textura e apresentação repetida

- A preparação dos vértices foi incorporada ao chamador para reutilizar os
  cálculos do lote. No mesmo núcleo do POCO, após aquecer os buffers, preparar
  30 mil triângulos passou de aproximadamente 1,64–1,82 ms para 1,52–1,59 ms,
  conforme o formato Z. O tempo exclui o flush e a espera da GPU; não é FPS.
- A comparação completa dos snapshots da fila continua preservando os campos.
  O renderizador pode ignorar os 25 campos de textura quando os dois desenhos
  desativam TME. Mudanças de paleta também conservam geometria sem textura,
  exceto quando ela fornece os pixels que a própria CLUT vai ler.
- Os testes GLES incluem mil sprites sem textura intercalados com mudanças de
  estado de textura e cargas reais de paleta; os pixels permaneceram iguais.
  A fila passou novamente nos testes de ordem, concorrência e ponto flutuante.
- A apresentação compartilhada reutiliza a textura publicada quando os dois
  circuitos de exibição e seus alvos não mudaram. Capturas continuam compondo
  a imagem; reset, tela em branco e clear invalidam a reutilização. O teste
  verificou a mesma sequência numa repetição, uma nova sequência após desenho
  e clear, e a transição para tela em branco e retorno, nos dois caminhos GLES.
- Preservar a prioridade Android herdada foi testado, mas não demonstrou ganho
  de FPS e foi revertido. Não foram alterados governador nem configurações
  globais de desempenho do aparelho.
- Com os contadores gráficos ativos, a versão opt22 mediu 15,20–18,21 FPS nas
  seis janelas de combate. Das 150 apresentações pedidas a cada cinco segundos,
  59–74 reutilizaram a textura anterior; isso reduz trabalho duplicado e não
  aumenta a contagem de quadros reais do jogo.
- O teste atualizado de ThinLTO na primeira luta apresentou 17,93–18,69 FPS;
  não demonstrou ganho material. A opção permanece desligada por padrão.

O arquivo `perf-render-stats` ativa apenas os contadores do renderizador, sem
habilitar os escopos completos e os contadores VU. Inclui apresentações pedidas
e reutilizadas. Reabra o processo ao alterá-lo e remova-o ao terminar os testes.

## Estados imutáveis e comparação com gow2-recomp

- A [análise da referência PS3](GOW2-RECOMP-COMPARISON.md) identifica técnicas
  aplicáveis e diferenças de arquitetura. O código Android da referência foi
  inspecionado; o runtime externo indicado por ele retornou HTTP 404.
- Tokens identificam os snapshots imutáveis do frontend e do replay da fila.
  O estado completo continua sendo comparado quando não há identidade válida.
  Atributos de vértice continuam sendo atualizados em cada chamada.
- O probe da fila passou com 3.000 desenhos adicionais atravessando vários
  blocos, mudança de snapshot no meio e uma chamada sem token intercalada.
  Também preservou FINISH, concorrência, leitura e controle de ponto flutuante.
- Os probes de frontend/VIF/GIF passaram, incluindo 43.200 casos de unpack e
  378 casos de fluxo GIF. A identidade se manteve nas mudanças de vértices e
  mudou nas escritas de textura e no reset.
- Os testes GLES passaram nos dois caminhos de feedback e em três ciclos EGL.
  O teste adicional verificou pixels após mudar cores/coordenadas com o mesmo
  snapshot, intercalar uma chamada sem token e trocar a máscara de escrita.
- Nas seis medições sintéticas por formato Z, a mediana para preparar 30 mil
  triângulos caiu de 1,657 para 1,174 ms (Z32), 1,647 para 1,238 ms (Z24) e
  1,732 para 1,373 ms (Z16). Exclui flush/espera de GPU; não mede FPS do jogo.
- `PS2X_GS_QUEUE_BYTES` permite comparar blocos de 4–256 KiB; o padrão continua
  64 KiB. O marcador local `perf-queue-32k` seleciona 32 KiB na próxima abertura,
  com 16 blocos pendentes para preservar a capacidade total de aproximadamente
  512 KiB da fila padrão de oito blocos de 64 KiB.
  A alteração mantém registros inteiros e as mesmas barreiras de sincronização.
- O probe da fila passou com 64 KiB/oito blocos, 32 KiB/16 blocos e 4 KiB/um
  bloco. No mesmo APK opt23 e sem diagnósticos, seis janelas de cinco segundos
  após a introdução mediram 15,45–16,95 FPS com 64 KiB e 16,34–18,88 FPS com
  32 KiB. As medianas dessas janelas foram 16,25 e 16,44 FPS, respectivamente.
  Uma execução por configuração não estabelece ganho consistente; 64 KiB
  permanece como padrão. Menus e as duas primeiras janelas de introdução não
  entram nesses intervalos. A meta de 30 FPS continua não cumprida.

## Profundidade invertida sem cópia de vértices

- A transformação `1-z` dos passes de profundidade invertida passou para o
  vertex shader. Isso remove o vetor temporário, sua alocação e a cópia de
  todos os vértices em cada lote invertido. O teste de profundidade e sua
  direção de comparação permanecem iguais.
- Um teste de pixels passou antes e depois da alteração, nos dois caminhos
  GLES e em três ciclos EGL: escrita de Z32, inversão, fragmentos aceitos e
  rejeitados, nova inversão e retorno à comparação normal. Sem erros GL.
- O APK opt24 foi instalado e, sem diagnósticos e com blocos de 64 KiB, mediu
  14,33–18,75 FPS nas seis janelas de combate; a mediana das janelas foi 18,06.
  A pior janela teve p95 de 110,08 ms. É uma execução, não uma prova de ganho
  sustentado ou de campanha validada. O objetivo de 30 FPS estáveis não foi atingido.
- A assinatura e o alinhamento do APK foram verificados; os segmentos ELF estão
  alinhados a 16 KiB. Os marcadores de diagnóstico e de blocos de 32 KiB foram
  removidos do aparelho. A compilação normal permanece instalada.

## Recorte, inicialização e aritmética dos shaders

- O descarte conservador passou em 200 mil triângulos contra recorte de
  polígonos e nos testes GLES de borda, textura e degeneração. Na primeira
  luta, encontrou aproximadamente 10–19% de triângulos fora da área visível,
  mas as seis janelas com contadores mediram 15,84–17,24 FPS. O experimento
  permanece desligado por padrão (`perf-culling` o ativa ao reabrir).
- A leitura de `gpubusy`, sem mudar clocks nem governadores, mostrou cerca de
  80% de ocupação mediana da GPU em 27 amostras de combate. Esse número inclui
  o restante do trabalho gráfico do sistema e não é tempo exclusivo do jogo.
- A criação de alvos de cor e profundidade agora define máscaras e desativa
  o scissor antes da limpeza inicial. O teste deixa uma máscara parcial e
  escrita de profundidade desativada antes de criar outro alvo; verificou a
  área desenhada e os pixels que devem permanecer pretos.
- A repetição de texturas com dimensões em potências de dois usa máscaras;
  endereços de alvos de 16/32 bits usam shifts. Dimensões não compatíveis
  mantêm divisão e módulo. `perf-integer-off` seleciona o caminho genérico.
- Os dois caminhos produziram os mesmos resultados esperados em 4.096 pixels
  de uma textura repetida com UVs negativos, tanto por ponto quanto bilinear.
  O teste usa o alvo bruto, antes da repetição intencional da borda de overscan
  na apresentação. Os três ciclos EGL passaram com fetch e com cópias, nos
  modos rápido, genérico e com descarte: 18 execuções, sem erros GL.

## Caminho sem textura sem framebuffer fetch

- Foi adicionado um programa simples de cor para lotes sem textura que não
  precisam ler o destino nem descartar fragmentos. Não declara entrada de
  framebuffer fetch. O shader geral continua atendendo texturas, blend por
  shader, DATE, Z alias e testes de alfa que descartam fragmentos.
- Os pixels passaram com o programa simples e com o geral, nos dois caminhos
  GLES e em três ciclos EGL. O teste inclui cinco incrementos e três decrementos
  de contagem de sombra (G=127 → 129), rejeição por Alpha NEVER e DATE, além das
  regressões de paleta, inicialização, profundidade e repetição de texturas.
- Os contadores não calculam o recorte de triângulos quando o experimento está
  desligado. O arquivo `perf-simple-off` seleciona somente o programa geral na
  próxima abertura, para comparação. A medição no jogo ainda está em andamento.

## Critérios restantes

- Desempenho de combate estável e correção visual em outras áreas e shaders.
- Combate e QTE com multitoque; reconexão de controle físico.
- Saves/carregamento; mais ciclos de pausa/retomada.
- Medições do FPS real, consumo de memória e aquecimento em sessões longas.
- Validação da campanha inteira, incluindo bosses, vídeos e as duas camadas do DVD.
- Integração dos ajustes aos menus originais do jogo, além do overlay móvel.


### Shaders comuns e leituras do destino (opt28–opt29)

O `opt28` usou o shader simples nas passagens sem textura compatíveis. Na primeira
luta, com contadores de renderização e leitura do `gpubusy` a cada segundo, as seis
janelas de combate registraram 14,975; 14,594; 14,497; 14,769; 14,557; 14,957 FPS
(mediana 14,682). O shader simples apareceu em cerca de 150 batches entre 15–16 mil
por janela, portanto alcançou uma fração pequena do trabalho. Não há ganho de
combate demonstrado. A bateria chegou a 38,9 °C; não se pode atribuir a diferença
entre execuções somente ao código. A mediana do indicador global `gpubusy` nos
últimos 30 s foi 72,7%, em 29 amostras. No `opt27`, foi 73,4%; as seis janelas de
combate tiveram mediana de 16,964 FPS. Esse indicador não isola o processo do jogo.

O `opt29` compila uma segunda variante do shader completo, sem leitura do destino,
selecionada somente com DATE, mistura por shader e alias de Z desativados no estado
GS. Texturas, filtragem, releitura de render targets, teste de alpha e seu passe de
falha continuam com as mesmas operações. Cada programa guarda suas próprias
localizações e valores de uniforms. `perf-direct-off` permite medir o caminho
anterior na mesma compilação. Falha ao compilar ou ligar a variante opcional mantém
o shader geral.

O teste no Adreno validou pixels RGBA durante alternância entre textura comum,
RGB_ONLY após falha de alpha, DATE aceitando/rejeitando, mistura por alpha do
destino e teste/escrita de Z armazenado na cor. Os testes anteriores de CLUT,
repetição negativa e bilinear também passaram. Foram 12 ciclos EGL: variante
nova e geral, cada uma com framebuffer fetch e cópia de fallback, repetidos três
vezes. A medição de combate do `opt29` ainda está em andamento; estes testes de
pixels não comprovam 30 FPS nem validam toda a campanha.

### Mistura com duas saídas e limites de alpha (opt31)

A [pesquisa em outros renderizadores](RENDERER-RESEARCH.md) levou à consulta das
extensões do aparelho. O Adreno 810 anuncia `GL_EXT_blend_func_extended`, mas o
adaptador removia a segunda saída do fragment shader em todo Android. Agora a
variante sem leitura do destino pode manter essa saída e usar os fatores SRC1.
O caminho só é usado quando os limites calculados de vértices, texel/CLUT e TFX
provam alpha <=128. DATE, alias de Z, operações não representáveis, limites
incertos e alpha/FIX acima de 128 conservam a equação no shader. As mudanças de
imagem/CLUT recalculam os limites. A opção `perf-dual-off` força a comparação.

O teste independente da equação do GS passou em 640 casos por ciclo: imagem
RGBA e paletizada, quatro TFX, TCC ligado/desligado, fatores As/FIX e alpha
0/64/128/129/255. Foram seis ciclos EGL com duas saídas e seis com a opção
inativa, divididos igualmente entre framebuffer fetch e cópia de fallback.
Nenhum erro GL ou divergência superior à tolerância de dois níveis de RGBA.
As 24 verificações Python/Java também passaram.

Na primeira luta, com estatísticas e leitura de `gpubusy`, as seis janelas após a
transição registraram **17,370; 18,218; 18,399; 17,381; 18,078; 17,250 FPS**:
mediana **17,730**, contra 15,977 na rodada `opt29`. A diferença entre essas
execuções é de aproximadamente 11%; ainda não é uma estimativa controlada do
ganho. O indicador global de GPU teve mediana 65,7% em 29 amostras nos últimos
30 s. A bateria registrou 38,4 °C durante o teste.

O uso do shader de mistura caiu de cerca de 12–13 mil batches por janela para
1,3–1,4 mil; a variante comum passou a atender cerca de 12–13 mil batches. Isso
confirma a mudança do trabalho executado, mas **não cumpre os 30 FPS**. Os p95
das janelas de combate ficaram entre 59,457 e 69,757 ms. As capturas mostram a
luta, texturas e HUD; as sombras duras ainda precisam de comparação com uma
referência no mesmo ponto. Não há validação de toda a campanha.

### Envio de vértices por buffer persistente (opt32)

A segunda experiência da pesquisa usa `GL_EXT_buffer_storage` no Adreno. Um anel
com quatro regiões de cerca de 8 MiB recebe os vértices por mapeamento coerente.
Fences protegem a reutilização; batches maiores que uma região usam o VBO/VAO
anterior. As passagens de falha de alpha e as misturas por triângulo usam o mesmo
offset do desenho principal. A ausência da extensão, falha de alocação ou
`perf-stream-off` mantém `BufferData`. Buffer, mapa e fences são liberados antes
do contexto EGL.

Os testes passaram em seis ciclos EGL normais e seis com regiões reduzidas a
53.248 bytes, divididos entre fetch e cópia. O teste submete 2.048 batches antes
da leitura, verifica os últimos pixels de 64 células após voltas do anel e
exercita o fallback para batches grandes. Três ciclos com o envio antigo também
passaram. Os 640 casos de mistura e testes anteriores permanecem sem erro GL.
Depois da inclusão da liberação explícita, os seis ciclos normais passaram de
novo. As 24 verificações Python/Java passaram.

Combate: **19,519; 19,604; 18,890; 18,629; 18,229; 17,998 FPS**, mediana **18,760**,
p95 de 52,687 a 61,432 ms. As duas primeiras janelas de transição (29,000/23,663)
foram excluídas. O indicador global de GPU teve mediana 63,9% em 29 amostras nos
últimos 30 s. A diferença entre as rodadas `opt29` e `opt32` é aproximadamente
17%; entre `opt31` e `opt32`, 6%. São observações de execuções distintas, com
variação do combate e da temperatura. Não demonstram 30 FPS sustentados.

O perfil subsequente de 16 s coletou 20.811 amostras sem perda. A participação
nos ciclos de CPU de usuário foi VUWorker 35,66%, GameThread 25,58%, GSThread
22,90% e GSFrontend 11,60%. Isso orienta a próxima investigação para VIF/VU,
sem interpretar essas participações como tempo de parede ou ganho possível.
